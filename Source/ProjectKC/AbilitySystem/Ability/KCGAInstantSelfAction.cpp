#include "ProjectKC/AbilitySystem/Ability/KCGAInstantSelfAction.h"

#include "AbilitySystemComponent.h"
#include "ProjectKC/AbilitySystem/Tag/KCGameplayTags.h"

UKCGAInstantSelfAction::UKCGAInstantSelfAction()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	AddRequiredActionHook(TAG_KC_ActionHook_Self_OnActivate);
}

void UKCGAInstantSelfAction::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	if (!IsActive())
	{
		return;
	}

	UAbilitySystemComponent* AbilitySystem =
		GetAbilitySystemComponentFromActorInfo();
	const bool bCommitted = CommitAbility(Handle, ActorInfo, ActivationInfo);
	const bool bExecuted = bCommitted && AbilitySystem &&
		ExecuteActionHook(
			TAG_KC_ActionHook_Self_OnActivate,
			AbilitySystem,
			GetAvatarActorFromActorInfo());
	EndAbility(Handle, ActorInfo, ActivationInfo, true, !bExecuted);
}
