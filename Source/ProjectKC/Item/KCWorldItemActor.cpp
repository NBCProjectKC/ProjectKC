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

/**
 * @brief Initializes the replicated world-item actor and its mesh and ability-source components.
 */
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

/**
 * @brief Requests pickup of this item for a valid interactor when the item can be picked up.
 *
 * @param Interactor Actor attempting to pick up the item.
 */
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

/**
 * @brief Refreshes the item definition and applies the corresponding presentation during construction.
 *
 * @param Transform The actor transform used for construction.
 */
void AKCWorldItemActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshDefinition();
	ApplyStatePresentation();
}

#if WITH_EDITOR
/**
 * @brief Validates the item's definition and inherited actor data.
 *
 * @param Context Validation context that receives an error when the item definition is missing or invalid.
 * @return EDataValidationResult Validation result for this actor.
 */
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

/**
 * @brief Initializes the item definition and applies its current state presentation when play begins.
 */
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

/**
 * @brief Replaces the item's definition while it is in the world.
 *
 * @param NewDefinition Valid definition to assign to the item.
 * @return `true` if the definition was initialized successfully, `false` otherwise.
 */
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

/**
 * @brief Transitions the item into a held state and attaches it to the holder.
 *
 * @param NewHolder Actor that will hold the item.
 * @param AttachParent Component to attach to; the holder's root component is used when null.
 * @param AttachSocket Socket on the attachment parent.
 * @return true if the item enters the held state successfully, false otherwise.
 */
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

/**
 * @brief Returns a held item to the world at the specified transform.
 *
 * Revokes the item's granted abilities, clears its holder and ownership, and
 * optionally applies an impulse when physics simulation is active.
 *
 * @param DropTransform World transform to apply after detaching the item.
 * @param DropImpulse Impulse to apply to the item, if nonzero.
 * @return true if the item was successfully returned to the world, false otherwise.
 */
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

/**
 * @brief Activates the item's use ability while the item is held and usable.
 *
 * @return `true` if the held item is usable and its ability activates successfully, `false` otherwise.
 */
bool AKCWorldItemActor::ActivateUse()
{
	return RuntimeState.State == EKCWorldItemState::Held &&
		IsUsable() &&
		AbilitySourceComponent->TryActivate();
}

/**
 * @brief Activates the held item's use ability against a target actor.
 *
 * @param TargetActor Actor targeted by the item ability.
 * @return true if the ability activates successfully, false otherwise.
 */
bool AKCWorldItemActor::ActivateUseWithTarget(AActor* TargetActor)
{
	return HasAuthority() &&
		RuntimeState.State == EKCWorldItemState::Held &&
		IsUsable() &&
		AbilitySourceComponent->TryActivateWithTarget(TargetActor);
}

/**
 * @brief Determines whether the item can be picked up.
 *
 * @return `true` if the item is in the world with a valid definition, `false` otherwise.
 */
bool AKCWorldItemActor::CanBePickedUp() const
{
	return RuntimeState.State == EKCWorldItemState::World &&
		bDefinitionValid;
}

/**
 * @brief Determines whether the item can be used.
 *
 * @return `true` if the item definition is valid and usable, `false` otherwise.
 */
bool AKCWorldItemActor::IsUsable() const
{
	return bDefinitionValid && ItemDefinition->IsUsable();
}

/**
 * @brief Gets the item's current state.
 *
 * @return EKCWorldItemState Current item state.
 */
EKCWorldItemState AKCWorldItemActor::GetItemState() const
{
	return RuntimeState.State;
}

/**
 * @brief Gets the actor currently holding the item.
 *
 * @return AActor* The holding actor, or `nullptr` when the item is not held.
 */
AActor* AKCWorldItemActor::GetHolder() const
{
	return RuntimeState.Holder;
}

/**
 * @brief Gets the item's definition.
 *
 * @return UKCItemDefinition* The item's current definition.
 */
UKCItemDefinition* AKCWorldItemActor::GetItemDefinition() const
{
	return ItemDefinition;
}

/**
 * @brief Gets the component that renders the world item.
 *
 * @return UStaticMeshComponent* The item's static mesh component.
 */
UStaticMeshComponent* AKCWorldItemActor::GetItemMesh() const
{
	return ItemMesh;
}

/**
 * @brief Registers the actor properties replicated to clients.
 *
 * @param OutLifetimeProps Collection to which replicated properties are added.
 */
void AKCWorldItemActor::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AKCWorldItemActor, RuntimeState);
	DOREPLIFETIME(AKCWorldItemActor, ItemDefinition);
}

/**
 * @brief Updates the item's presentation, attachment, and state notifications after replicated state changes.
 */
void AKCWorldItemActor::OnRep_RuntimeState()
{
	ApplyStatePresentation();
	RefreshReplicatedAttachment();
	BroadcastStateChanged();
}

/**
 * @brief Refreshes the item definition and reapplies its replicated presentation and attachment.
 */
void AKCWorldItemActor::OnRep_ItemDefinition()
{
	RefreshDefinition();
	ApplyStatePresentation();
	RefreshReplicatedAttachment();
}

/**
 * @brief Refreshes the item's presentation and ability configuration from its definition.
 *
 * @param OutError Receives a description of the failure, or is cleared on success. May be null.
 * @return true if the item definition is valid and its ability configuration is applied, false otherwise.
 */
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

/**
 * @brief Aligns the item to the grip transform defined by its item definition.
 *
 * @return true if the grip alignment was applied, false if the required item data or alignment transform is unavailable.
 */
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

/**
 * @brief Restores the client's attachment to the replicated holder and hand socket.
 *
 * Detaches the item when it is in the world and retries attachment when the holder,
 * attachment component, or socket is not yet available.
 */
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

/**
 * @brief Applies collision and physics settings for the item's current definition and world state.
 */
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

/**
 * @brief Notifies listeners of the item's current state and holder.
 */
void AKCWorldItemActor::BroadcastStateChanged()
{
	OnItemStateChanged.Broadcast(RuntimeState.State, RuntimeState.Holder);
}
