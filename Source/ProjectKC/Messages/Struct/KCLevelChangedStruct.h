#pragma once

#include "CoreMinimal.h"
#include "GameSystem/Enum/KCLevelType.h"
#include "KCLevelChangedStruct.generated.h"

USTRUCT(BlueprintType)
struct FKCLevelChangedStruct
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	EKCLevelType NewLevelType = EKCLevelType::None;
};