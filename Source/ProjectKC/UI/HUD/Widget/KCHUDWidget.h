#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/UI/Common/Widget/KCUserWidget.h"
#include "KCHUDWidget.generated.h"

class UBorder;
class UCommonTextStyle;
class UTextBlock;
class UKCHUDRecipeListWidget;
class UKCHUDViewModel;
struct FKCRecipeViewData;

UCLASS(Abstract, Blueprintable)
class PROJECTKC_API UKCHUDWidget : public UKCUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	UKCHUDViewModel* GetHUDViewModel() const { return HUDViewModel; }

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	UKCHUDRecipeListWidget* GetRecipeListWidget() const { return RecipeListWidget; }

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UKCHUDRecipeListWidget> RecipeListWidget;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "KC|UI")
	TObjectPtr<UKCHUDViewModel> HUDViewModel;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|UI|Style")
	TSubclassOf<UCommonTextStyle> ScoreTextStyle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|UI|Style")
	TSubclassOf<UCommonTextStyle> RecipeTitleTextStyle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|UI|Style")
	TSubclassOf<UCommonTextStyle> RecipeIngredientTextStyle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|UI|Style")
	TSubclassOf<UCommonTextStyle> RecipeCheckTextStyle;

private:
	void ApplyHUDTextStyles();
	void HandleTeamScoresChanged(const TArray<int32>& TeamScores);
	void HandleRecipesChanged();
	void RefreshRoughHUD();
	void RefreshRoughScore();
	void RefreshRoughRecipes();
	static FText BuildStarsText(int32 DifficultyStars);
	static FText BuildIngredientsText(const FKCRecipeViewData& Recipe);
	static FText BuildChecksText(const FKCRecipeViewData& Recipe);
	static void ApplyTextStyle(UTextBlock* TextBlock, TSubclassOf<UCommonTextStyle> TextStyleClass);

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HUDScoreText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RecipeStars_0;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RecipeStars_1;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RecipeStars_2;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> FoodLabel_0;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> FoodLabel_1;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> FoodLabel_2;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> IngredientSlots_0;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> IngredientSlots_1;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> IngredientSlots_2;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> IngredientChecks_0;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> IngredientChecks_1;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> IngredientChecks_2;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> TeamBar_0;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> TeamBar_2;
};
