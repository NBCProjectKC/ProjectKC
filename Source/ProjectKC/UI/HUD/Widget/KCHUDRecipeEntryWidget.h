#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "ProjectKC/UI/Common/Widget/KCUserWidget.h"
#include "ProjectKC/UI/HUD/ViewModel/KCHUDViewModel.h"
#include "KCHUDRecipeEntryWidget.generated.h"

class UTextBlock;
class UHorizontalBox;
class UVerticalBox;
class UKCColorStyle;
class UTexture2D;
class UProgressBar;
class UKCHUDRecipeIngredientWidget;

UCLASS(BlueprintType)
class PROJECTKC_API UKCHUDRecipeIngredientListItem : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "KC|UI")
	FKCRecipeIngredientViewData Ingredient;
};

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
	virtual void NativeApplyColorStyle(const UKCColorStyle* InColorStyle) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "KC|UI", meta = (DisplayName = "On Recipe Set"))
	void BP_OnRecipeSet(const FKCRecipeViewData& Recipe);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|UI")
	TSubclassOf<UKCHUDRecipeIngredientWidget> IngredientWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|UI|Recipe")
	TObjectPtr<UTexture2D> FilledStarTexture;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|UI|Recipe")
	TObjectPtr<UTexture2D> EmptyStarTexture;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UTextBlock> FoodNameText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UHorizontalBox> HB_Stars;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UHorizontalBox> HB_Ingredients;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UVerticalBox> IngredientEntryContainer;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UProgressBar> Team1ProgressBar;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UProgressBar> Team2ProgressBar;

private:
	void RefreshDifficultyStars(int32 DifficultyStars);
	void RefreshIngredientWidgets(const TArray<FKCRecipeIngredientViewData>& Ingredients);
	void RefreshTeamProgressBars(const FKCRecipeViewData& Recipe);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UKCHUDRecipeIngredientListItem>> IngredientItems;
};
