#pragma once

#include "CoreMinimal.h"
#include "KCPotStateType.generated.h"

UENUM(BlueprintType)
enum class EKCPotStateType : uint8
{
	Idle,
	Cooking,
	Completed
};
