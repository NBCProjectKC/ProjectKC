#include "ProjectKC/UI/HUD/Widget/KCHUDRecipeIngredientWidget.h"

#include "Components/TextBlock.h"
#include "ProjectKC/UI/Common/Style/KCColorStyle.h"
#include "ProjectKC/UI/HUD/Widget/KCHUDRecipeEntryWidget.h"

void UKCHUDRecipeIngredientWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	if (const UKCHUDRecipeIngredientListItem* IngredientItem = Cast<UKCHUDRecipeIngredientListItem>(ListItemObject))
	{
		SetIngredient(IngredientItem->Ingredient);
	}
}

void UKCHUDRecipeIngredientWidget::SetIngredient(const FKCRecipeIngredientViewData& Ingredient)
{
	if (IngredientText)
	{
		IngredientText->SetText(Ingredient.DisplayName);
	}

	if (CheckText)
	{
		CheckText->SetVisibility(Ingredient.bSubmitted
			? ESlateVisibility::SelfHitTestInvisible
			: ESlateVisibility::Collapsed);
	}

	BP_OnIngredientSet(Ingredient);
}

void UKCHUDRecipeIngredientWidget::NativeApplyColorStyle(const UKCColorStyle* InColorStyle)
{
	if (!InColorStyle)
	{
		return;
	}

	if (IngredientText)
	{
		IngredientText->SetColorAndOpacity(FSlateColor(InColorStyle->RecipeText));
	}

	if (CheckText)
	{
		CheckText->SetColorAndOpacity(FSlateColor(InColorStyle->RecipeCheck));
	}
}
