#include "ProjectKC/UI/HUD/Widget/KCHUDWidget.h"

#include <string>

#include "Components/TextBlock.h"
#include "ProjectKC/UI/Common/Style/KCColorStyle.h"
#include "ProjectKC/UI/HUD/ViewModel/KCHUDViewModel.h"
#include "ProjectKC/UI/HUD/Widget/KCHUDRecipeListWidget.h"

void UKCHUDWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
}

void UKCHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!HUDViewModel)
	{
		HUDViewModel = NewObject<UKCHUDViewModel>(this);
	}

	HUDViewModel->StartListening(this);
	HUDViewModel->OnTeamScoresChangedNative.AddUObject(this, &ThisClass::HandleTeamScoresChanged);
	HUDViewModel->OnRecipesChangedNative.AddUObject(this, &ThisClass::HandleRecipesChanged);
	RefreshRoughHUD();
}

void UKCHUDWidget::NativeDestruct()
{
	if (HUDViewModel)
	{
		HUDViewModel->OnTeamScoresChangedNative.RemoveAll(this);
		HUDViewModel->OnRecipesChangedNative.RemoveAll(this);
		HUDViewModel->StopListening();
	}

	Super::NativeDestruct();
}

void UKCHUDWidget::HandleTeamScoresChanged(const TArray<int32>& TeamScores)
{
	RefreshScore();
}

void UKCHUDWidget::HandleRecipesChanged()
{
	RefreshRecipes();
}

void UKCHUDWidget::RefreshRoughHUD()
{
	RefreshScore();
	RefreshRecipes();
}

void UKCHUDWidget::RefreshScore()
{
	if (!HUDViewModel)
	{
		return;
	}

	const int32 LeftScore = HUDViewModel->GetTeamScore(0);
	const int32 RightScore = HUDViewModel->GetTeamScore(1);
	
	Team1ScoreText->SetText(FText::AsNumber(LeftScore));
	Team2ScoreText->SetText(FText::AsNumber(RightScore));
	
}

void UKCHUDWidget::RefreshRecipes()
{
	if (!HUDViewModel)
	{
		return;
	}

	const TArray<FKCRecipeViewData>& Recipes = HUDViewModel->GetRecipes();
	if (WBP_HUDRecipeList)
	{
		WBP_HUDRecipeList->SetRecipes(Recipes);
	}
}

void UKCHUDWidget::NativeApplyColorStyle(const UKCColorStyle* InColorStyle)
{
	// if (!HUDScoreText || !InColorStyle)
	// {
	// 	return;
	// }

	// HUDScoreText->SetColorAndOpacity(FSlateColor(InColorStyle->ScoreText));
}
