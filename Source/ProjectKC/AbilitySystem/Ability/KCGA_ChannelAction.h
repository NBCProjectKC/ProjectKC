#pragma once

#include "Abilities/GameplayAbilityTypes.h"
#include "CoreMinimal.h"
#include "ProjectKC/AbilitySystem/Ability/KCGA_ActionRuntimeBase.h"
#include "KCGA_ChannelAction.generated.h"

class UKCAbilityTask_PlayActionMontage;

/** Press부터 Release까지 유지되고 Execute Event마다 결과를 만드는 ServerOnly Action이다. */
UCLASS(Blueprintable, meta = (DisplayName = "KCGA_ChannelAction"))
class PROJECTKC_API UKCGA_ChannelAction : public UKCGA_ActionRuntimeBase
{
	GENERATED_BODY()

public:
	UKCGA_ChannelAction();

	UFUNCTION()
	void HandleExecuteEvent(FGameplayEventData Payload);

	UFUNCTION()
	void HandleMontageEnded();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void InputReleased(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UKCAbilityTask_PlayActionMontage> ActiveMontageTask;
};
