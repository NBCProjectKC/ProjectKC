#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/AbilitySystem/Targeting/KCActionTargeting.h"
#include "KCSelfTargeting.generated.h"

/** 행동을 수행하는 Avatar 자신을 대상으로 삼는다. */
UCLASS(meta = (DisplayName = "Self Targeting"))
class PROJECTKC_API UKCSelfTargeting : public UKCInstantActionTargeting
{
	GENERATED_BODY()

public:
	virtual void GatherTargets(
		const FKCActionTargetingContext& Context,
		TArray<FKCActionTarget>& OutTargets) const override;
};
