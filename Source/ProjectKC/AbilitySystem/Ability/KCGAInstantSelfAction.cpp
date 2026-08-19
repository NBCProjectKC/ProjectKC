#include "ProjectKC/AbilitySystem/Ability/KCGAInstantSelfAction.h"

#include "AbilitySystemComponent.h"
#include "ProjectKC/AbilitySystem/Tag/KCAbilityGameplayTags.h"

/**
 * @brief Configures the ability for server-only execution and registers the self-activation action hook.
 */
UKCGAInstantSelfAction::UKCGAInstantSelfAction()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	AddRequiredActionHook(TAG_KC_ActionHook_Self_OnActivate);
}

/**
 * @brief Determines whether the self-action can be prepared.
 *
 * @param TriggerEventData Gameplay event data associated with the action.
 * @return true if an ability system component is available, false otherwise.
 */
bool UKCGAInstantSelfAction::PrepareUseAction(
	const FGameplayEventData* TriggerEventData)
{
	return GetAbilitySystemComponentFromActorInfo() != nullptr;
}

/**
 * @brief Executes the self-activation action hook.
 *
 * @return `true` if the hook executes successfully; `false` if the ability system component is unavailable or execution fails.
 */
bool UKCGAInstantSelfAction::ExecuteUseAction()
{
	UAbilitySystemComponent* AbilitySystem =
		GetAbilitySystemComponentFromActorInfo();
	return AbilitySystem && ExecuteActionHook(
		TAG_KC_ActionHook_Self_OnActivate,
		AbilitySystem,
		GetAvatarActorFromActorInfo());
}
