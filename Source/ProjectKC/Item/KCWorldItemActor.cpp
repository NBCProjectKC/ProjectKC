#include "ProjectKC/Item/KCWorldItemActor.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/DataValidation.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "ProjectKC/AbilitySystem/Component/KCAbilitySourceComponent.h"
#include "ProjectKC/AbilitySystem/Component/KCAbilitySystemComponent.h"
#include "ProjectKC/Item/Component/KCHeldItemComponent.h"
#include "ProjectKC/Item/Definition/KCItemDefinition.h"
#include "ProjectKC/Messages/KCGameplayTags.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogKCWorldItem, Log, All);

AKCWorldItemActor::AKCWorldItemActor()
{
	bReplicates = true;
	SetReplicateMovement(true);

	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
	SetRootComponent(ItemMesh);
	ItemMesh->SetCollisionProfileName(TEXT("KCWorldItem"));
	ItemMesh->SetGenerateOverlapEvents(true);
	ItemMesh->SetSimulatePhysics(false);
	ItemMesh->ComponentTags.AddUnique(TEXT("Interactable"));

	AbilitySourceComponent =
		CreateDefaultSubobject<UKCAbilitySourceComponent>(TEXT("AbilitySource"));
}

void AKCWorldItemActor::Interact_Implementation(AActor* Interactor)
{
	if (!HasAuthority() || !IsValid(Interactor) || !CanBePickedUp())
	{
		return;
	}

	if (UKCHeldItemComponent* HolderItemComponent =
		Interactor->FindComponentByClass<UKCHeldItemComponent>())
	{
		HolderItemComponent->TryPickUp(this);
	}
}

FGameplayTag AKCWorldItemActor::GetInteractionPromptTag_Implementation(
	AActor* Interactor) const
{
	const UKCHeldItemComponent* HeldItemComponent = Interactor
		? Interactor->FindComponentByClass<UKCHeldItemComponent>()
		: nullptr;
	if (!CanBePickedUp() || !HeldItemComponent ||
		HeldItemComponent->HasHeldItem())
	{
		return FGameplayTag();
	}

	return KCGameplayTags::Interaction_Item_PickUp;
}

FVector AKCWorldItemActor::GetInteractionPromptWorldLocation_Implementation(
	AActor* Interactor) const
{
	return ItemMesh ? ItemMesh->Bounds.Origin : GetActorLocation();
}

void AKCWorldItemActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshDefinition();
	ApplyStatePresentation();
}

#if WITH_EDITOR
EDataValidationResult AKCWorldItemActor::IsDataValid(
	FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	FString Error;
	if (!ItemDefinition || !ItemDefinition->Validate(Error))
	{
		Context.AddError(ItemDefinition
			? FText::FromString(Error)
			: FText::FromString(TEXT("ItemDefinition이 비어 있습니다.")));
		return EDataValidationResult::Invalid;
	}

	return Result == EDataValidationResult::NotValidated
		? EDataValidationResult::Valid
		: Result;
}
#endif

void AKCWorldItemActor::BeginPlay()
{
	Super::BeginPlay();
	FString DefinitionError;
	if (!RefreshDefinition(&DefinitionError))
	{
		UE_LOG(
			LogKCWorldItem,
			Error,
			TEXT("Item '%s'의 Definition이 유효하지 않습니다: %s"),
			*GetName(),
			*DefinitionError);
	}
	ApplyStatePresentation();
}

bool AKCWorldItemActor::InitializeItem(UKCItemDefinition* NewDefinition)
{
	if (!HasAuthority() || RuntimeState.State != EKCWorldItemState::World ||
		!IsValid(NewDefinition) || bUseConsumptionPending ||
		bUseConsumptionDestructionScheduled)
	{
		return false;
	}

	UKCItemDefinition* PreviousDefinition = ItemDefinition;
	ItemDefinition = NewDefinition;
	FString Error;
	if (!RefreshDefinition(&Error))
	{
		ItemDefinition = PreviousDefinition;
		RefreshDefinition();
		return false;
	}

	ResetDurability();
	bUseConsumptionPending = false;
	bUseConsumptionDestructionScheduled = false;
	ApplyStatePresentation();
	ForceNetUpdate();
	return true;
}

bool AKCWorldItemActor::EnterHeldState(
	AActor* NewHolder,
	USceneComponent* AttachParent,
	FName AttachSocket)
{
	if (!HasAuthority() || !CanBePickedUp() || !IsValid(NewHolder))
	{
		return false;
	}

	if (!AttachParent)
	{
		AttachParent = NewHolder->GetRootComponent();
	}
	if (!AttachParent || AttachParent->GetOwner() != NewHolder)
	{
		return false;
	}

	ItemMesh->SetSimulatePhysics(false);
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (!AttachToComponent(
		AttachParent,
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		AttachSocket))
	{
		AbilitySourceComponent->Revoke(true);
		ApplyStatePresentation();
		return false;
	}

	AlignGripToAttachmentSocket();

	RuntimeState.Holder = NewHolder;
	RuntimeState.State = EKCWorldItemState::Held;
	SetOwner(NewHolder);

	// 사용 Ability 부여에 실패해도 운반은 가능하다.
	// 사용만 막히고, TryActivate()가 부여된 Handle이 없으므로 자연히 false를 반환한다.
	if (bDefinitionValid && ItemDefinition->IsUsable())
	{
		UKCAbilitySystemComponent* HolderAbilitySystem =
			Cast<UKCAbilitySystemComponent>(
				UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(NewHolder));
		if (!HolderAbilitySystem ||
			!AbilitySourceComponent->GrantToAbilitySystem(HolderAbilitySystem))
		{
			UE_LOG(
				LogKCWorldItem,
				Warning,
				TEXT("Holder '%s'에 KC ASC가 없거나 부여에 실패해 Item '%s'을 운반 전용으로 듭니다."),
				*GetNameSafe(NewHolder),
				*GetName());
		}
	}

	ForceNetUpdate();
	BroadcastStateChanged();
	return true;
}

bool AKCWorldItemActor::ExitHeldState(
	const FTransform& DropTransform,
	const FVector& DropImpulse)
{
	if (!HasAuthority() || RuntimeState.State != EKCWorldItemState::Held)
	{
		return false;
	}

	// 회수에 실패해도 드롭은 진행한다. 아이템이 손에 갇히는 쪽이 더 나쁘다.
	// 남은 Spec은 Held 상태가 아니면 PressUse()가 막고, 재획득 시 재사용된다.
	if (!AbilitySourceComponent->Revoke(true))
	{
		UE_LOG(
			LogKCWorldItem,
			Warning,
			TEXT("Item '%s'의 Ability 회수에 실패했지만 드롭은 진행합니다."),
			*GetName());
	}
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetOwner(nullptr);
	RuntimeState.Holder = nullptr;
	RuntimeState.State = EKCWorldItemState::World;
	// 클라이언트가 물리를 켜기 전에 같은 지점으로 옮길 수 있도록 함께 복제한다.
	RuntimeState.DropLocation = DropTransform.GetLocation();
	RuntimeState.DropRotation = DropTransform.Rotator();
	SetActorTransform(DropTransform, false, nullptr, ETeleportType::TeleportPhysics);
	ApplyStatePresentation();

	if (!DropImpulse.IsNearlyZero() && ItemMesh->IsSimulatingPhysics())
	{
		ItemMesh->AddImpulse(DropImpulse, NAME_None, true);
	}

	ForceNetUpdate();
	BroadcastStateChanged();
	return true;
}

bool AKCWorldItemActor::PressUse(
	FGameplayAbilitySpecHandle& OutPressedHandle)
{
	return RuntimeState.State == EKCWorldItemState::Held &&
		IsUsable() &&
		AbilitySourceComponent->PressInput(OutPressedHandle);
}

bool AKCWorldItemActor::ReleaseUse(FGameplayAbilitySpecHandle PressedHandle)
{
	return bDefinitionValid && ItemDefinition->IsUsable() &&
		AbilitySourceComponent->ReleaseInput(PressedHandle);
}

bool AKCWorldItemActor::ActivateUseWithTarget(AActor* TargetActor)
{
	return HasAuthority() &&
		RuntimeState.State == EKCWorldItemState::Held &&
		IsUsable() &&
		AbilitySourceComponent->TryActivateWithTarget(TargetActor);
}

bool AKCWorldItemActor::CanBePickedUp() const
{
	return RuntimeState.State == EKCWorldItemState::World &&
		bDefinitionValid && !bUseConsumptionPending;
}

bool AKCWorldItemActor::IsUsable() const
{
	return bDefinitionValid && ItemDefinition->IsUsable() &&
		!IsBroken() && !bUseConsumptionPending;
}

bool AKCWorldItemActor::UsesDurability() const
{
	return bDefinitionValid && ItemDefinition &&
		ItemDefinition->Durability.IsEnabled();
}

bool AKCWorldItemActor::IsBroken() const
{
	return UsesDurability() && CurrentDurability <= 0.0f;
}

float AKCWorldItemActor::GetCurrentDurability() const
{
	return CurrentDurability;
}

float AKCWorldItemActor::GetMaximumDurability() const
{
	return FKCItemDurabilityStruct::MaximumDurability;
}

float AKCWorldItemActor::GetDurabilityNormalized() const
{
	return FMath::Clamp(
		CurrentDurability / FKCItemDurabilityStruct::MaximumDurability,
		0.0f,
		1.0f);
}

bool AKCWorldItemActor::IsUseConsumptionPending() const
{
	return bUseConsumptionPending;
}

bool AKCWorldItemActor::TryBeginUseConsumption()
{
	if (!HasAuthority() || !bDefinitionValid || !ItemDefinition ||
		ItemDefinition->UseLifecycle !=
			EKCItemUseLifecycle::ConsumeOnSuccessfulExecute ||
		bUseConsumptionPending || bUseConsumptionDestructionScheduled)
	{
		return false;
	}

	bUseConsumptionPending = true;
	ApplyStatePresentation();
	ForceNetUpdate();
	return true;
}

bool AKCWorldItemActor::FinalizePendingUseConsumption()
{
	if (!HasAuthority() || !bUseConsumptionPending ||
		bUseConsumptionDestructionScheduled)
	{
		return false;
	}

	bUseConsumptionDestructionScheduled = true;
	GetWorldTimerManager().SetTimerForNextTick(
		this,
		&AKCWorldItemActor::DestroyConsumedItem);
	return true;
}

bool AKCWorldItemActor::TryConsumeDurability(
	EKCItemDurabilityConsumeMode ConsumeMode,
	float ConsumptionScale)
{
	if (!HasAuthority() || !UsesDurability() || IsBroken() ||
		ItemDefinition->Durability.ConsumeMode != ConsumeMode ||
		!FMath::IsFinite(ConsumptionScale) || ConsumptionScale <= 0.0f)
	{
		return false;
	}

	const float Consumption =
		ItemDefinition->Durability.ConsumeAmount * ConsumptionScale;
	if (!FMath::IsFinite(Consumption) || Consumption <= 0.0f)
	{
		return false;
	}

	SetCurrentDurability(CurrentDurability - Consumption);
	return true;
}

EKCWorldItemState AKCWorldItemActor::GetItemState() const
{
	return RuntimeState.State;
}

AActor* AKCWorldItemActor::GetHolder() const
{
	return RuntimeState.Holder;
}

UKCItemDefinition* AKCWorldItemActor::GetItemDefinition() const
{
	return ItemDefinition;
}

UStaticMeshComponent* AKCWorldItemActor::GetItemMesh() const
{
	return ItemMesh;
}

void AKCWorldItemActor::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AKCWorldItemActor, RuntimeState);
	DOREPLIFETIME(AKCWorldItemActor, ItemDefinition);
	DOREPLIFETIME(AKCWorldItemActor, CurrentDurability);
	DOREPLIFETIME(AKCWorldItemActor, bUseConsumptionPending);
}

void AKCWorldItemActor::OnRep_RuntimeState()
{
	// 손에 들려 있던 아이템이 World로 돌아온 순간만 드롭으로 취급한다.
	// 뒤늦게 관련성이 생긴 클라이언트가 예전 드롭 지점으로 되돌리지 않게 한다.
	const bool bDroppedFromHand =
		bHasObservedState &&
		LastObservedState == EKCWorldItemState::Held &&
		RuntimeState.State == EKCWorldItemState::World;
	LastObservedState = RuntimeState.State;
	bHasObservedState = true;

	if (bDroppedFromHand)
	{
		// 서버 ExitHeldState()와 같은 순서다. 부착 해제 → 위치 확정 → 물리 활성화.
		// 물리를 먼저 켜면 손 위치에서 낙하한 뒤 보정으로 끌려가게 된다.
		RefreshReplicatedAttachment();
		SetActorTransform(
			FTransform(RuntimeState.DropRotation, RuntimeState.DropLocation),
			false,
			nullptr,
			ETeleportType::TeleportPhysics);
		ApplyStatePresentation();
	}
	else
	{
		// 서버 EnterHeldState()와 같은 순서다. 물리를 멈춘 뒤 부착해야 웰딩되지 않는다.
		ApplyStatePresentation();
		RefreshReplicatedAttachment();
	}
	BroadcastStateChanged();
}

void AKCWorldItemActor::OnRep_ItemDefinition()
{
	RefreshDefinition();
	ApplyStatePresentation();
	RefreshReplicatedAttachment();
}

void AKCWorldItemActor::OnRep_CurrentDurability(float PreviousDurability)
{
	BroadcastDurabilityChanged(PreviousDurability);
}

void AKCWorldItemActor::OnRep_UseConsumptionPending()
{
	ApplyStatePresentation();
}

void AKCWorldItemActor::MulticastPlayBreakEffects_Implementation(
	FVector_NetQuantize BreakLocation,
	FRotator BreakRotation)
{
	if (GetNetMode() == NM_DedicatedServer || !ItemDefinition)
	{
		return;
	}

	if (ItemDefinition->Durability.BreakSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			ItemDefinition->Durability.BreakSound,
			BreakLocation,
			BreakRotation);
	}

	if (ItemDefinition->Durability.BreakVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this,
			ItemDefinition->Durability.BreakVFX,
			BreakLocation,
			BreakRotation);
	}
}

bool AKCWorldItemActor::RefreshDefinition(FString* OutError)
{
	bDefinitionValid = false;
	if (!ItemDefinition)
	{
		ItemMesh->SetStaticMesh(nullptr);
		AbilitySourceComponent->ConfigureAbilityDefinition(nullptr);
		if (OutError)
		{
			*OutError = TEXT("ItemDefinition이 비어 있습니다.");
		}
		return false;
	}

	FString ValidationError;
	if (!ItemDefinition->Validate(ValidationError))
	{
		ItemMesh->SetStaticMesh(nullptr);
		AbilitySourceComponent->ConfigureAbilityDefinition(nullptr);
		if (OutError)
		{
			*OutError = MoveTemp(ValidationError);
		}
		return false;
	}

	ItemMesh->SetStaticMesh(ItemDefinition->Presentation.StaticMesh);
	if (!AbilitySourceComponent->ConfigureAbilityDefinition(
		ItemDefinition->UseAction))
	{
		if (OutError)
		{
			*OutError = TEXT("Grant 중인 Ability Definition은 교체할 수 없습니다.");
		}
		return false;
	}

	bDefinitionValid = true;
	if (OutError)
	{
		OutError->Reset();
	}
	return true;
}

void AKCWorldItemActor::AlignGripToAttachmentSocket()
{
	// Grip 소켓은 선택 사항이다. 없으면 아이템 원점을 Hand 소켓에 맞춘다.
	FTransform AlignmentTransform = FTransform::Identity;
	if (ItemDefinition &&
		!ItemDefinition->Presentation.TryGetGripAlignmentTransform(
			AlignmentTransform))
	{
		UE_LOG(
			LogKCWorldItem,
			Verbose,
			TEXT("Item '%s'에 Grip 소켓이 없어 원점을 Hand 소켓에 맞춥니다."),
			*GetName());
	}

	SetActorRelativeTransform(AlignmentTransform);
}

void AKCWorldItemActor::RefreshReplicatedAttachment()
{
	if (HasAuthority())
	{
		return;
	}

	if (RuntimeState.State == EKCWorldItemState::World)
	{
		if (GetRootComponent() && GetRootComponent()->GetAttachParent())
		{
			DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		}
		return;
	}

	if (!IsValid(RuntimeState.Holder))
	{
		return;
	}

	UKCHeldItemComponent* HolderItemComponent =
		RuntimeState.Holder->FindComponentByClass<UKCHeldItemComponent>();
	USceneComponent* AttachParent = HolderItemComponent
		? HolderItemComponent->ResolveAttachmentComponent()
		: nullptr;
	const FName AttachSocket = HolderItemComponent
		? HolderItemComponent->HandSocketName
		: NAME_None;
	if (!AttachParent || AttachSocket.IsNone() ||
		!AttachParent->DoesSocketExist(AttachSocket))
	{
		UE_LOG(
			LogKCWorldItem,
			Verbose,
			TEXT("Client Item '%s'의 Holder '%s'에서 부착 소켓을 아직 확인할 수 없습니다."),
			*GetName(),
			*GetNameSafe(RuntimeState.Holder));
		return;
	}

	const bool bAlreadyAttached =
		GetRootComponent() &&
		GetRootComponent()->GetAttachParent() == AttachParent &&
		GetAttachParentSocketName() == AttachSocket;
	if (!bAlreadyAttached &&
		!AttachToComponent(
			AttachParent,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			AttachSocket))
	{
		UE_LOG(
			LogKCWorldItem,
			Warning,
			TEXT("Client Item '%s'을 Holder '%s'의 소켓 '%s'에 부착하지 못했습니다."),
			*GetName(),
			*GetNameSafe(RuntimeState.Holder),
			*AttachSocket.ToString());
		return;
	}

	AlignGripToAttachmentSocket();
}

void AKCWorldItemActor::ApplyStatePresentation()
{
	ItemMesh->SetVisibility(!bUseConsumptionPending, true);

	if (!bDefinitionValid || bUseConsumptionPending)
	{
		ItemMesh->SetSimulatePhysics(false);
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return;
	}

	if (RuntimeState.State == EKCWorldItemState::Held)
	{
		ItemMesh->SetSimulatePhysics(false);
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return;
	}

	ItemMesh->SetCollisionProfileName(
		ItemDefinition->Presentation.WorldCollisionProfile);
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ItemMesh->SetSimulatePhysics(
		ItemDefinition->Presentation.bSimulatePhysicsInWorld);
}

void AKCWorldItemActor::BroadcastStateChanged()
{
	OnItemStateChanged.Broadcast(RuntimeState.State, RuntimeState.Holder);
}

void AKCWorldItemActor::ResetDurability()
{
	bBreakDestructionScheduled = false;
	SetCurrentDurability(FKCItemDurabilityStruct::MaximumDurability);
}

void AKCWorldItemActor::SetCurrentDurability(float NewDurability)
{
	const float ClampedDurability = FMath::Clamp(
		NewDurability,
		0.0f,
		FKCItemDurabilityStruct::MaximumDurability);
	if (FMath::IsNearlyEqual(CurrentDurability, ClampedDurability))
	{
		return;
	}

	const float PreviousDurability = CurrentDurability;
	CurrentDurability = ClampedDurability;
	ForceNetUpdate();
	BroadcastDurabilityChanged(PreviousDurability);
}

void AKCWorldItemActor::BroadcastDurabilityChanged(float PreviousDurability)
{
	OnDurabilityChanged.Broadcast(PreviousDurability, CurrentDurability);
	if (PreviousDurability > 0.0f && CurrentDurability <= 0.0f)
	{
		OnItemBroken.Broadcast();
		HandleBroken();
	}
}

void AKCWorldItemActor::HandleBroken()
{
	if (!HasAuthority() || !ShouldDestroyWhenBroken() ||
		bBreakDestructionScheduled)
	{
		return;
	}

	bBreakDestructionScheduled = true;
	MulticastPlayBreakEffects(GetActorLocation(), GetActorRotation());

	// 명중 처리나 Ability 실행 도중 Source Actor를 바로 없애지 않는다.
	// 다음 틱에 Ability/보유 참조를 먼저 정리한 뒤 Actor를 파괴한다.
	GetWorldTimerManager().SetTimerForNextTick(
		this,
		&AKCWorldItemActor::DestroyBrokenItem);
}

void AKCWorldItemActor::DestroyBrokenItem()
{
	bBreakDestructionScheduled = false;
	if (!HasAuthority() || !IsBroken() || !ShouldDestroyWhenBroken())
	{
		return;
	}

	DestroyItemActor();
}

void AKCWorldItemActor::DestroyConsumedItem()
{
	bUseConsumptionDestructionScheduled = false;
	if (!HasAuthority() || !bUseConsumptionPending)
	{
		return;
	}

	DestroyItemActor();
}

void AKCWorldItemActor::DestroyItemActor()
{
	if (!HasAuthority())
	{
		return;
	}

	if (RuntimeState.State == EKCWorldItemState::Held &&
		IsValid(RuntimeState.Holder))
	{
		if (UKCHeldItemComponent* HolderItemComponent =
			RuntimeState.Holder->FindComponentByClass<UKCHeldItemComponent>())
		{
			HolderItemComponent->ClearHeldItemForDestruction(this);
		}
	}

	// 비정상 보유 참조에서도 활성 Ability와 부착 상태가 남지 않게 한다.
	if (RuntimeState.State == EKCWorldItemState::Held)
	{
		ExitHeldState(GetActorTransform(), FVector::ZeroVector);
	}

	Destroy();
}

bool AKCWorldItemActor::ShouldDestroyWhenBroken() const
{
	return UsesDurability() &&
		ItemDefinition->Durability.BreakBehavior ==
			EKCItemBreakBehavior::Destroy;
}
