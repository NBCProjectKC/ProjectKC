#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GameplayAbilitySpecHandle.h"
#include "KCAbilitySystemComponent.generated.h"

struct FGameplayEventData;
class UKCAbilityDefinition;

/** 소스 독립적인 Ability Binding을 정확한 SpecHandle 단위로 관리한다. */
UCLASS(BlueprintType, Blueprintable, ClassGroup = (KC), meta = (BlueprintSpawnableComponent))
class PROJECTKC_API UKCAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	static bool ResolveDefinitionFromSource(
		const UObject* SourceObject,
		const UKCAbilityDefinition*& OutDefinition,
		FString* OutError = nullptr);

	UFUNCTION(BlueprintCallable, Category = "KC|Ability")
	FGameplayAbilitySpecHandle GrantAbilityFromSource(
		UObject* SourceObject,
		int32 InputId = -1);

	FGameplayAbilitySpecHandle GrantAbilityDefinition(
		const UKCAbilityDefinition* Definition,
		UObject* SourceObject,
		int32 InputId = INDEX_NONE);

	UFUNCTION(BlueprintCallable, Category = "KC|Ability")
	bool TryActivateGrantedAbility(FGameplayAbilitySpecHandle AbilityHandle);

	bool TryActivateGrantedAbilityWithEvent(
		FGameplayAbilitySpecHandle AbilityHandle,
		FGameplayTag EventTag,
		const FGameplayEventData& EventData);

	UFUNCTION(BlueprintCallable, Category = "KC|Ability")
	bool RevokeAbilityByHandle(
		FGameplayAbilitySpecHandle AbilityHandle,
		bool bCancelActiveAbility = true);

	FGameplayAbilitySpecHandle FindGrantedAbilityBySource(
		const UObject* SourceObject) const;
};
