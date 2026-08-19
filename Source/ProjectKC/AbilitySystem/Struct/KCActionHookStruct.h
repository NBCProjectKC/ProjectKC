#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ProjectKC/AbilitySystem/Fragment/KCActionFragment.h"
#include "KCActionHookStruct.generated.h"

/** GA의 의미 있는 실행 시점과 그 시점에 조립된 결과 Fragment 목록이다. */
USTRUCT(BlueprintType)
struct /**
 * Validates the action hook and its fragments.
 * @param OutError Receives a description of the validation failure when validation fails.
 * @return true if the action hook is valid, false otherwise.
 */

/**
 * Determines whether the action hook's fragments declare a set-by-caller tag.
 * @param DataTag Set-by-caller gameplay tag to query.
 * @return true if a fragment declares the specified tag, false otherwise.
 */
PROJECTKC_API FKCActionHookStruct
{
	GENERATED_BODY()

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "KC|Ability|Action",
		meta = (Categories = "ActionHook"))
	FGameplayTag HookTag;

	UPROPERTY(
		EditDefaultsOnly,
		Instanced,
		BlueprintReadOnly,
		Category = "KC|Ability|Action")
	TArray<TObjectPtr<UKCActionFragment>> Fragments;

	bool Validate(FString& OutError) const;
	bool DeclaresSetByCallerTag(FGameplayTag DataTag) const;
};
