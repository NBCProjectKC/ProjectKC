#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/AbilitySystem/Ability/KCGA_Base.h"
#include "KCGA_ActionRuntimeBase.generated.h"

class UKCAbilityTask_ActionTraceWindow;
class AKCWorldItemActor;
enum class EKCItemDurabilityConsumeMode : uint8;
struct FKCActionTarget;
struct FKCActionTargetingContext;
struct FKCLoopingCueStruct;

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
	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

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

	/** 충전 입력처럼 Press에서는 활성 상태만 만들고 실제 Commit을 미룰 때 사용한다. */
	virtual bool ShouldDeferActionExecutionStart() const;
	bool BeginActionExecution();
	bool HasActionExecutionStarted() const;

	/** 대상이 없는 시점의 Hook을 소스 자신에게 한 번 실행한다. */
	void ExecuteSourceHook(FGameplayTag HookTag);

	void FinishAction(bool bWasCancelled, bool bRunCompleteHook);
	bool IsFinishingAction() const;

	/**
	 * 실행 구간 동안 유지할 Looping Cue 설정이다. 쓰지 않는 수명주기는 nullptr를 준다.
	 * 설정은 Definition이 소유하지만 Cue의 수명은 이 GA가 쥔다.
	 */
	virtual const FKCLoopingCueStruct* GetLoopingCueConfig() const;

private:
	friend class UKCAbilityTask_ActionTraceWindow;

	FKCActionTargetingContext BuildTargetingContext() const;
	bool ExecuteTargets(const TArray<FKCActionTarget>& Targets);
	AKCWorldItemActor* ResolveSourceItem(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo) const;
	bool TryBeginActiveItemUseConsumption();
	bool TryConsumeActiveItemDurability(
		EKCItemDurabilityConsumeMode ConsumeMode,
		float ConsumptionScale = 1.0f);
	void StartLoopingCue();
	void StopLoopingCue();
	void StartActiveDurabilityDrain();
	void StopActiveDurabilityDrain(bool bConsumeRemainingTime);
	void TickActiveDurabilityDrain();

	TWeakObjectPtr<AActor> ActivationTarget;
	FHitResult ActivationHitResult;
	bool bHasActivationHitResult = false;
	bool bFinishingAction = false;
	bool bActionExecutionStarted = false;
	bool bDurabilityConsumedThisActivation = false;
	bool bUseConsumptionPendingThisActivation = false;
	bool bDurabilityDrainActive = false;
	/** 현재 붙어 있는 Looping Cue다. 비어 있으면 붙은 Cue가 없다. */
	FGameplayTag ActiveLoopingCueTag;
	double LastDurabilityDrainTimeSeconds = 0.0;
	FTimerHandle DurabilityDrainTimerHandle;
	TWeakObjectPtr<AKCWorldItemActor> ActiveSourceItem;

	UPROPERTY(Transient)
	TObjectPtr<UKCAbilityTask_ActionTraceWindow> ActiveTraceTask;
};
