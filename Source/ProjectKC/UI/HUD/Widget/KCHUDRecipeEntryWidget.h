#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "ProjectKC/UI/Common/Widget/KCUserWidget.h"
#include "ProjectKC/UI/HUD/ViewModel/KCHUDViewModel.h"
#include "KCHUDRecipeEntryWidget.generated.h"

class UListView;
class UTextBlock;
class UVerticalBox;
class UKCColorStyle;
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

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UTextBlock> DifficultyText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UTextBlock> FoodNameText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UListView> IngredientListView;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UVerticalBox> IngredientEntryContainer;

private:
	static FText BuildStarsText(int32 DifficultyStars);
	TSubclassOf<UKCHUDRecipeIngredientWidget> ResolveIngredientWidgetClass() const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UKCHUDRecipeIngredientListItem>> IngredientItems;
};
