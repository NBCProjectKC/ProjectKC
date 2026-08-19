#include "ProjectKC/AbilitySystem/Tag/KCGameplayTagBlueprintLibrary.h"

/**
 * @brief Retrieves a registered gameplay tag by name.
 *
 * @param TagName Name of the gameplay tag to retrieve.
 * @return The matching registered gameplay tag, or an empty tag if the name is unset or unregistered.
 */
FGameplayTag UKCGameplayTagBlueprintLibrary::RequestRegisteredGameplayTag(
	FName TagName)
{
	return TagName.IsNone()
		? FGameplayTag()
		: FGameplayTag::RequestGameplayTag(TagName, false);
}
