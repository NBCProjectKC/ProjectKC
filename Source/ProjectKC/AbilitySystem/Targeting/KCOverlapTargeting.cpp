#include "ProjectKC/AbilitySystem/Targeting/KCOverlapTargeting.h"

#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"

DEFINE_LOG_CATEGORY_STATIC(LogKCOverlapTargeting, Log, All);

UKCOverlapTargeting::UKCOverlapTargeting()
{
	TargetActorClass = APawn::StaticClass();
}

bool UKCOverlapTargeting::Validate(FString& OutError) const
{
	OutError.Reset();
	if (MaxTargets < 1 || MaxTargets > 32)
	{
		OutError = TEXT("MaxTargets는 1 이상 32 이하여야 합니다.");
		return false;
	}

	return true;
}

UPrimitiveComponent* UKCOverlapTargeting::ResolveOverlapComponent(
	AActor& SourceActor) const
{
	if (UPrimitiveComponent* Referenced =
		Cast<UPrimitiveComponent>(OverlapComponent.GetComponent(&SourceActor)))
	{
		return Referenced;
	}

	return Cast<UPrimitiveComponent>(SourceActor.GetRootComponent());
}

void UKCOverlapTargeting::GatherTargets(
	const FKCActionTargetingContext& Context,
	TArray<FKCActionTarget>& OutTargets) const
{
	AActor* SourceActor = Context.SourceActor;
	if (!IsValid(SourceActor))
	{
		return;
	}

	UPrimitiveComponent* Volume = ResolveOverlapComponent(*SourceActor);
	if (!Volume)
	{
		UE_LOG(
			LogKCOverlapTargeting,
			Warning,
			TEXT("소스 '%s'에서 판정에 쓸 Primitive 볼륨을 찾지 못했습니다."),
			*SourceActor->GetName());
		return;
	}

	TArray<AActor*> OverlappingActors;
	Volume->GetOverlappingActors(OverlappingActors, TargetActorClass);

	for (AActor* Candidate : OverlappingActors)
	{
		if (OutTargets.Num() >= MaxTargets)
		{
			break;
		}

		// 소스 자신과 소스에 딸린 Actor는 대상이 아니다.
		if (!IsValid(Candidate) || Candidate == SourceActor ||
			Candidate->IsOwnedBy(SourceActor) ||
			Candidate->IsAttachedTo(SourceActor))
		{
			continue;
		}

		// 겹침만으로는 캡슐 크기만큼 넓어지므로 중심이 볼륨 안인지 확인한다.
		FVector ClosestPoint = FVector::ZeroVector;
		const float Distance = Volume->GetDistanceToCollision(
			Candidate->GetActorLocation(),
			ClosestPoint);
		if (Distance < 0.0f)
		{
			// 판정할 수 없는 지오메트리다. 겹침 결과를 그대로 신뢰한다.
			UE_LOG(
				LogKCOverlapTargeting,
				Verbose,
				TEXT("볼륨 '%s'에서 중심 포함을 판정할 수 없어 겹침 결과를 사용합니다."),
				*Volume->GetName());
		}
		else if (!FMath::IsNearlyZero(Distance))
		{
			continue;
		}

		FKCActionTarget& Target = OutTargets.AddDefaulted_GetRef();
		Target.Actor = Candidate;
	}
}
