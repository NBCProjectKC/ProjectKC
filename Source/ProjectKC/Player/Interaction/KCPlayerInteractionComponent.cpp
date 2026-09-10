#include "Player/Interaction/KCPlayerInteractionComponent.h"

#include "Camera/CameraComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Materials/MaterialInterface.h"
#include "Interaction/Interface/KCInteractableInterface.h"
#include "ProjectKC/Item/Component/KCHeldItemComponent.h"
#include "ProjectKC/Item/KCWorldItemActor.h"
#include "ProjectKC/Player/KCPlayerState.h"

UKCPlayerInteractionComponent::UKCPlayerInteractionComponent()
{
	SetIsReplicatedByDefault(true);
	InitSphereRadius(250.0f);
	SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);
	SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);
	SetGenerateOverlapEvents(true);
	SetCanEverAffectNavigation(false);
	PrimaryComponentTick.bCanEverTick = true;
}

void UKCPlayerInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
	RefreshBestInteractable();
}

void UKCPlayerInteractionComponent::TickComponent(
	const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn && OwnerPawn->IsLocallyControlled())
	{
		ApplyItemHighlightPostProcess();
		RefreshBestInteractable();
	}
}

void UKCPlayerInteractionComponent::TryInteract()
{
	RefreshBestInteractable();

	AActor* TargetActor = GetCurrentBestInteractable();
	if (!TargetActor)
	{
		return;
	}

	ServerTryInteract(TargetActor);
}

AActor* UKCPlayerInteractionComponent::GetBestInteractable() const
{
	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return nullptr;
	}

	TArray<UPrimitiveComponent*> OverlappingTargetComponents;
	GetOverlappingComponents(OverlappingTargetComponents);
	AActor* BestTargetActor = nullptr;
	float BestDistanceSquared = TNumericLimits<float>::Max();

	for (UPrimitiveComponent* CandidateComponent : OverlappingTargetComponents)
	{
		FVector ClosestInteractionPoint;
		if (!IsValidInteractionComponent(
			CandidateComponent,
			bRequireLineOfSight,
			&ClosestInteractionPoint))
		{
			continue;
		}

		AActor* CandidateActor = CandidateComponent->GetOwner();
		const float DistanceSquared = FVector::DistSquared2D(
			OwnerActor->GetActorLocation(), ClosestInteractionPoint);
		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestTargetActor = CandidateActor;
		}
	}

	return BestTargetActor;
}

AActor* UKCPlayerInteractionComponent::GetCurrentBestInteractable() const
{
	return CurrentBestInteractable.Get();
}

void UKCPlayerInteractionComponent::RefreshBestInteractable()
{
	AActor* NewBestInteractable = GetBestInteractable();
	if (CurrentBestInteractable.Get() == NewBestInteractable)
	{
		if (AKCWorldItemActor* CurrentItem =
			Cast<AKCWorldItemActor>(NewBestInteractable))
		{
			CurrentItem->ApplyInteractionOutlineForTeam(GetOwnerTeamId());
		}
		return;
	}

	AActor* PreviousBestInteractable = CurrentBestInteractable.Get();
	CurrentBestInteractable = NewBestInteractable;
	ApplyInteractableOutline(PreviousBestInteractable, NewBestInteractable);
	OnBestInteractableChanged.Broadcast(NewBestInteractable);
}

void UKCPlayerInteractionComponent::ServerTryInteract_Implementation(AActor* TargetActor)
{
	if (!FindInteractionComponent(TargetActor, bRequireLineOfSight))
	{
		return;
	}

	IKCInteractableInterface::Execute_Interact(TargetActor, GetOwner());
}

UPrimitiveComponent* UKCPlayerInteractionComponent::FindInteractionComponent(
	AActor* TargetActor,
	const bool bCheckLineOfSight) const
{
	if (!IsValid(TargetActor))
	{
		return nullptr;
	}

	TArray<UPrimitiveComponent*> OverlappingTargetComponents;
	GetOverlappingComponents(OverlappingTargetComponents);
	UPrimitiveComponent* BestTargetComponent = nullptr;
	float BestDistanceSquared = TNumericLimits<float>::Max();

	for (UPrimitiveComponent* CandidateComponent : OverlappingTargetComponents)
	{
		if (!CandidateComponent || CandidateComponent->GetOwner() != TargetActor
			|| !IsValidInteractionComponent(CandidateComponent, bCheckLineOfSight))
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared2D(
			GetOwner()->GetActorLocation(), CandidateComponent->Bounds.Origin);
		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestTargetComponent = CandidateComponent;
		}
	}

	return BestTargetComponent;
}

void UKCPlayerInteractionComponent::ApplyInteractableOutline(
	AActor* PreviousTarget,
	AActor* NewTarget) const
{
	if (AKCWorldItemActor* PreviousItem = Cast<AKCWorldItemActor>(PreviousTarget))
	{
		PreviousItem->ApplyDefaultOutline();
	}

	if (AKCWorldItemActor* NewItem = Cast<AKCWorldItemActor>(NewTarget))
	{
		NewItem->ApplyInteractionOutlineForTeam(GetOwnerTeamId());
	}
}

void UKCPlayerInteractionComponent::ApplyItemHighlightPostProcess()
{
	if (bItemHighlightPostProcessApplied || ItemHighlightPostProcessMaterial.IsNull())
	{
		return;
	}

	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn || !OwnerPawn->IsLocallyControlled())
	{
		return;
	}

	UCameraComponent* CameraComponent = OwnerPawn->FindComponentByClass<UCameraComponent>();
	if (!CameraComponent)
	{
		return;
	}

	UMaterialInterface* HighlightMaterial = ItemHighlightPostProcessMaterial.LoadSynchronous();
	if (!HighlightMaterial)
	{
		return;
	}

	CameraComponent->AddOrUpdateBlendable(
		HighlightMaterial,
		ItemHighlightPostProcessWeight);
	bItemHighlightPostProcessApplied = true;
}

int32 UKCPlayerInteractionComponent::GetOwnerTeamId() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	const AKCPlayerState* KCPlayerState = OwnerPawn
		? OwnerPawn->GetPlayerState<AKCPlayerState>()
		: nullptr;
	return KCPlayerState ? KCPlayerState->GetTeamId() : INDEX_NONE;
}

bool UKCPlayerInteractionComponent::IsValidInteractionComponent(
	UPrimitiveComponent* TargetComponent,
	const bool bCheckLineOfSight,
	FVector* OutClosestInteractionPoint) const
{
	const AActor* OwnerActor = GetOwner();
	AActor* TargetActor = TargetComponent ? TargetComponent->GetOwner() : nullptr;
	if (!OwnerActor || !IsValid(TargetComponent) || !IsValid(TargetActor)
		|| TargetActor == OwnerActor
		|| !TargetComponent->ComponentHasTag(InteractableComponentTag)
		|| !TargetActor->GetClass()->ImplementsInterface(UKCInteractableInterface::StaticClass()))
	{
		return false;
	}

	const FVector OwnerLocation = OwnerActor->GetActorLocation();
	FVector ClosestInteractionPoint;
	if (TargetComponent->GetClosestPointOnCollision(
		OwnerLocation,
		ClosestInteractionPoint) < 0.0f)
	{
		return false;
	}

	if (const AKCWorldItemActor* WorldItem = Cast<AKCWorldItemActor>(TargetActor))
	{
		const UKCHeldItemComponent* HeldItemComponent =
			OwnerActor->FindComponentByClass<UKCHeldItemComponent>();
		if (!HeldItemComponent || !HeldItemComponent->CanPickUpItem(WorldItem))
		{
			return false;
		}
	}

	const FVector ToTarget =
		(ClosestInteractionPoint - OwnerLocation).GetSafeNormal2D();
	if (!ToTarget.IsNearlyZero())
	{
		if (FVector::DistSquared2D(OwnerLocation, ClosestInteractionPoint)
			> FMath::Square(GetScaledSphereRadius()))
		{
			return false;
		}

		const FVector OwnerForward = OwnerActor->GetActorForwardVector().GetSafeNormal2D();
		if (FVector::DotProduct(OwnerForward, ToTarget) < MinimumForwardDot)
		{
			return false;
		}
	}

	if (bCheckLineOfSight &&
		!HasLineOfSightTo(TargetComponent, ClosestInteractionPoint))
	{
		return false;
	}

	if (OutClosestInteractionPoint)
	{
		*OutClosestInteractionPoint = ClosestInteractionPoint;
	}

	return true;
}

bool UKCPlayerInteractionComponent::HasLineOfSightTo(
	const UPrimitiveComponent* TargetComponent,
	const FVector& InteractionPoint) const
{
	const AActor* OwnerActor = GetOwner();
	const AActor* TargetActor = TargetComponent ? TargetComponent->GetOwner() : nullptr;
	UWorld* World = GetWorld();
	if (!OwnerActor || !TargetComponent || !TargetActor || !World)
	{
		return false;
	}

	FCollisionQueryParams QueryParams(
		SCENE_QUERY_STAT(PlayerInteractionLineTrace),
		false,
		OwnerActor);
	if (TargetActor->IsA<AKCWorldItemActor>())
	{
		TArray<AActor*> OverlappingWorldItems;
		GetOverlappingActors(
			OverlappingWorldItems,
			AKCWorldItemActor::StaticClass());
		for (AActor* OverlappingWorldItem : OverlappingWorldItems)
		{
			if (OverlappingWorldItem != TargetActor)
			{
				QueryParams.AddIgnoredActor(OverlappingWorldItem);
			}
		}
	}

	FHitResult HitResult;
	const bool bBlockingHit = World->LineTraceSingleByChannel(
		HitResult,
		OwnerActor->GetActorLocation(),
		InteractionPoint,
		InteractionTraceChannel,
		QueryParams);

	return !bBlockingHit || HitResult.GetActor() == TargetActor;
}
