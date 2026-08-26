#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/UI/Common/Widget/KCUserWidget.h"
#include "ProjectKC/UI/HUD/ViewModel/KCHUDViewModel.h"
#include "KCHUDRecipeListWidget.generated.h"

class UListView;
class UVerticalBox;
class UKCHUDRecipeEntryWidget;

UCLASS(BlueprintType)
class PROJECTKC_API UKCHUDRecipeListItem : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "KC|UI")
	FKCRecipeViewData Recipe;
};

UCLASS(Abstract, Blueprintable)
class PROJECTKC_API UKCHUDRecipeListWidget : public UKCUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativePreConstruct() override;

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetRecipes(const TArray<FKCRecipeViewData>& Recipes);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|UI")
	TSubclassOf<UKCHUDRecipeEntryWidget> EntryWidgetClass;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UListView> RecipeListView;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UVerticalBox> RecipeEntryContainer;

private:
	TSubclassOf<UKCHUDRecipeEntryWidget> ResolveEntryWidgetClass() const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UKCHUDRecipeListItem>> RecipeItems;
};
