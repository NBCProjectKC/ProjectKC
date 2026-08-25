#include "ProjectKC/AbilitySystem/Effect/KCGE_CookingProgressDecrease.h"

#include "ProjectKC/AbilitySystem/Attribute/KCCookingProgressAttributeSet.h"
#include "ProjectKC/AbilitySystem/Tag/KCAbilityGameplayTags.h"

UKCGE_CookingProgressDecrease::UKCGE_CookingProgressDecrease()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo& ProgressModifier = Modifiers.AddDefaulted_GetRef();
	ProgressModifier.Attribute =
		UKCCookingProgressAttributeSet::GetCookingProgressAttribute();
	ProgressModifier.ModifierOp = EGameplayModOp::Additive;

	FSetByCallerFloat ProgressDecrease;
	ProgressDecrease.DataTag = TAG_KC_Data_Cooking_Progress_Decrease;
	ProgressModifier.ModifierMagnitude =
		FGameplayEffectModifierMagnitude(ProgressDecrease);
}
