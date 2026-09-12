#pragma once

#include "CoreMinimal.h"
#include "KCGamePhaseType.generated.h"

UENUM(BlueprintType)
enum class EKCGamePhaseType : uint8
{
	Waiting		UMETA(DisplayName = "대기중"),
	Countdown	UMETA(DisplayName = "카운트다운"),
	Playing		UMETA(DisplayName = "진행중"),
	Ending		UMETA(DisplayName = "종료")
};
