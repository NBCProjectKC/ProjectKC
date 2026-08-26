#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/UI/Common/Widget/KCUserWidget.h"
#include "ProjectKC/UI/HUD/ViewModel/KCHUDViewModel.h"
#include "KCHUDRecipeEntryWidget.generated.h"

UCLASS(Abstract, Blueprintable)
class PROJECTKC_API UKCHUDRecipeEntryWidget : public UKCUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "KC|UI")
	void SetRecipe(const FKCRecipeViewData& Recipe);
};
