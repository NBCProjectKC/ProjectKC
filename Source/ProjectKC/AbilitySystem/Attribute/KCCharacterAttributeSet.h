#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "KCCharacterAttributeSet.generated.h"

/**
 * KC 캐릭터가 공통으로 소유하는 핵심 Gameplay Attribute 모음이다.
 * 같은 ASC와 생명주기를 공유하는 생명력, 스태미나, 이동속도를 함께 관리한다.
 */
UCLASS(BlueprintType)
class PROJECTKC_API UKCCharacterAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UKCCharacterAttributeSet();

	ATTRIBUTE_ACCESSORS_BASIC(UKCCharacterAttributeSet, Health)
	ATTRIBUTE_ACCESSORS_BASIC(UKCCharacterAttributeSet, MaxHealth)
	ATTRIBUTE_ACCESSORS_BASIC(UKCCharacterAttributeSet, Stamina)
	ATTRIBUTE_ACCESSORS_BASIC(UKCCharacterAttributeSet, MaxStamina)
	ATTRIBUTE_ACCESSORS_BASIC(UKCCharacterAttributeSet, MoveSpeed)

	virtual void PreAttributeBaseChange(
		const FGameplayAttribute& Attribute,
		float& NewValue) const override;
	virtual void PreAttributeChange(
		const FGameplayAttribute& Attribute,
		float& NewValue) override;
	virtual void PostAttributeChange(
		const FGameplayAttribute& Attribute,
		float OldValue,
		float NewValue) override;
	virtual void PostGameplayEffectExecute(
		const FGameplayEffectModCallbackData& Data) override;
	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldHealth);

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);

	UFUNCTION()
	void OnRep_Stamina(const FGameplayAttributeData& OldStamina);

	UFUNCTION()
	void OnRep_MaxStamina(const FGameplayAttributeData& OldMaxStamina);

	UFUNCTION()
	void OnRep_MoveSpeed(const FGameplayAttributeData& OldMoveSpeed);

	UPROPERTY(
		BlueprintReadOnly,
		ReplicatedUsing = OnRep_Health,
		Category = "KC|Attributes")
	FGameplayAttributeData Health;

	UPROPERTY(
		BlueprintReadOnly,
		ReplicatedUsing = OnRep_MaxHealth,
		Category = "KC|Attributes")
	FGameplayAttributeData MaxHealth;

	UPROPERTY(
		BlueprintReadOnly,
		ReplicatedUsing = OnRep_Stamina,
		Category = "KC|Attributes")
	FGameplayAttributeData Stamina;

	UPROPERTY(
		BlueprintReadOnly,
		ReplicatedUsing = OnRep_MaxStamina,
		Category = "KC|Attributes")
	FGameplayAttributeData MaxStamina;

	UPROPERTY(
		BlueprintReadOnly,
		ReplicatedUsing = OnRep_MoveSpeed,
		Category = "KC|Attributes")
	FGameplayAttributeData MoveSpeed;

private:
	float ClampAttributeValue(
		const FGameplayAttribute& Attribute,
		float NewValue) const;
};
