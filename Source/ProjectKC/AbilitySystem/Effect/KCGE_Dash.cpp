#include "ProjectKC/AbilitySystem/Effect/KCGE_Dash.h"

#include "ProjectKC/AbilitySystem/Attribute/KCCharacterAttributeSet.h"

namespace KCDashEffect
{
	constexpr float StaminaCost = 20.0f;
	constexpr float CooldownDuration = 0.8f;
}

UKCGE_DashCost::UKCGE_DashCost()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo& StaminaModifier = Modifiers.AddDefaulted_GetRef();
	StaminaModifier.Attribute = UKCCharacterAttributeSet::GetStaminaAttribute();
	StaminaModifier.ModifierOp = EGameplayModOp::Additive;
	StaminaModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(
		FScalableFloat(-KCDashEffect::StaminaCost));
}

UKCGE_DashCooldown::UKCGE_DashCooldown()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(
		FScalableFloat(KCDashEffect::CooldownDuration));
}
