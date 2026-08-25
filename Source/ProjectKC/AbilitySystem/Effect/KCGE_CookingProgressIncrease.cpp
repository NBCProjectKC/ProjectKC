#include "ProjectKC/AbilitySystem/Effect/KCGE_CookingProgressIncrease.h"

#include "ProjectKC/AbilitySystem/Attribute/KCCookingProgressAttributeSet.h"
#include "ProjectKC/AbilitySystem/Tag/KCAbilityGameplayTags.h"

UKCGE_CookingProgressIncrease::UKCGE_CookingProgressIncrease()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo& ProgressModifier = Modifiers.AddDefaulted_GetRef();
	ProgressModifier.Attribute =
		UKCCookingProgressAttributeSet::GetCookingProgressAttribute();
	ProgressModifier.ModifierOp = EGameplayModOp::Additive;

	FSetByCallerFloat ProgressIncrease;
	ProgressIncrease.DataTag = TAG_KC_Data_Cooking_Progress_Increase;
	ProgressModifier.ModifierMagnitude =
		FGameplayEffectModifierMagnitude(ProgressIncrease);
}
