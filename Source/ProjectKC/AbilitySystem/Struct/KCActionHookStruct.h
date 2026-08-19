#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ProjectKC/AbilitySystem/Fragment/KCActionFragment.h"
#include "KCActionHookStruct.generated.h"

/** GA의 의미 있는 실행 시점과 그 시점에 조립된 결과 Fragment 목록이다. */
USTRUCT(BlueprintType)
struct PROJECTKC_API FKCActionHookStruct
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
