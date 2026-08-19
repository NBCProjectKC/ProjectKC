#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "ProjectKC/AbilitySystem/Ability/KCGameplayAbility.h"
#include "ProjectKC/AbilitySystem/Struct/KCActionMontageSpec.h"
#include "KCGAUseActionBase.generated.h"

class UAnimMontage;

/**
 * 사용 행동 한 번의 수명주기를 Action Montage에 맞춰 관리하는 GA 기반 클래스다.
 * Montage가 있으면 Execute Notify 시점에, 없으면 활성화 즉시 결과를 실행한다.
 * 활성화 절차를 파생 GA마다 복사하지 않는 것이 이 클래스의 존재 이유다.
 */
UCLASS(Abstract)
class /**
 * Manages a use action lifecycle and coordinates its execution with an optional action montage.
 *
 * Derived abilities validate and prepare action data before implementing the action execution.
 */
PROJECTKC_API UKCGAUseActionBase : public UKCGameplayAbility
{
	GENERATED_BODY()

public:
	UKCGAUseActionBase();

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

	/** 실행 전에 필요한 데이터를 검증하고 보관한다. false면 사용을 취소한다. */
	virtual bool PrepareUseAction(const FGameplayEventData* TriggerEventData)
		PURE_VIRTUAL(UKCGAUseActionBase::PrepareUseAction, return false;);

	/** 실제 Action Hook을 실행한다. Ability 한 번당 최대 한 번만 호출된다. */
	virtual bool ExecuteUseAction()
		PURE_VIRTUAL(UKCGAUseActionBase::ExecuteUseAction, return false;);

private:
	UFUNCTION()
	void OnActionExecuteEventReceived(FGameplayEventData Payload);

	UFUNCTION()
	void OnActionMontageFinished();

	UFUNCTION()
	void OnActionMontageAborted();

	bool StartActionMontage(const FKCActionMontageSpec& MontageSpec);
	void RunUseActionOnce();
	void FinishUseAction(bool bWasCancelled);

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> RemoteOwnerMontage;

	bool bStopRemoteOwnerMontageOnEnd = false;
	bool bUseActionAttempted = false;
	bool bUseActionSucceeded = false;
	bool bFinishingUseAction = false;
};
