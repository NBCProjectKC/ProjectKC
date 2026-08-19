#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/AbilitySystem/Ability/KCGameplayAbility.h"
#include "KCGAInstantSelfAction.generated.h"

/** 자신을 실행 대상으로 삼아 Self.OnActivate Fragment를 실행하고 즉시 끝난다. */
UCLASS(Blueprintable)
class PROJECTKC_API UKCGAInstantSelfAction : public UKCGameplayAbility
{
	GENERATED_BODY()

public:
	UKCGAInstantSelfAction();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
