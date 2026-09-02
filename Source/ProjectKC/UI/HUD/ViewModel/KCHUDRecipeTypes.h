#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "KCHUDRecipeTypes.generated.h"

class UTexture2D;

USTRUCT(BlueprintType)
struct FKCRecipeIngredientViewData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	FText DisplayName;

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI", meta = (Categories = "Item.Id"))
	FGameplayTag IngredientId;

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	bool bSubmitted = false;

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	int32 SubmittedTeamId = INDEX_NONE;

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	bool bSubmittedByTeam0 = false;

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	bool bSubmittedByTeam1 = false;
};

USTRUCT(BlueprintType)
struct FKCRecipeViewData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI", meta = (ClampMin = "0", ClampMax = "5"))
	int32 DifficultyStars = 0;

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	FName RecipeRowName;

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	FText DisplayName;

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	TSoftObjectPtr<UTexture2D> FoodIcon;

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	TArray<FKCRecipeIngredientViewData> Ingredients;

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	float Team0Progress = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	float Team1Progress = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	bool bTeam0ProgressVisible = false;

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	bool bTeam1ProgressVisible = false;

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	bool bTeam0Cooking = false;

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	bool bTeam1Cooking = false;
};
