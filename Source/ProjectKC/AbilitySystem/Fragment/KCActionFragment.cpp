#include "ProjectKC/AbilitySystem/Fragment/KCActionFragment.h"

/**
 * @brief Validates the action fragment.
 *
 * @param OutError Error message output, cleared when validation succeeds.
 * @return true, indicating that the action fragment is valid.
 */
bool UKCActionFragment::Validate(FString& OutError) const
{
	OutError.Reset();
	return true;
}

/**
 * @brief Determines whether this fragment declares a set-by-caller tag.
 *
 * @param DataTag Tag to check.
 * @return `true` if the tag is declared, `false` otherwise.
 */
bool UKCActionFragment::DeclaresSetByCallerTag(FGameplayTag DataTag) const
{
	return false;
}

/**
 * @brief Appends the set-by-caller tags declared by this fragment.
 *
 * This fragment declares no set-by-caller tags, so the output container remains unchanged.
 *
 * @param OutTags Container to which declared tags are appended.
 */
void UKCActionFragment::AppendDeclaredSetByCallerTags(
	FGameplayTagContainer& OutTags) const
{
}

/**
 * @brief Determines whether the action fragment can execute in the given context.
 *
 * @param OutError Cleared execution error message.
 * @return `true` for any execution context.
 */
bool UKCActionFragment::CanExecute(
	const FKCActionExecutionContext& Context,
	FString& OutError) const
{
	OutError.Reset();
	return true;
}
