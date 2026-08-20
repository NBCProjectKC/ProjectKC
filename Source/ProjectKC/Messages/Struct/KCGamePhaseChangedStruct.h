#pragma once

#include "CoreMinimal.h"
#include "GameSystem/KCGamePhaseType.h"
#include "KCGamePhaseChangedStruct.generated.h"

USTRUCT(BlueprintType)
struct FKCGamePhaseChangedStruct
{
	GENERATED_BODY()
// 수정사항 잡히는지
	UPROPERTY(BlueprintReadOnly)
	EKCGamePhaseType NewPhase = EKCGamePhaseType::Waiting;
};
