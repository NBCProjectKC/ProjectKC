#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/AbilitySystem/Ability/KCGA_Base.h"
#include "KCGA_ActionRuntimeBase.generated.h"

/** Targeting, Hook 실행, 종료 정리를 공유하는 Action GA 런타임 기반이다. */
UCLASS(Abstract)
class PROJECTKC_API UKCGA_ActionRuntimeBase : public UKCGA_Base
{
	GENERATED_BODY()

public:
	UKCGA_ActionRuntimeBase();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	/** 현재 ActionTargeting으로 대상을 다시 수집해 OnExecute를 한 번 실행한다. */
	void ExecutePulse();

	/** 대상이 없는 시점의 Hook을 소스 자신에게 한 번 실행한다. */
	void ExecuteSourceHook(FGameplayTag HookTag);

	void FinishAction(bool bWasCancelled, bool bRunCompleteHook);
	bool IsFinishingAction() const;

private:
	TWeakObjectPtr<AActor> ActivationTarget;
	FHitResult ActivationHitResult;
	bool bHasActivationHitResult = false;
	bool bFinishingAction = false;
};
