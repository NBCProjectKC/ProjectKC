#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/UI/Common/Widget/KCUserWidget.h"
#include "ProjectKC/UI/HUD/ViewModel/KCHUDViewModel.h"
#include "KCHUDRecipeIngredientWidget.generated.h"

UCLASS(Abstract, Blueprintable)
class PROJECTKC_API UKCHUDRecipeIngredientWidget : public UKCUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "KC|UI")
	void SetIngredient(const FKCRecipeIngredientViewData& Ingredient);
};
