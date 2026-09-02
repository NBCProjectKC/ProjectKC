#include "ProjectKC/UI/Interaction/Data/KCInteractionPromptRegistry.h"

bool UKCInteractionPromptRegistry::FindPrompt(
	const FGameplayTag PromptTag,
	FKCInteractionPromptEntry& OutEntry) const
{
	if (!PromptTag.IsValid())
	{
		return false;
	}

	for (const FKCInteractionPromptEntry& Entry : PromptEntries)
	{
		if (Entry.PromptTag == PromptTag)
		{
			OutEntry = Entry;
			return true;
		}
	}

	return false;
}
