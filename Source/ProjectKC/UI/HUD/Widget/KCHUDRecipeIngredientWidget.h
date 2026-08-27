#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "ProjectKC/UI/Common/Widget/KCUserWidget.h"
#include "ProjectKC/UI/HUD/ViewModel/KCHUDViewModel.h"
#include "KCHUDRecipeIngredientWidget.generated.h"

class UTextBlock;
class UKCColorStyle;

UCLASS(Abstract, Blueprintable)
class PROJECTKC_API UKCHUDRecipeIngredientWidget : public UKCUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetIngredient(const FKCRecipeIngredientViewData& Ingredient);

protected:
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
	virtual void NativeApplyColorStyle(const UKCColorStyle* InColorStyle) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "KC|UI", meta = (DisplayName = "On Ingredient Set"))
	void BP_OnIngredientSet(const FKCRecipeIngredientViewData& Ingredient);

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UTextBlock> IngredientText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UTextBlock> CheckText;
};
