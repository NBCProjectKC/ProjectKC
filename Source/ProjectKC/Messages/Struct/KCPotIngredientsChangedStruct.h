#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "KCPotIngredientsChangedStruct.generated.h"

USTRUCT(BlueprintType)
struct FKCPotIngredientsChangedStruct
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "KC|Game")
	int32 TeamId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "KC|Game", meta = (Categories = "Item.Id"))
	FGameplayTagContainer Ingredients;
};
