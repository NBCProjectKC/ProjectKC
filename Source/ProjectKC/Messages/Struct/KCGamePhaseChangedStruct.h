#pragma once

#include "CoreMinimal.h"
#include "KCGamePhaseType.h"
#include "KCGamePhaseChangedStruct.generated.h"

USTRUCT(BlueprintType)
struct FKCGamePhaseChangedStruct
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	EKCGamePhaseType NewPhase = EKCGamePhaseType::Waiting;
};
