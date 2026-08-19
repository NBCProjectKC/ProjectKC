#pragma once

#include "CoreMinimal.h"
#include "KCGamePhaseType.generated.h"

UENUM(BlueprintType)
enum class EKCGamePhaseType : uint8
{
	Waiting		UMETA(DisplayName = "대기중"),
	Playing		UMETA(DisplayName = "진행중"),
	Ended		UMETA(DisplayName = "종료")
};
