#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "KCLevelSettings.generated.h"

class UDataTable;

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "ProjectKC Level"))
class PROJECTKC_API UKCLevelSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Level")
	TSoftObjectPtr<UDataTable> LevelInfoTable;
};
