#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/AbilitySystem/Ability/KCGA_Base.h"
#include "KCGA_ActionRuntimeBase.generated.h"

class UKCAbilityTask_ActionTraceWindow;
struct FKCActionTarget;
struct FKCActionTargetingContext;

/** Targeting, Hook 실행, 종료 정리를 공유하는 Action GA 런타임 기반이다. */
UCLASS(Abstract)
class PROJECTKC_API UKCGA_ActionRuntimeBase : public UKCGA_Base
{
	GENERATED_BODY()

public:
	UKCGA_ActionRuntimeBase();

	/** 현재 몽타주의 Socket Trace NotifyState가 호출하는 정확한 Ability 진입점이다. */
	void NotifySocketTraceWindowBegin();
	void NotifySocketTraceWindowTick();
	void NotifySocketTraceWindowEnd();

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

	/** Single/Channel이 각자의 규칙으로 새 실행 구간을 받을지 결정한다. */
	virtual bool TryBeginExecutionWindow() PURE_VIRTUAL(
		UKCGA_ActionRuntimeBase::TryBeginExecutionWindow,
		return false;);

	/** 대상이 없는 시점의 Hook을 소스 자신에게 한 번 실행한다. */
	void ExecuteSourceHook(FGameplayTag HookTag);

	void FinishAction(bool bWasCancelled, bool bRunCompleteHook);
	bool IsFinishingAction() const;

private:
	friend class UKCAbilityTask_ActionTraceWindow;

	FKCActionTargetingContext BuildTargetingContext() const;
	void ExecuteTargets(const TArray<FKCActionTarget>& Targets);

	TWeakObjectPtr<AActor> ActivationTarget;
	FHitResult ActivationHitResult;
	bool bHasActivationHitResult = false;
	bool bFinishingAction = false;

	UPROPERTY(Transient)
	TObjectPtr<UKCAbilityTask_ActionTraceWindow> ActiveTraceTask;
};
