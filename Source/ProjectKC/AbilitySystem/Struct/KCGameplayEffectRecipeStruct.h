#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ProjectKC/AbilitySystem/Struct/KCSetByCallerValueStruct.h"
#include "KCGameplayEffectRecipeStruct.generated.h"

class UGameplayEffect;

/** 하나의 GameplayEffectSpec을 만들기 위한 소스 독립적인 재료다. */
USTRUCT(BlueprintType)
struct PROJECTKC_API FKCGameplayEffectRecipeStruct
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect")
	TSubclassOf<UGameplayEffect> EffectClass;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Effect",
		meta = (ClampMin = "0.0"))
	float EffectLevel = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect")
	TArray<FKCSetByCallerValueStruct> SetByCallers;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect")
	FGameplayTagContainer DynamicGrantedTags;

	bool Validate(FString& OutError) const;
};
