#include "ProjectKC/AbilitySystem/Effect/KCGE_StaminaRegen.h"

#include "ProjectKC/AbilitySystem/Attribute/KCCharacterAttributeSet.h"

namespace KCStaminaRegen
{
	constexpr float PeriodSeconds = 0.2f;
	constexpr float AmountPerPeriod = 2.0f;
}

UKCGE_StaminaRegen::UKCGE_StaminaRegen()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	Period = FScalableFloat(KCStaminaRegen::PeriodSeconds);
	bExecutePeriodicEffectOnApplication = false;

	FGameplayModifierInfo& StaminaModifier = Modifiers.AddDefaulted_GetRef();
	StaminaModifier.Attribute = UKCCharacterAttributeSet::GetStaminaAttribute();
	StaminaModifier.ModifierOp = EGameplayModOp::Additive;
	StaminaModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(
		FScalableFloat(KCStaminaRegen::AmountPerPeriod));
}
