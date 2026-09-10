#pragma once

#include "CoreMinimal.h"
#include "GameSystem/Enum/KCLevelType.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "KCLevelTypeLibrary.generated.h"

struct FKCLevelInfoRow;
/**
 * EKCLevelType과 실제 레벨 이름(FName) 사이의 변환을 관리합니다.
 * 레벨 이름이 바뀌어도 이 파일만 고치면 됨.
 */
UCLASS()
class PROJECTKC_API UKCLevelTypeLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "KC|Level")
	static FName GetLevelName(EKCLevelType LevelType);

	UFUNCTION(BlueprintPure, Category = "KC|Level")
	static EKCLevelType GetLevelType(FName LevelName);
	
	UFUNCTION(BlueprintPure, Category = "KC|Level")
	static EKCLevelType GetLevelTypeFromWorld(const UWorld* World);

	UFUNCTION(BlueprintPure, Category = "KC|Level")
	static bool IsPlayableLevel(EKCLevelType LevelType);

	UFUNCTION(BlueprintPure, Category = "KC|Level")
	static bool IsInGameLevel(EKCLevelType LevelType);
	
	static const FKCLevelInfoRow* GetLevelInfoRow(EKCLevelType LevelType);
};