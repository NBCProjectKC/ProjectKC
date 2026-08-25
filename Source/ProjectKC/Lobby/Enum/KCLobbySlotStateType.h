#pragma once

#include "CoreMinimal.h"
#include "KCLobbySlotStateType.generated.h"

UENUM(BlueprintType)
enum class EKCLobbySlotStateType : uint8
{
	Empty UMETA(DisplayName = "Empty"),
	Occupied UMETA(DisplayName = "Occupied"),
	Ready UMETA(DisplayName = "Ready"),
	Closed UMETA(DisplayName = "Closed")
};
