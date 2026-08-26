#include "ProjectKC/AbilitySystem/Effect/KCGE_Dash.h"

#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "ProjectKC/AbilitySystem/Attribute/KCCharacterAttributeSet.h"
#include "ProjectKC/AbilitySystem/Tag/KCAbilityGameplayTags.h"

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

UKCGE_DashCooldown::UKCGE_DashCooldown(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(
		FScalableFloat(KCDashEffect::CooldownDuration));

	UTargetTagsGameplayEffectComponent* TargetTagsComponent =
		ObjectInitializer.CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(
			this, TEXT("DashCooldownTargetTags"));

	FInheritedTagContainer GrantedTags;
	GrantedTags.AddTag(TAG_KC_Cooldown_Ability_Dash);
	TargetTagsComponent->SetAndApplyTargetTagChanges(GrantedTags);
	GEComponents.Add(TargetTagsComponent);
}
