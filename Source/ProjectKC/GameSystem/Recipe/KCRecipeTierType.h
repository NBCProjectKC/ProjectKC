#pragma once

#include "CoreMinimal.h"
#include "KCRecipeTierType.generated.h"

UENUM(BlueprintType)
enum class EKCRecipeTierType : uint8
{
	Low		UMETA(DisplayName = "하급"),   // 재료 2개, 2점
	Medium	UMETA(DisplayName = "중급"),   // 재료 3개, 3점
	High	UMETA(DisplayName = "상급")    // 재료 4개, 4점
};