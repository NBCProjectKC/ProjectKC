#include "ProjectKC/UI/HUD/Widget/KCHUDRecipeEntryWidget.h"

#include "Components/CheckBox.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Components/VerticalBox.h"
#include "ProjectKC/UI/Common/Style/KCColorStyle.h"
#include "ProjectKC/UI/HUD/Widget/KCHUDRecipeListWidget.h"

void UKCHUDRecipeEntryWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

}

void UKCHUDRecipeEntryWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	if (const UKCHUDRecipeListItem* RecipeItem = Cast<UKCHUDRecipeListItem>(ListItemObject))
	{
		SetRecipe(RecipeItem->Recipe);
	}
}

void UKCHUDRecipeEntryWidget::SetRecipe(const FKCRecipeViewData& Recipe)
{
	BP_OnRecipeSet(Recipe);

	if (HB_Ingredients)
	{
		HB_Ingredients->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (FoodNameText)
	{
		FoodNameText->SetText(Recipe.DisplayName.IsEmpty() ? FText::FromName(Recipe.RecipeRowName) : Recipe.DisplayName);
	}

	RefreshDifficultyStars(Recipe.DifficultyStars);
	RefreshTeamProgressBars(Recipe);

	RefreshTestIngredientWidgets(Recipe.Ingredients);
}

void UKCHUDRecipeEntryWidget::RefreshDifficultyStars(const int32 DifficultyStars)
{
	if (!HB_Stars)
	{
		return;
	}

	const int32 FilledStarCount = FMath::Clamp(DifficultyStars, 0, HB_Stars->GetChildrenCount());
	for (int32 ChildIndex = 0; ChildIndex < HB_Stars->GetChildrenCount(); ++ChildIndex)
	{
		if (UWidget* StarWidget = HB_Stars->GetChildAt(ChildIndex))
		{
			StarWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

			UImage* StarImage = Cast<UImage>(StarWidget);
			if (!StarImage)
			{
				continue;
			}

			UTexture2D* StarTexture = ChildIndex < FilledStarCount ? FilledStarTexture.Get() : EmptyStarTexture.Get();
			if (StarTexture)
			{
				StarImage->SetBrushFromTexture(StarTexture, false);
			}
		}
	}
}

void UKCHUDRecipeEntryWidget::RefreshTestIngredientWidgets(const TArray<FKCRecipeIngredientViewData>& Ingredients)
{
	if (!VB_TestIngredients)
	{
		return;
	}
	int32 IngredientIndex = 0;
	for (int32 ChildIndex = 0; ChildIndex < VB_TestIngredients->GetChildrenCount(); ++ChildIndex)
	{
		UWidget* RowWidget = VB_TestIngredients->GetChildAt(ChildIndex);
		if (!RowWidget)
		{
			continue;
		}

		const bool bHasIngredient = Ingredients.IsValidIndex(IngredientIndex);
		
		RowWidget->SetVisibility(bHasIngredient
			? ESlateVisibility::SelfHitTestInvisible
			: ESlateVisibility::Collapsed);

		UHorizontalBox* RowPanel = Cast<UHorizontalBox>(RowWidget);
		if (!bHasIngredient || !RowPanel)
		{	
			continue;
		}

		const FKCRecipeIngredientViewData& Ingredient = Ingredients[IngredientIndex];
		if (UCheckBox* IngredientCheckBox = Cast<UCheckBox>(RowPanel->GetChildAt(0)))
		{
			IngredientCheckBox->SetIsChecked(Ingredient.bSubmitted);
			IngredientCheckBox->SetIsEnabled(false);
		}

		if (UTextBlock* IngredientTextBlock = Cast<UTextBlock>(RowPanel->GetChildAt(1)))
		{
			IngredientTextBlock->SetText(Ingredient.DisplayName);
		}
		
		++IngredientIndex;
	}
}

void UKCHUDRecipeEntryWidget::RefreshTeamProgressBars(const FKCRecipeViewData& Recipe)
{
	auto ApplyTeamProgress = [](UProgressBar* ProgressBar, float Progress, bool bVisible)
	{
		if (!ProgressBar)
		{
			return;
		}

		if (!bVisible)
		{
			ProgressBar->SetPercent(0.0f);
			ProgressBar->SetVisibility(ESlateVisibility::Hidden);
			return;
		}

		ProgressBar->SetPercent(FMath::Clamp(Progress, 0.0f, 1.0f));
		ProgressBar->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	};

	// Team1ProgressBar is the visual slot for TeamId 0. Team2ProgressBar is for TeamId 1.
	ApplyTeamProgress(Team1ProgressBar.Get(), Recipe.Team0Progress, Recipe.bTeam0ProgressVisible);
	ApplyTeamProgress(Team2ProgressBar.Get(), Recipe.Team1Progress, Recipe.bTeam1ProgressVisible);
}

void UKCHUDRecipeEntryWidget::NativeApplyColorStyle(const UKCColorStyle* InColorStyle)
{
	if (!InColorStyle)
	{
		return;
	}

	if (FoodNameText)
	{
		FoodNameText->SetColorAndOpacity(FSlateColor(InColorStyle->RecipeText));
	}

	if (Team1ProgressBar && InColorStyle->TeamColors.IsValidIndex(0))
	{
		Team1ProgressBar->SetFillColorAndOpacity(InColorStyle->TeamColors[0]);
	}

	if (Team2ProgressBar && InColorStyle->TeamColors.IsValidIndex(1))
	{
		Team2ProgressBar->SetFillColorAndOpacity(InColorStyle->TeamColors[1]);
	}
}
