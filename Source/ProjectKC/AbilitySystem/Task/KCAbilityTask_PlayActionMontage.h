#pragma once

#include "Abilities/GameplayAbilityTypes.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "CoreMinimal.h"
#include "ProjectKC/AbilitySystem/Struct/KCActionMontageConfigStruct.h"
#include "KCAbilityTask_PlayActionMontage.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FKCActionMontageExecuteDelegate,
	FGameplayEventData,
	Payload);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FKCActionMontageTerminalDelegate);

/**
 * Action 몽타주 재생과 선택적인 Execute Event 대기를 한 수명주기로 묶는다.
 * Event 방식에서는 payload가 이 태스크의 Ability를 가리킬 때만 Execute를 전달한다.
 */
UCLASS()
class PROJECTKC_API UKCAbilityTask_PlayActionMontage : public UAbilityTask
{
	GENERATED_BODY()

public:
	static UKCAbilityTask_PlayActionMontage* Create(
		UGameplayAbility* OwningAbility,
		const FKCActionMontageConfigStruct& MontageConfig,
		bool bInListenForExecuteEvent);

	UPROPERTY(BlueprintAssignable)
	FKCActionMontageExecuteDelegate OnExecute;

	UPROPERTY(BlueprintAssignable)
	FKCActionMontageTerminalDelegate OnCompleted;

	UPROPERTY(BlueprintAssignable)
	FKCActionMontageTerminalDelegate OnInterrupted;

	virtual void Activate() override;

protected:
	virtual void OnDestroy(bool bAbilityEnded) override;

private:
	UFUNCTION()
	void HandleGameplayEvent(FGameplayEventData Payload);

	UFUNCTION()
	void HandleMontageCompleted();

	UFUNCTION()
	void HandleMontageInterrupted();

	void FinishTask(bool bInterrupted);

	UPROPERTY()
	TObjectPtr<UAnimMontage> Montage;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> EventTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	FGameplayAbilitySpecHandle AbilityHandle;
	float PlayRate = 1.0f;
	FName StartSection = NAME_None;
	bool bTerminal = false;
	bool bCleaningUp = false;
	bool bListenForExecuteEvent = true;
};
