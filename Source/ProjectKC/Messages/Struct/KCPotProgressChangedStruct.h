#pragma once

#include "CoreMinimal.h"
#include "KCPotProgressChangedStruct.generated.h"

USTRUCT(BlueprintType)
struct FKCPotProgressChangedStruct
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "KC|Game")
	int32 TeamId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "KC|Game")
	float ProgressPercent = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "KC|Game")
	int32 RemainingSeconds = 0;

	UPROPERTY(BlueprintReadOnly, Category = "KC|Game")
	bool bVisible = false;

	UPROPERTY(BlueprintReadOnly, Category = "KC|Game")
	bool bCompleted = false;
};
