#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "KCRecipeTierType.h"
#include "KCRecipeStruct.generated.h"

// 레시피 DataTable 행

USTRUCT(BlueprintType)
struct FKCRecipeStruct : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FText RecipeName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	EKCRecipeTierType Tier = EKCRecipeTierType::Low;

	// 필요한 재료 목록 (종류당 1개씩, 수량 개념 X)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<FGameplayTag> RequiredIngredients;

	// 냄비 체력바가 1초에 채워지는 양(%)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float ProgressSpeedPerSecond = 5.0f;
	
	// 레시피 등급별 점수
	int32 GetScoreValue() const
	{
		switch (Tier)
		{
		case EKCRecipeTierType::Low:    return 2;
		case EKCRecipeTierType::Medium: return 3;
		case EKCRecipeTierType::High:   return 4;
		}
		return 0;
	}
};