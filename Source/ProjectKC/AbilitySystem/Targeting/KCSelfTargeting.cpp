#include "ProjectKC/AbilitySystem/Targeting/KCSelfTargeting.h"

#include "GameFramework/Actor.h"

void UKCSelfTargeting::GatherTargets(
	const FKCActionTargetingContext& Context,
	TArray<FKCActionTarget>& OutTargets) const
{
	if (!IsValid(Context.SourceActor))
	{
		return;
	}

	FKCActionTarget& Target = OutTargets.AddDefaulted_GetRef();
	Target.Actor = Context.SourceActor;
}
