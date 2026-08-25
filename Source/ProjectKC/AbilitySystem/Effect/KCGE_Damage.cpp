#include "ProjectKC/AbilitySystem/Effect/KCGE_Damage.h"

#include "ProjectKC/AbilitySystem/Attribute/KCCharacterAttributeSet.h"
#include "ProjectKC/AbilitySystem/Tag/KCAbilityGameplayTags.h"

UKCGE_Damage::UKCGE_Damage()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo& HealthModifier = Modifiers.AddDefaulted_GetRef();
	HealthModifier.Attribute = UKCCharacterAttributeSet::GetHealthAttribute();
	HealthModifier.ModifierOp = EGameplayModOp::Additive;

	FSetByCallerFloat HealthDelta;
	HealthDelta.DataTag = TAG_KC_Data_Damage_Flat;
	HealthModifier.ModifierMagnitude =
		FGameplayEffectModifierMagnitude(HealthDelta);
}
