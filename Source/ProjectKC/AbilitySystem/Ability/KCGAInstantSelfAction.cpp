#include "ProjectKC/AbilitySystem/Ability/KCGAInstantSelfAction.h"

#include "AbilitySystemComponent.h"
#include "ProjectKC/AbilitySystem/Tag/KCAbilityGameplayTags.h"

UKCGAInstantSelfAction::UKCGAInstantSelfAction()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	AddRequiredActionHook(TAG_KC_ActionHook_Self_OnActivate);
}

bool UKCGAInstantSelfAction::PrepareUseAction(
	const FGameplayEventData* TriggerEventData)
{
	return GetAbilitySystemComponentFromActorInfo() != nullptr;
}

bool UKCGAInstantSelfAction::ExecuteUseAction()
{
	UAbilitySystemComponent* AbilitySystem =
		GetAbilitySystemComponentFromActorInfo();
	return AbilitySystem && ExecuteActionHook(
		TAG_KC_ActionHook_Self_OnActivate,
		AbilitySystem,
		GetAvatarActorFromActorInfo());
}
