#include "ProjectKC/UI/HUD/Widget/KCHUDRecipeEntryWidget.h"

#include "Components/Border.h"
#include "Components/CheckBox.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/ListView.h"
#include "Components/PanelWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Components/VerticalBox.h"
#include "ProjectKC/UI/Common/Style/KCColorStyle.h"
#include "ProjectKC/UI/HUD/ViewModel/KCHUDRecipeViewModel.h"
#include "ProjectKC/UI/HUD/Widget/KCRecipeEntryIngredient.h"

void UKCHUDRecipeEntryWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

}

void UKCHUDRecipeEntryWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	if (UKCHUDRecipeViewModel* InRecipeViewModel = Cast<UKCHUDRecipeViewModel>(ListItemObject))
	{
		SetRecipeViewModel(InRecipeViewModel);
	}
}

void UKCHUDRecipeEntryWidget::NativeOnEntryReleased()
{
	UnbindRecipeViewModel();
}

void UKCHUDRecipeEntryWidget::SetRecipeViewModel(UKCHUDRecipeViewModel* InRecipeViewModel)
{
	BindRecipeViewModel(InRecipeViewModel);
	SetRecipe(InRecipeViewModel ? InRecipeViewModel->GetRecipe() : FKCRecipeViewData());
}

void UKCHUDRecipeEntryWidget::SetRecipe(const FKCRecipeViewData& Recipe)
{
	BP_OnRecipeSet(Recipe);

	if (LV_Ingredients)
	{
		LV_Ingredients->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	if (VB_TestIngredients)
	{
		VB_TestIngredients->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (FoodNameText)
	{
		FoodNameText->SetText(Recipe.DisplayName.IsEmpty() ? FText::FromName(Recipe.RecipeRowName) : Recipe.DisplayName);
	}

	RefreshDifficultyStars(Recipe.DifficultyStars);
	RefreshCookingIndicators(Recipe);
	RefreshTeamProgressBars(Recipe);
	RefreshIngredientList(Recipe.Ingredients);
}

void UKCHUDRecipeEntryWidget::BindRecipeViewModel(UKCHUDRecipeViewModel* InRecipeViewModel)
{
	if (RecipeViewModel.Get() == InRecipeViewModel)
	{
		return;
	}

	UnbindRecipeViewModel();
	RecipeViewModel = InRecipeViewModel;

	if (InRecipeViewModel)
	{
		RecipeChangedHandle = InRecipeViewModel->OnRecipeChangedNative.AddUObject(
			this,
			&ThisClass::HandleRecipeViewModelChanged);
	}
}

void UKCHUDRecipeEntryWidget::UnbindRecipeViewModel()
{
	if (UKCHUDRecipeViewModel* BoundRecipeViewModel = RecipeViewModel.Get())
	{
		BoundRecipeViewModel->OnRecipeChangedNative.Remove(RecipeChangedHandle);
	}

	RecipeViewModel.Reset();
	RecipeChangedHandle.Reset();
}

void UKCHUDRecipeEntryWidget::HandleRecipeViewModelChanged(const FKCRecipeViewData& Recipe)
{
	SetRecipe(Recipe);
}

void UKCHUDRecipeEntryWidget::RefreshDifficultyStars(const int32 DifficultyStars)
{
	if (!HB_Stars)
	{
		return;
	}

	int32 StarCount = 0;
	for (int32 ChildIndex = 0; ChildIndex < HB_Stars->GetChildrenCount(); ++ChildIndex)
	{
		if (Cast<UImage>(HB_Stars->GetChildAt(ChildIndex)))
		{
			++StarCount;
		}
	}

	const int32 FilledStarCount = FMath::Clamp(DifficultyStars, 0, StarCount);
	int32 StarIndex = 0;
	for (int32 ChildIndex = 0; ChildIndex < HB_Stars->GetChildrenCount(); ++ChildIndex)
	{
		UImage* StarImage = Cast<UImage>(HB_Stars->GetChildAt(ChildIndex));
		if (!StarImage)
		{
			continue;
		}

		StarImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		UTexture2D* StarTexture = StarIndex < FilledStarCount ? FilledStarTexture.Get() : EmptyStarTexture.Get();
		if (StarTexture)
		{
			StarImage->SetBrushFromTexture(StarTexture, false);
		}

		++StarIndex;
	}
}

void UKCHUDRecipeEntryWidget::RefreshCookingIndicators(const FKCRecipeViewData& Recipe)
{
	ApplyCookingIndicator(Team1CookingBorder.Get(), Team1CookingImage.Get(), Team1Cooking.Get(), Recipe.bTeam0Cooking);
	ApplyCookingIndicator(Team2CookingBorder.Get(), Team2CookingImage.Get(), Team2Cooking.Get(), Recipe.bTeam1Cooking);
}

void UKCHUDRecipeEntryWidget::ApplyCookingIndicator(UBorder* CookingBorder, UImage* CookingImage, UWidgetAnimation* CookingAnimation, const bool bCooking)
{
	if (CookingBorder)
	{
		// CookingImage->SetVisibility(bCooking
		// 	? ESlateVisibility::SelfHitTestInvisible
		// 	: ESlateVisibility::Hidden);
		CookingBorder->SetVisibility(bCooking
			? ESlateVisibility::SelfHitTestInvisible
			: ESlateVisibility::Hidden);
	}

	if (!CookingAnimation)
	{
		return;
	}

	if (bCooking)
	{
		if (!IsAnimationPlaying(CookingAnimation))
		{
			PlayAnimation(CookingAnimation, 0.0f, 0);
		}
	}
	else if (IsAnimationPlaying(CookingAnimation))
	{
		StopAnimation(CookingAnimation);
	}
}

void UKCHUDRecipeEntryWidget::RefreshIngredientList(const TArray<FKCRecipeIngredientViewData>& Ingredients)
{
	if (!LV_Ingredients)
	{
		return;
	}

	if (CanReuseIngredientItems(Ingredients))
	{
		for (int32 IngredientIndex = 0; IngredientIndex < Ingredients.Num(); ++IngredientIndex)
		{
			IngredientItems[IngredientIndex]->SetIngredient(Ingredients[IngredientIndex]);
		}

		return;
	}

	IngredientItems.Reset();
	IngredientItems.Reserve(Ingredients.Num());

	TArray<UObject*> ListItems;
	ListItems.Reserve(Ingredients.Num());

	for (const FKCRecipeIngredientViewData& Ingredient : Ingredients)
	{
		UKCRecipeIngredientListItem* IngredientItem = NewObject<UKCRecipeIngredientListItem>(this);
		if (!IngredientItem)
		{
			continue;
		}

		IngredientItem->SetIngredient(Ingredient);
		IngredientItems.Add(IngredientItem);
		ListItems.Add(IngredientItem);
	}

	LV_Ingredients->SetListItems(ListItems);
}

bool UKCHUDRecipeEntryWidget::CanReuseIngredientItems(const TArray<FKCRecipeIngredientViewData>& Ingredients) const
{
	if (IngredientItems.Num() != Ingredients.Num())
	{
		return false;
	}

	for (int32 IngredientIndex = 0; IngredientIndex < Ingredients.Num(); ++IngredientIndex)
	{
		const UKCRecipeIngredientListItem* IngredientItem = IngredientItems[IngredientIndex];
		if (!IngredientItem || IngredientItem->Ingredient.IngredientId != Ingredients[IngredientIndex].IngredientId)
		{
			return false;
		}
	}

	return true;
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
		// FoodNameText->SetColorAndOpacity(FSlateColor(InColorStyle->RecipeText));
	}

	if (Team1CookingImage && InColorStyle->TeamColors.IsValidIndex(0))
	{
		Team1CookingImage->SetColorAndOpacity(InColorStyle->TeamColors[0]);
	}

	if (Team1CookingBorder && InColorStyle->TeamColors.IsValidIndex(0))
	{
		Team1CookingBorder->SetBrushColor(InColorStyle->TeamColors[0]);
	}

	if (Team2CookingImage && InColorStyle->TeamColors.IsValidIndex(1))
	{
		Team2CookingImage->SetColorAndOpacity(InColorStyle->TeamColors[1]);
	}

	if (Team2CookingBorder && InColorStyle->TeamColors.IsValidIndex(1))
	{
		Team2CookingBorder->SetBrushColor(InColorStyle->TeamColors[1]);
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
