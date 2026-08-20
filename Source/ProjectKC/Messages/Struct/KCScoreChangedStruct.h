#pragma once

#include "CoreMinimal.h"
#include "KCScoreChangedStruct.generated.h"

USTRUCT(BlueprintType)
struct FKCScoreChangedStruct
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 TeamId = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 NewScore = 0;
};
