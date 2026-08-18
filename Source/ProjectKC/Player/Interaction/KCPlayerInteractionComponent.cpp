#include "Player/Interaction/KCPlayerInteractionComponent.h"

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

	TArray<AActor*> OverlappingActors;
	GetOverlappingActors(OverlappingActors);
	AActor* BestTargetActor = nullptr;
	float BestDistanceSquared = TNumericLimits<float>::Max();

	for (AActor* CandidateActor : OverlappingActors)
	{
		if (!IsValidInteractionTarget(CandidateActor, bRequireLineOfSight))
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared2D(
			OwnerActor->GetActorLocation(), CandidateActor->GetActorLocation());
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
	if (!IsValidInteractionTarget(TargetActor, bRequireLineOfSight))
	{
		return;
	}

	IKCInteractableInterface::Execute_Interact(TargetActor, GetOwner());
}

bool UKCPlayerInteractionComponent::IsValidInteractionTarget(
	AActor* TargetActor,
	const bool bCheckLineOfSight) const
{
	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !IsValid(TargetActor) || TargetActor == OwnerActor
		|| !TargetActor->GetClass()->ImplementsInterface(UKCInteractableInterface::StaticClass()))
	{
		return false;
	}

	const FVector OwnerLocation = OwnerActor->GetActorLocation();
	const FVector TargetLocation = TargetActor->GetActorLocation();
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

	if (!IKCInteractableInterface::Execute_CanInteract(TargetActor, GetOwner()))
	{
		return false;
	}

	return !bCheckLineOfSight || HasLineOfSightTo(TargetActor);
}

bool UKCPlayerInteractionComponent::HasLineOfSightTo(const AActor* TargetActor) const
{
	const AActor* OwnerActor = GetOwner();
	UWorld* World = GetWorld();
	if (!OwnerActor || !TargetActor || !World)
	{
		return false;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PlayerInteractionLineTrace), false, OwnerActor);
	FHitResult HitResult;
	const bool bBlockingHit = World->LineTraceSingleByChannel(
		HitResult,
		OwnerActor->GetActorLocation(),
		TargetActor->GetActorLocation(),
		InteractionTraceChannel,
		QueryParams);

	return !bBlockingHit || HitResult.GetActor() == TargetActor;
}
