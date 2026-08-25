#include "ProjectKC/AbilitySystem/Attribute/KCCharacterAttributeSet.h"

#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

namespace KCCharacterAttributes
{
	constexpr float DefaultMaxHealth = 100.0f;
	constexpr float DefaultMaxStamina = 100.0f;
	constexpr float DefaultMoveSpeed = 600.0f;
	constexpr float MinimumMaxHealth = 1.0f;
}

UKCCharacterAttributeSet::UKCCharacterAttributeSet()
{
	InitMaxHealth(KCCharacterAttributes::DefaultMaxHealth);
	InitHealth(KCCharacterAttributes::DefaultMaxHealth);
	InitMaxStamina(KCCharacterAttributes::DefaultMaxStamina);
	InitStamina(KCCharacterAttributes::DefaultMaxStamina);
	InitMoveSpeed(KCCharacterAttributes::DefaultMoveSpeed);
}

void UKCCharacterAttributeSet::PreAttributeBaseChange(
	const FGameplayAttribute& Attribute,
	float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);
	NewValue = ClampAttributeValue(Attribute, NewValue);
}

void UKCCharacterAttributeSet::PreAttributeChange(
	const FGameplayAttribute& Attribute,
	float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	NewValue = ClampAttributeValue(Attribute, NewValue);
}

void UKCCharacterAttributeSet::PostAttributeChange(
	const FGameplayAttribute& Attribute,
	float OldValue,
	float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	if (Attribute == GetMaxHealthAttribute() && GetHealth() > NewValue)
	{
		SetHealth(NewValue);
	}
	else if (Attribute == GetMaxStaminaAttribute() && GetStamina() > NewValue)
	{
		SetStamina(NewValue);
	}
}

void UKCCharacterAttributeSet::PostGameplayEffectExecute(
	const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	const FGameplayAttribute& ModifiedAttribute = Data.EvaluatedData.Attribute;
	if (ModifiedAttribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
	}
	else if (ModifiedAttribute == GetMaxHealthAttribute())
	{
		SetMaxHealth(FMath::Max(
			GetMaxHealth(),
			KCCharacterAttributes::MinimumMaxHealth));
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
	}
	else if (ModifiedAttribute == GetStaminaAttribute())
	{
		SetStamina(FMath::Clamp(GetStamina(), 0.0f, GetMaxStamina()));
	}
	else if (ModifiedAttribute == GetMaxStaminaAttribute())
	{
		SetMaxStamina(FMath::Max(GetMaxStamina(), 0.0f));
		SetStamina(FMath::Clamp(GetStamina(), 0.0f, GetMaxStamina()));
	}
	else if (ModifiedAttribute == GetMoveSpeedAttribute())
	{
		SetMoveSpeed(FMath::Max(GetMoveSpeed(), 0.0f));
	}
}

void UKCCharacterAttributeSet::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(
		UKCCharacterAttributeSet,
		Health,
		COND_None,
		REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(
		UKCCharacterAttributeSet,
		MaxHealth,
		COND_None,
		REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(
		UKCCharacterAttributeSet,
		Stamina,
		COND_None,
		REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(
		UKCCharacterAttributeSet,
		MaxStamina,
		COND_None,
		REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(
		UKCCharacterAttributeSet,
		MoveSpeed,
		COND_None,
		REPNOTIFY_Always);
}

void UKCCharacterAttributeSet::OnRep_Health(
	const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UKCCharacterAttributeSet, Health, OldHealth);
}

void UKCCharacterAttributeSet::OnRep_MaxHealth(
	const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(
		UKCCharacterAttributeSet,
		MaxHealth,
		OldMaxHealth);
}

void UKCCharacterAttributeSet::OnRep_Stamina(
	const FGameplayAttributeData& OldStamina)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UKCCharacterAttributeSet, Stamina, OldStamina);
}

void UKCCharacterAttributeSet::OnRep_MaxStamina(
	const FGameplayAttributeData& OldMaxStamina)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(
		UKCCharacterAttributeSet,
		MaxStamina,
		OldMaxStamina);
}

void UKCCharacterAttributeSet::OnRep_MoveSpeed(
	const FGameplayAttributeData& OldMoveSpeed)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UKCCharacterAttributeSet, MoveSpeed, OldMoveSpeed);
}

float UKCCharacterAttributeSet::ClampAttributeValue(
	const FGameplayAttribute& Attribute,
	float NewValue) const
{
	if (!FMath::IsFinite(NewValue))
	{
		return Attribute == GetMaxHealthAttribute()
			? KCCharacterAttributes::MinimumMaxHealth
			: 0.0f;
	}

	if (Attribute == GetMaxHealthAttribute())
	{
		return FMath::Max(NewValue, KCCharacterAttributes::MinimumMaxHealth);
	}

	if (Attribute == GetHealthAttribute())
	{
		return FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}

	if (Attribute == GetMaxStaminaAttribute() ||
		Attribute == GetMoveSpeedAttribute())
	{
		return FMath::Max(NewValue, 0.0f);
	}

	if (Attribute == GetStaminaAttribute())
	{
		return FMath::Clamp(NewValue, 0.0f, GetMaxStamina());
	}

	return NewValue;
}
