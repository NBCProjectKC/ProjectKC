#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "KCCookingProgressAttributeSet.generated.h"

/**
 * 조리 가능한 월드 Actor가 소유하는 진행도 AttributeSet이다.
 * 진행도는 0에서 시작하며 항상 0~MaxCookingProgress 범위로 유지된다.
 */
UCLASS(BlueprintType)
class PROJECTKC_API UKCCookingProgressAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UKCCookingProgressAttributeSet();

	ATTRIBUTE_ACCESSORS_BASIC(UKCCookingProgressAttributeSet, CookingProgress)
	ATTRIBUTE_ACCESSORS_BASIC(UKCCookingProgressAttributeSet, MaxCookingProgress)

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
	void OnRep_CookingProgress(
		const FGameplayAttributeData& OldCookingProgress);

	UFUNCTION()
	void OnRep_MaxCookingProgress(
		const FGameplayAttributeData& OldMaxCookingProgress);

	UPROPERTY(
		BlueprintReadOnly,
		ReplicatedUsing = OnRep_CookingProgress,
		Category = "KC|Cooking")
	FGameplayAttributeData CookingProgress;

	UPROPERTY(
		BlueprintReadOnly,
		ReplicatedUsing = OnRep_MaxCookingProgress,
		Category = "KC|Cooking")
	FGameplayAttributeData MaxCookingProgress;

private:
	float ClampAttributeValue(
		const FGameplayAttribute& Attribute,
		float NewValue) const;
};
