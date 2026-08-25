#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "ProjectKC/AbilitySystem/Ability/KCGA_ActionRuntimeBase.h"
#include "KCGA_Action.generated.h"

class UKCAbilityTask_PlayActionMontage;

/**
 * Press 한 번에 OnExecute를 최대 한 번 실행하는 ServerOnly Action이다.
 * 몽타주가 없으면 즉시 실행하고, 있으면 자기 몽타주의 첫 Event 또는 TraceWindow를 받는다.
 */
UCLASS(Blueprintable, meta = (DisplayName = "KCGA_Action"))
class PROJECTKC_API UKCGA_Action : public UKCGA_ActionRuntimeBase
{
	GENERATED_BODY()

public:
	UKCGA_Action();

	UFUNCTION()
	void HandleExecuteEvent(FGameplayEventData Payload);

	UFUNCTION()
	void HandleMontageCompleted();

	UFUNCTION()
	void HandleMontageInterrupted();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual bool TryBeginExecutionWindow() override;

private:
	bool bExecuteAttempted = false;

	UPROPERTY(Transient)
	TObjectPtr<UKCAbilityTask_PlayActionMontage> ActiveMontageTask;
};
