#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/AbilitySystem/Ability/KCGameplayAbility.h"
#include "KCGAInstantTargetAction.generated.h"

/** GameplayEvent의 Target을 실행 대상으로 삼아 Target.OnTrigger Fragment를 실행한다. */
UCLASS(Blueprintable)
class PROJECTKC_API UKCGAInstantTargetAction : public UKCGameplayAbility
{
	GENERATED_BODY()

public:
	UKCGAInstantTargetAction();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
