#include "ProjectKC/AbilitySystem/Fragment/KCActionFragment.h"

bool UKCActionFragment::Validate(FString& OutError) const
{
	OutError.Reset();
	return true;
}

bool UKCActionFragment::DeclaresSetByCallerTag(FGameplayTag DataTag) const
{
	return false;
}

void UKCActionFragment::AppendDeclaredSetByCallerTags(
	FGameplayTagContainer& OutTags) const
{
}

bool UKCActionFragment::CanExecute(
	const FKCActionExecutionContext& Context,
	FString& OutError) const
{
	OutError.Reset();
	return true;
}
