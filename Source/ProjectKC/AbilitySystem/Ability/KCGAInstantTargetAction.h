#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/AbilitySystem/Ability/KCGAUseActionBase.h"
#include "KCGAInstantTargetAction.generated.h"

class AActor;

/** GameplayEvent의 Target을 실행 대상으로 삼아 Target.OnTrigger Fragment를 실행한다. */
UCLASS(Blueprintable)
class PROJECTKC_API UKCGAInstantTargetAction : public UKCGAUseActionBase
{
	GENERATED_BODY()

public:
	UKCGAInstantTargetAction();

protected:
	virtual bool PrepareUseAction(
		const FGameplayEventData* TriggerEventData) override;
	virtual bool ExecuteUseAction() override;

private:
	/** 몽타주 대기 중 대상이 파괴될 수 있으므로 약참조로 보관한다. */
	TWeakObjectPtr<AActor> PreparedTargetActor;

	FHitResult PreparedHitResult;
	bool bHasPreparedHitResult = false;
};
