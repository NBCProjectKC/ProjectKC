#pragma once

#include "CoreMinimal.h"
#include "KCDishRuinedStruct.generated.h"

/**
 * 잘못된 재료가 들어갔을 때 발행하는 이벤트 데이터.
 * GameSystem이 재료 투입 시 판정하여 발행합니다. UI가 받아서 표시합니다.
 * 추후 페널티 적용 가능성 있습니다.
 */
USTRUCT(BlueprintType)
struct FKCDishRuinedStruct
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 TeamId = 0;
};