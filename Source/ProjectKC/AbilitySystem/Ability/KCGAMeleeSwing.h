#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/AbilitySystem/Ability/KCGAUseActionBase.h"
#include "KCGAMeleeSwing.generated.h"

class AActor;
class UKCMeleeActionConfig;

/** 서버에서 전방 근접 Sweep을 수행하고 각 대상의 Target.OnHit Hook을 실행한다. */
UCLASS(Blueprintable)
class /**
 * Performs a forward melee sweep and invokes hit responses for affected targets.
 */
PROJECTKC_API UKCGAMeleeSwing : public UKCGAUseActionBase
{
	GENERATED_BODY()

public:
	UKCGAMeleeSwing();

protected:
	virtual bool PrepareUseAction(
		const FGameplayEventData* TriggerEventData) override;
	virtual bool ExecuteUseAction() override;

private:
	void GatherHitResults(
		const UKCMeleeActionConfig& Config,
		AActor& SourceActor,
		TArray<FHitResult>& OutHits) const;

	bool IsPathUnobstructed(
		const UKCMeleeActionConfig& Config,
		const AActor& SourceActor,
		const AActor& TargetActor) const;
};
