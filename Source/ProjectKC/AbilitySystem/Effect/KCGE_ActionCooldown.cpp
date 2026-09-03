#include "ProjectKC/AbilitySystem/Effect/KCGE_ActionCooldown.h"

#include "ProjectKC/AbilitySystem/Tag/KCAbilityGameplayTags.h"

UKCGE_ActionCooldown::UKCGE_ActionCooldown()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	FSetByCallerFloat CooldownDuration;
	CooldownDuration.DataTag = TAG_KC_Data_Cooldown_Duration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(CooldownDuration);
}
