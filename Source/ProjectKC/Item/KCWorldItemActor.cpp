#include "ProjectKC/Item/KCWorldItemActor.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Misc/DataValidation.h"
#include "Net/UnrealNetwork.h"
#include "ProjectKC/AbilitySystem/Component/KCAbilitySourceComponent.h"
#include "ProjectKC/AbilitySystem/Component/KCAbilitySystemComponent.h"
#include "ProjectKC/Item/Component/KCHeldItemComponent.h"
#include "ProjectKC/Item/Definition/KCItemDefinition.h"

DEFINE_LOG_CATEGORY_STATIC(LogKCWorldItem, Log, All);

AKCWorldItemActor::AKCWorldItemActor()
{
	bReplicates = true;
	SetReplicateMovement(true);

	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
	SetRootComponent(ItemMesh);
	ItemMesh->SetCollisionProfileName(TEXT("PhysicsActor"));
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
		!IsValid(NewDefinition))
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

	const FTransform PreviousWorldTransform = GetActorTransform();
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

	if (!AlignGripToAttachmentSocket())
	{
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		SetActorTransform(PreviousWorldTransform);
		ApplyStatePresentation();
		return false;
	}

	RuntimeState.Holder = NewHolder;
	RuntimeState.State = EKCWorldItemState::Held;
	SetOwner(NewHolder);

	if (IsUsable())
	{
		UKCAbilitySystemComponent* HolderAbilitySystem =
			Cast<UKCAbilitySystemComponent>(
				UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(NewHolder));
		if (!HolderAbilitySystem ||
			!AbilitySourceComponent->GrantToAbilitySystem(HolderAbilitySystem))
		{
			SetOwner(nullptr);
			RuntimeState.Holder = nullptr;
			RuntimeState.State = EKCWorldItemState::World;
			DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
			SetActorTransform(PreviousWorldTransform);
			ApplyStatePresentation();
			return false;
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

	if (!AbilitySourceComponent->Revoke(true))
	{
		return false;
	}
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetOwner(nullptr);
	RuntimeState.Holder = nullptr;
	RuntimeState.State = EKCWorldItemState::World;
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

bool AKCWorldItemActor::ActivateUse()
{
	return RuntimeState.State == EKCWorldItemState::Held &&
		IsUsable() &&
		AbilitySourceComponent->TryActivate();
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
		bDefinitionValid;
}

bool AKCWorldItemActor::IsUsable() const
{
	return bDefinitionValid && ItemDefinition->IsUsable();
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
}

void AKCWorldItemActor::OnRep_RuntimeState()
{
	ApplyStatePresentation();
	RefreshReplicatedAttachment();
	BroadcastStateChanged();
}

void AKCWorldItemActor::OnRep_ItemDefinition()
{
	RefreshDefinition();
	ApplyStatePresentation();
	RefreshReplicatedAttachment();
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

bool AKCWorldItemActor::AlignGripToAttachmentSocket()
{
	if (!ItemDefinition || !ItemMesh)
	{
		return false;
	}

	FTransform AlignmentTransform;
	if (!ItemDefinition->Presentation.TryGetGripAlignmentTransform(
		AlignmentTransform))
	{
		return false;
	}

	SetActorRelativeTransform(AlignmentTransform);
	return true;
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

	if (!AlignGripToAttachmentSocket())
	{
		UE_LOG(
			LogKCWorldItem,
			Verbose,
			TEXT("Client Item '%s'의 Grip 정렬 데이터가 아직 준비되지 않았습니다."),
			*GetName());
	}
}

void AKCWorldItemActor::ApplyStatePresentation()
{
	if (!bDefinitionValid)
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
