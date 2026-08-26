#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/UI/Common/Widget/KCUserWidget.h"
#include "ProjectKC/UI/HUD/ViewModel/KCHUDViewModel.h"
#include "KCHUDRecipeListWidget.generated.h"

UCLASS(Abstract, Blueprintable)
class PROJECTKC_API UKCHUDRecipeListWidget : public UKCUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "KC|UI")
	void SetRecipes(const TArray<FKCRecipeViewData>& Recipes);
};
