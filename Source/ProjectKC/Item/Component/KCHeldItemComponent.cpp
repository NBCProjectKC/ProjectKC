#include "ProjectKC/Item/Component/KCHeldItemComponent.h"

#include "CollisionQueryParams.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "ProjectKC/Item/KCWorldItemActor.h"

DEFINE_LOG_CATEGORY_STATIC(LogKCHeldItemComponent, Log, All);

UKCHeldItemComponent::UKCHeldItemComponent()
{
	SetIsReplicatedByDefault(true);
}

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

void UKCHeldItemComponent::TryInteract()
{
	AActor* Holder = GetOwner();
	if (!Holder)
	{
		return;
	}

	if (Holder->HasAuthority())
	{
		TryInteractAuthority();
	}
	else
	{
		ServerTryInteract();
	}
}

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

bool UKCHeldItemComponent::TryPickUp(AKCWorldItemActor* Item)
{
	AActor* Holder = GetOwner();
	if (!Holder || !Holder->HasAuthority() || IsValid(HeldItem) ||
		!IsValid(Item) || !Item->CanBePickedUp() ||
		InteractionRadius <= 0.0f ||
		FVector::DistSquared(Holder->GetActorLocation(), Item->GetActorLocation()) >
			FMath::Square(InteractionRadius))
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

bool UKCHeldItemComponent::UseHeldItem()
{
	return IsValid(HeldItem) && HeldItem->ActivateUse();
}

bool UKCHeldItemComponent::UseHeldItemWithTarget(AActor* TargetActor)
{
	return GetOwner() && GetOwner()->HasAuthority() &&
		IsValid(HeldItem) &&
		HeldItem->ActivateUseWithTarget(TargetActor);
}

AKCWorldItemActor* UKCHeldItemComponent::GetHeldItem() const
{
	return HeldItem;
}

bool UKCHeldItemComponent::HasHeldItem() const
{
	return IsValid(HeldItem);
}

USceneComponent* UKCHeldItemComponent::GetAttachmentComponent() const
{
	return ResolveAttachmentComponent();
}

FName UKCHeldItemComponent::GetHandSocketName() const
{
	return HandSocketName;
}

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

void UKCHeldItemComponent::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UKCHeldItemComponent, HeldItem);
}

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

void UKCHeldItemComponent::ServerTryInteract_Implementation()
{
	TryInteractAuthority();
}

void UKCHeldItemComponent::ServerDropHeldItem_Implementation()
{
	DropHeldItemAuthority();
}

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

void UKCHeldItemComponent::TryInteractAuthority()
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || HasHeldItem())
	{
		return;
	}

	if (AKCWorldItemActor* Candidate = FindBestPickupCandidate())
	{
		TryPickUp(Candidate);
	}
}

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

AKCWorldItemActor* UKCHeldItemComponent::FindBestPickupCandidate() const
{
	const AActor* Holder = GetOwner();
	UWorld* World = GetWorld();
	if (!Holder || !World || InteractionRadius <= 0.0f)
	{
		return nullptr;
	}

	FCollisionObjectQueryParams ObjectQuery;
	ObjectQuery.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQuery.AddObjectTypesToQuery(ECC_PhysicsBody);

	FCollisionQueryParams QueryParams(
		SCENE_QUERY_STAT(KCItemPickup),
		false,
		Holder);
	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(
		Overlaps,
		Holder->GetActorLocation(),
		FQuat::Identity,
		ObjectQuery,
		FCollisionShape::MakeSphere(InteractionRadius),
		QueryParams);

	AKCWorldItemActor* BestItem = nullptr;
	float BestDistanceSquared = TNumericLimits<float>::Max();
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AKCWorldItemActor* Item = Cast<AKCWorldItemActor>(Overlap.GetActor());
		if (!Item || !Item->CanBePickedUp())
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared(
			Holder->GetActorLocation(),
			Item->GetActorLocation());
		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestItem = Item;
		}
	}

	return BestItem;
}

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

void UKCHeldItemComponent::BroadcastHeldItemChanged()
{
	OnHeldItemChanged.Broadcast(HeldItem);
}
