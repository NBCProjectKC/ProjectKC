#include "ProjectKC/AbilitySystem/Targeting/KCEventTargeting.h"

#include "GameFramework/Actor.h"

void UKCEventTargeting::GatherTargets(
	const FKCActionTargetingContext& Context,
	TArray<FKCActionTarget>& OutTargets) const
{
	// 타이밍 대기 중 대상이 파괴됐을 수 있다.
	if (!IsValid(Context.ActivationTarget))
	{
		return;
	}

	FKCActionTarget& Target = OutTargets.AddDefaulted_GetRef();
	Target.Actor = Context.ActivationTarget;
	Target.HitResult = Context.ActivationHitResult;
	Target.bHasHitResult = Context.bHasActivationHitResult;
}
