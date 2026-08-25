#pragma once

#include "CoreMinimal.h"
#include "KCDishFinishedStruct.generated.h"

/**
 * ProgressBar가 100%가 되면 냄비에서 발행하는 요리 완성 이벤트 데이터입니다.
 * GameSystem이 받아서 점수 획득을 반영합니다.
 */
USTRUCT(BlueprintType)
struct FKCDishFinishedStruct
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 TeamId = 0;

	UPROPERTY(BlueprintReadOnly)
	FName RecipeRowName;
};