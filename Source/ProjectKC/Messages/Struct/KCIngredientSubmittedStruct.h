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

	// 투입된 재료의 ItemId(Item.Id.xxx)
	UPROPERTY(BlueprintReadOnly, meta = (Categories = "Item.Id"))
	FGameplayTag IngredientId;
};
