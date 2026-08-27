#pragma once

#include "CoreMinimal.h"
#include "KCActiveRecipesChangedStruct.generated.h"

USTRUCT(BlueprintType)
struct FKCActiveRecipesChangedStruct
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "KC|Game")
	TArray<FName> RecipeRowNames;
};
