#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "KCRecipeCompletedStruct.generated.h"

/**
 * 레시피에 맞게 재료가 다 모여서 요리가 시작될 때 발행하는 이벤트 데이터.
 * GameSystem 이 발행하고, 냄비에서 이걸 받아 요리 ProgressBar가 시작됩니다.
 */
USTRUCT(BlueprintType)
struct FKCRecipeCompletedStruct
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 TeamId = 0;

	// DataTable의 RowName으로 어떤 레시피인지 식별
	UPROPERTY(BlueprintReadOnly)
	FName RecipeRowName;
};