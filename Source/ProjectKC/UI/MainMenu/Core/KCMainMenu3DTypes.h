#pragma once

#include "CoreMinimal.h"
#include "KCMainMenu3DTypes.generated.h"

UENUM(BlueprintType)
enum class EKCMainMenu3DAction : uint8
{
	CreateLobby UMETA(DisplayName = "Create Lobby"),
	Option UMETA(DisplayName = "Option"),
	Exit UMETA(DisplayName = "Exit")
};
