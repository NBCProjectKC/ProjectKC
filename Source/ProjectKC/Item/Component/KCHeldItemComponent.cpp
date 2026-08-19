#include "ProjectKC/Item/Component/KCHeldItemComponent.h"

#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "ProjectKC/Item/KCWorldItemActor.h"

DEFINE_LOG_CATEGORY_STATIC(LogKCHeldItemComponent, Log, All);

/**
 * @brief Initializes the component with replication enabled by default.
 */
UKCHeldItemComponent::UKCHeldItemComponent()
{
	SetIsReplicatedByDefault(true);
}

/**
 * @brief Configures the component and socket used to attach the held item.
 *
 * @param NewAttachmentComponent Component that owns the attachment socket.
 * @param NewHandSocketName Socket name used for attachment.
 * @return true if the attachment configuration is accepted, false otherwise.
 */
bool UKCHeldItemComponent::ConfigureAttachment(
	USceneComponent* NewAttachmentComponent,
	FName NewHandSocketName)
{
	if (IsValid(HeldItem) || !IsValid(NewAttachmentComponent) ||
		NewAttachmentComponent->GetOwner() != GetOwner() ||
		NewHandSocketName.IsNone() ||
		!NewAttachmentComponent->DoesSocketExist(NewHandSocketName))
	{
		return false;
	}

	RuntimeAttachmentComponent = NewAttachmentComponent;
	HandSocketName = NewHandSocketName;
	return true;
}

/**
 * @brief Requests that the currently held item be dropped.
 *
 * Executes the drop on authority or sends a server request when called on a client.
 */
void UKCHeldItemComponent::TryDropHeldItem()
{
	AActor* Holder = GetOwner();
	if (!Holder)
	{
		return;
	}

	if (Holder->HasAuthority())
	{
		DropHeldItemAuthority();
	}
	else
	{
		ServerDropHeldItem();
	}
}

/**
 * @brief Attempts to pick up an eligible item within the configured distance.
 *
 * @param Item Item to pick up.
 * @return true if the item enters the held state, false otherwise.
 */
bool UKCHeldItemComponent::TryPickUp(AKCWorldItemActor* Item)
{
	AActor* Holder = GetOwner();
	if (!Holder || !Holder->HasAuthority() || IsValid(HeldItem) ||
		!IsValid(Item) || !Item->CanBePickedUp() ||
		MaxPickupDistance <= 0.0f ||
		FVector::DistSquared(Holder->GetActorLocation(), Item->GetActorLocation()) >
			FMath::Square(MaxPickupDistance))
	{
		return false;
	}

	USceneComponent* AttachComponent = ResolveAttachmentComponent();
	if (!AttachComponent || HandSocketName.IsNone() ||
		!AttachComponent->DoesSocketExist(HandSocketName))
	{
		return false;
	}

	if (!Item->EnterHeldState(Holder, AttachComponent, HandSocketName))
	{
		return false;
	}

	HeldItem = Item;
	Holder->ForceNetUpdate();
	BroadcastHeldItemChanged();
	return true;
}

/**
 * @brief Drops the currently held item using the specified transform and impulse.
 *
 * @param DropTransform Transform to apply to the dropped item.
 * @param DropImpulse Impulse to apply when dropping the item.
 * @return true if the item was successfully dropped, false otherwise.
 */
bool UKCHeldItemComponent::DropHeldItem(
	const FTransform& DropTransform,
	FVector DropImpulse)
{
	AActor* Holder = GetOwner();
	if (!Holder || !Holder->HasAuthority() || !IsValid(HeldItem))
	{
		return false;
	}

	if (!HeldItem->ExitHeldState(DropTransform, DropImpulse))
	{
		return false;
	}

	HeldItem = nullptr;
	Holder->ForceNetUpdate();
	BroadcastHeldItemChanged();
	return true;
}

/**
 * @brief Activates the currently held item.
 *
 * @return `true` if a valid held item is activated, `false` otherwise.
 */
bool UKCHeldItemComponent::UseHeldItem()
{
	return IsValid(HeldItem) && HeldItem->ActivateUse();
}

/**
 * @brief Activates the held item against a target actor when called by the authority.
 *
 * @param TargetActor Actor to use the held item against.
 * @return true if the held item was activated successfully, false otherwise.
 */
bool UKCHeldItemComponent::UseHeldItemWithTarget(AActor* TargetActor)
{
	return GetOwner() && GetOwner()->HasAuthority() &&
		IsValid(HeldItem) &&
		HeldItem->ActivateUseWithTarget(TargetActor);
}

/**
 * @brief Gets the item currently held by the owner.
 *
 * @return AKCWorldItemActor* The held item, or `nullptr` if no item is held.
 */
AKCWorldItemActor* UKCHeldItemComponent::GetHeldItem() const
{
	return HeldItem;
}

/**
 * @brief Determines whether this component currently holds an item.
 *
 * @return `true` if a valid item is held, `false` otherwise.
 */
bool UKCHeldItemComponent::HasHeldItem() const
{
	return IsValid(HeldItem);
}

/**
 * @brief Retrieves the component containing the configured attachment socket.
 *
 * @return USceneComponent* The resolved attachment component, or `nullptr` if no suitable component is found.
 */
USceneComponent* UKCHeldItemComponent::GetAttachmentComponent() const
{
	return ResolveAttachmentComponent();
}

/**
 * @brief Gets the configured hand socket name.
 *
 * @return FName The name of the socket used to attach the held item.
 */
FName UKCHeldItemComponent::GetHandSocketName() const
{
	return HandSocketName;
}

/**
 * @brief Initializes the runtime attachment component and verifies the configured hand socket.
 *
 * Logs a warning when no valid attachment component provides the configured hand socket.
 */
void UKCHeldItemComponent::BeginPlay()
{
	Super::BeginPlay();

	RuntimeAttachmentComponent = ResolveAttachmentComponent();
	if (!RuntimeAttachmentComponent || HandSocketName.IsNone() ||
		!RuntimeAttachmentComponent->DoesSocketExist(HandSocketName))
	{
		UE_LOG(
			LogKCHeldItemComponent,
			Warning,
			TEXT("Holder '%s'에서 Hand 소켓 '%s'를 제공하는 Attachment Component를 찾지 못했습니다."),
			*GetNameSafe(GetOwner()),
			*HandSocketName.ToString());
	}
}

/**
 * @brief Releases the held item when the component is ending play on authority.
 *
 * @param EndPlayReason Reason the component is ending play.
 */
void UKCHeldItemComponent::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	if (GetOwner() && GetOwner()->HasAuthority() && IsValid(HeldItem))
	{
		HeldItem->ExitHeldState(HeldItem->GetActorTransform(), FVector::ZeroVector);
		HeldItem = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

/**
 * @brief Registers the held item property for network replication.
 *
 * @param OutLifetimeProps Collection of properties to replicate.
 */
void UKCHeldItemComponent::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UKCHeldItemComponent, HeldItem);
}

/**
 * @brief Synchronizes the replicated held item's attachment and notifies listeners of the held-item change.
 */
void UKCHeldItemComponent::OnRep_HeldItem()
{
	if (IsValid(HeldItem))
	{
		// RuntimeState와 HeldItem은 서로 다른 Actor/Component에서 복제되므로
		// 어느 쪽 RepNotify가 먼저 와도 클라이언트 부착을 다시 맞춘다.
		HeldItem->RefreshReplicatedAttachment();
	}
	BroadcastHeldItemChanged();
}

void UKCHeldItemComponent::ServerDropHeldItem_Implementation()
{
	DropHeldItemAuthority();
}

/**
 * @brief Resolves the scene component that provides the configured hand socket.
 *
 * @return USceneComponent* A valid attachment component containing the hand socket, or nullptr if none is found.
 */
USceneComponent* UKCHeldItemComponent::ResolveAttachmentComponent() const
{
	if (IsValid(RuntimeAttachmentComponent))
	{
		return RuntimeAttachmentComponent;
	}

	AActor* Holder = GetOwner();
	if (!Holder)
	{
		return nullptr;
	}

	if (USceneComponent* ExplicitComponent =
		Cast<USceneComponent>(AttachmentComponent.GetComponent(Holder)))
	{
		// 비어 있는 FComponentReference는 Owner의 RootComponent를 반환할 수 있다.
		// 실제 Hand 소켓을 제공할 때만 명시적 설정으로 채택한다.
		if (ExplicitComponent->DoesSocketExist(HandSocketName))
		{
			return ExplicitComponent;
		}
	}

	// 별도 Configure 호출 없이 컴포넌트만 붙여도 동작하도록, 같은 이름의
	// 소켓을 가진 SkeletalMesh를 우선 탐색한다.
	TInlineComponentArray<USkeletalMeshComponent*> SkeletalMeshes(Holder);
	for (USkeletalMeshComponent* SkeletalMesh : SkeletalMeshes)
	{
		if (IsValid(SkeletalMesh) && SkeletalMesh->DoesSocketExist(HandSocketName))
		{
			return SkeletalMesh;
		}
	}

	TInlineComponentArray<USceneComponent*> SceneComponents(Holder);
	for (USceneComponent* SceneComponent : SceneComponents)
	{
		if (IsValid(SceneComponent) &&
			SceneComponent->DoesSocketExist(HandSocketName))
		{
			return SceneComponent;
		}
	}

	return nullptr;
}

/**
 * @brief Drops the held item from the authoritative owner using the configured forward impulse.
 */
void UKCHeldItemComponent::DropHeldItemAuthority()
{
	AActor* Holder = GetOwner();
	if (!Holder || !Holder->HasAuthority() || !HasHeldItem())
	{
		return;
	}

	DropHeldItem(
		MakeHeldItemDropTransform(),
		Holder->GetActorForwardVector() * DropForwardImpulse);
}

/**
 * @brief Calculates the transform used to place a dropped held item.
 *
 * @return FTransform A transform positioned ahead of and above the owning actor with its rotation, or the identity transform when no owner exists.
 */
FTransform UKCHeldItemComponent::MakeHeldItemDropTransform() const
{
	const AActor* Holder = GetOwner();
	if (!Holder)
	{
		return FTransform::Identity;
	}

	const FVector DropLocation = Holder->GetActorLocation() +
		Holder->GetActorForwardVector() * DropForwardDistance +
		FVector::UpVector * DropHeightOffset;
	return FTransform(Holder->GetActorRotation(), DropLocation);
}

/**
 * @brief Broadcasts the current held item through the held-item change event.
 */
void UKCHeldItemComponent::BroadcastHeldItemChanged()
{
	OnHeldItemChanged.Broadcast(HeldItem);
}
