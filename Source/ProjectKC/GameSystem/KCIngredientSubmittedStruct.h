#pragma once

#include "CoreMinimal.h"
#include "KCIngredientSubmittedStruct.generated.h"

// this must be broadcasted at server only!

USTRUCT(BlueprintType)
struct FKCIngredientSubmittedStruct
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 TeamId = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 SubmittedCount = 1;
};
