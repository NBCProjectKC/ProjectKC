#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "KCLevelInfoRow.generated.h"

USTRUCT(BlueprintType)
struct PROJECTKC_API FKCLevelInfoRow : public FTableRowBase
{
	GENERATED_BODY()

	/** 실제 .umap 파일 이름 (예: "L_GasRange") */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KC|LevelInfo")
	FName MapName;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KC|LevelInfo")
	TObjectPtr<USoundBase> BGM;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KC|LevelInfo")
	TArray<FPrimaryAssetType> AssetTypesToPreload;
};