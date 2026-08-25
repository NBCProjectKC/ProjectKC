#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "ProjectKC/AbilitySystem/Ability/KCGA_Base.h"
#include "KCGA_Action.generated.h"

class UKCActionTiming;

/**
 * Definition이 조립한 축을 그대로 실행하는 단일 Action GA다.
 *
 *   누구를  ActionTargeting  대상 수집
 *   언제    ActionTiming     실행 시점 (비면 즉시)
 *   무엇을  ActionHooks      시점별 Fragment
 *
 * 아이템·함정·AI·캐릭터 내재 능력이 모두 이 경로를 공유하고,
 * 차이는 GA 클래스가 아니라 데이터 조합으로만 표현한다.
 */
UCLASS(Blueprintable, meta = (DisplayName = "KCGA_Action"))
class PROJECTKC_API UKCGA_Action : public UKCGA_Base
{
	GENERATED_BODY()

public:
	UKCGA_Action();

	/** ActionTiming이 실행 시점을 알릴 때 호출한다. */
	UFUNCTION()
	void HandleTimingExecuteEvent(FGameplayEventData Payload);

	UFUNCTION()
	void HandleTimingCompleted();

	UFUNCTION()
	void HandleTimingAborted();

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

private:
	void RunExecuteOnce();
	void FinishAction(bool bWasCancelled);

	/** 대상이 없는 시점의 Hook이다. 소스를 대상으로 삼아 한 번만 실행한다. */
	void ExecuteSourceHook(FGameplayTag HookTag);

	TWeakObjectPtr<const UKCActionTiming> ActiveTiming;

	/** 활성화 이벤트가 넘겨준 대상. 타이밍 대기 중 파괴될 수 있어 약참조다. */
	TWeakObjectPtr<AActor> ActivationTarget;
	FHitResult ActivationHitResult;
	bool bHasActivationHitResult = false;

	bool bExecuteAttempted = false;
	bool bFinishingAction = false;
};
