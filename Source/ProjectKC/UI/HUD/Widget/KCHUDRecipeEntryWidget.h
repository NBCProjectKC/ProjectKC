#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "ProjectKC/UI/Common/Widget/KCUserWidget.h"
#include "ProjectKC/UI/HUD/ViewModel/KCHUDViewModel.h"
#include "KCHUDRecipeEntryWidget.generated.h"

class UTextBlock;
class UCheckBox;
class UPanelWidget;
class UVerticalBox;
class UKCColorStyle;
class UTexture2D;
class UProgressBar;
class UListView;
class UImage;
class UWidgetAnimation;
class UKCHUDRecipeListItem;
class UKCRecipeIngredientListItem;

UCLASS(Abstract, Blueprintable)
class PROJECTKC_API UKCHUDRecipeEntryWidget : public UKCUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

public:
	virtual void NativePreConstruct() override;

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetRecipe(const FKCRecipeViewData& Recipe);

protected:
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
	virtual void NativeOnEntryReleased() override;
	virtual void NativeApplyColorStyle(const UKCColorStyle* InColorStyle) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "KC|UI", meta = (DisplayName = "On Recipe Set"))
	void BP_OnRecipeSet(const FKCRecipeViewData& Recipe);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|UI|Recipe")
	TObjectPtr<UTexture2D> FilledStarTexture;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|UI|Recipe")
	TObjectPtr<UTexture2D> EmptyStarTexture;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UTextBlock> FoodNameText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UImage> Team1CookingImage;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UImage> Team2CookingImage;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> Team1Cooking;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> Team2Cooking;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UPanelWidget> HB_Stars;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UListView> LV_Ingredients;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UVerticalBox> VB_TestIngredients;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UProgressBar> Team1ProgressBar;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UProgressBar> Team2ProgressBar;

private:
	void BindRecipeListItem(UKCHUDRecipeListItem* RecipeItem);
	void UnbindRecipeListItem();
	void HandleRecipeListItemChanged(const FKCRecipeViewData& Recipe);
	void RefreshDifficultyStars(int32 DifficultyStars);
	void RefreshCookingIndicators(const FKCRecipeViewData& Recipe);
	void ApplyCookingIndicator(UImage* CookingImage, UWidgetAnimation* CookingAnimation, bool bCooking);
	void RefreshIngredientList(const TArray<FKCRecipeIngredientViewData>& Ingredients);
	bool CanReuseIngredientItems(const TArray<FKCRecipeIngredientViewData>& Ingredients) const;
	void RefreshTestIngredientWidgets(const TArray<FKCRecipeIngredientViewData>& Ingredients);
	void RefreshTeamProgressBars(const FKCRecipeViewData& Recipe);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UKCRecipeIngredientListItem>> IngredientItems;

	TWeakObjectPtr<UKCHUDRecipeListItem> BoundRecipeItem;
	FDelegateHandle RecipeChangedHandle;
};
