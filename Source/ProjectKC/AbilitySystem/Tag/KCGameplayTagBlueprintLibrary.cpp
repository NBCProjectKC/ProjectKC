#include "ProjectKC/AbilitySystem/Tag/KCGameplayTagBlueprintLibrary.h"

FGameplayTag UKCGameplayTagBlueprintLibrary::RequestRegisteredGameplayTag(
	FName TagName)
{
	return TagName.IsNone()
		? FGameplayTag()
		: FGameplayTag::RequestGameplayTag(TagName, false);
}
