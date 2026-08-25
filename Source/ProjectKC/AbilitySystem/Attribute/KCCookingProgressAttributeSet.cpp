#include "ProjectKC/AbilitySystem/Attribute/KCCookingProgressAttributeSet.h"

#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

namespace KCCookingProgressAttributes
{
	constexpr float DefaultMaxCookingProgress = 100.0f;
	constexpr float MinimumMaxCookingProgress = 1.0f;
}

UKCCookingProgressAttributeSet::UKCCookingProgressAttributeSet()
{
	InitMaxCookingProgress(
		KCCookingProgressAttributes::DefaultMaxCookingProgress);
	InitCookingProgress(0.0f);
}

void UKCCookingProgressAttributeSet::PreAttributeBaseChange(
	const FGameplayAttribute& Attribute,
	float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);
	NewValue = ClampAttributeValue(Attribute, NewValue);
}

void UKCCookingProgressAttributeSet::PreAttributeChange(
	const FGameplayAttribute& Attribute,
	float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	NewValue = ClampAttributeValue(Attribute, NewValue);
}

void UKCCookingProgressAttributeSet::PostAttributeChange(
	const FGameplayAttribute& Attribute,
	float OldValue,
	float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	if (Attribute == GetMaxCookingProgressAttribute() &&
		GetCookingProgress() > NewValue)
	{
		SetCookingProgress(NewValue);
	}
}

void UKCCookingProgressAttributeSet::PostGameplayEffectExecute(
	const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	const FGameplayAttribute& ModifiedAttribute = Data.EvaluatedData.Attribute;
	if (ModifiedAttribute == GetCookingProgressAttribute())
	{
		SetCookingProgress(FMath::Clamp(
			GetCookingProgress(),
			0.0f,
			GetMaxCookingProgress()));
	}
	else if (ModifiedAttribute == GetMaxCookingProgressAttribute())
	{
		SetMaxCookingProgress(FMath::Max(
			GetMaxCookingProgress(),
			KCCookingProgressAttributes::MinimumMaxCookingProgress));
		SetCookingProgress(FMath::Clamp(
			GetCookingProgress(),
			0.0f,
			GetMaxCookingProgress()));
	}
}

void UKCCookingProgressAttributeSet::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(
		UKCCookingProgressAttributeSet,
		CookingProgress,
		COND_None,
		REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(
		UKCCookingProgressAttributeSet,
		MaxCookingProgress,
		COND_None,
		REPNOTIFY_Always);
}

void UKCCookingProgressAttributeSet::OnRep_CookingProgress(
	const FGameplayAttributeData& OldCookingProgress)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(
		UKCCookingProgressAttributeSet,
		CookingProgress,
		OldCookingProgress);
}

void UKCCookingProgressAttributeSet::OnRep_MaxCookingProgress(
	const FGameplayAttributeData& OldMaxCookingProgress)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(
		UKCCookingProgressAttributeSet,
		MaxCookingProgress,
		OldMaxCookingProgress);
}

float UKCCookingProgressAttributeSet::ClampAttributeValue(
	const FGameplayAttribute& Attribute,
	float NewValue) const
{
	if (!FMath::IsFinite(NewValue))
	{
		return Attribute == GetMaxCookingProgressAttribute()
			? KCCookingProgressAttributes::MinimumMaxCookingProgress
			: 0.0f;
	}

	if (Attribute == GetMaxCookingProgressAttribute())
	{
		return FMath::Max(
			NewValue,
			KCCookingProgressAttributes::MinimumMaxCookingProgress);
	}

	if (Attribute == GetCookingProgressAttribute())
	{
		return FMath::Clamp(NewValue, 0.0f, GetMaxCookingProgress());
	}

	return NewValue;
}
