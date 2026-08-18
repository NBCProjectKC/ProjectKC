#include "Player/Interaction/KCPlayerInteractionComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interface/KCInteractableInterface.h"

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
}

void UKCPlayerInteractionComponent::TryInteract()
{
	AActor* TargetActor = GetBestInteractable();
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
		if (!IsValidInteractionComponent(CandidateComponent, bRequireLineOfSight))
		{
			continue;
		}

		AActor* CandidateActor = CandidateComponent->GetOwner();
		const float DistanceSquared = FVector::DistSquared2D(
			OwnerActor->GetActorLocation(), CandidateComponent->Bounds.Origin);
		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestTargetActor = CandidateActor;
		}
	}

	return BestTargetActor;
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

bool UKCPlayerInteractionComponent::IsValidInteractionComponent(
	UPrimitiveComponent* TargetComponent,
	const bool bCheckLineOfSight) const
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
	const FVector TargetLocation = TargetComponent->Bounds.Origin;
	const FVector ToTarget = (TargetLocation - OwnerLocation).GetSafeNormal2D();
	if (ToTarget.IsNearlyZero())
	{
		return false;
	}

	if (FVector::DistSquared2D(OwnerLocation, TargetLocation)
		> FMath::Square(GetScaledSphereRadius()))
	{
		return false;
	}

	const FVector OwnerForward = OwnerActor->GetActorForwardVector().GetSafeNormal2D();
	if (FVector::DotProduct(OwnerForward, ToTarget) < MinimumForwardDot)
	{
		return false;
	}

	return !bCheckLineOfSight || HasLineOfSightTo(TargetComponent);
}

bool UKCPlayerInteractionComponent::HasLineOfSightTo(
	const UPrimitiveComponent* TargetComponent) const
{
	const AActor* OwnerActor = GetOwner();
	const AActor* TargetActor = TargetComponent ? TargetComponent->GetOwner() : nullptr;
	UWorld* World = GetWorld();
	if (!OwnerActor || !TargetComponent || !TargetActor || !World)
	{
		return false;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PlayerInteractionLineTrace), false, OwnerActor);
	FHitResult HitResult;
	const bool bBlockingHit = World->LineTraceSingleByChannel(
		HitResult,
		OwnerActor->GetActorLocation(),
		TargetComponent->Bounds.Origin,
		InteractionTraceChannel,
		QueryParams);

	return !bBlockingHit || HitResult.GetActor() == TargetActor;
}
