#include "ProjectKC/UI/HUD/Widget/KCHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "ProjectKC/UI/Common/Style/KCColorStyle.h"
#include "ProjectKC/UI/HUD/ViewModel/KCHUDViewModel.h"
#include "ProjectKC/UI/HUD/Widget/KCHUDPotProgressWidget.h"
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
	HUDViewModel->OnMatchTimerChangedNative.AddUObject(this, &ThisClass::HandleMatchTimerChanged);
	HUDViewModel->OnRecipesChangedNative.AddUObject(this, &ThisClass::HandleRecipesChanged);
	HUDViewModel->OnPotProgressChangedNative.AddUObject(this, &ThisClass::HandlePotProgressChanged);
	HUDViewModel->OnLocalDishRuinedNative.AddUObject(this, &ThisClass::HandleLocalDishRuined);
	RefreshHUD();
}

void UKCHUDWidget::NativeDestruct()
{
	if (HUDViewModel)
	{
		HUDViewModel->OnTeamScoresChangedNative.RemoveAll(this);
		HUDViewModel->OnMatchTimerChangedNative.RemoveAll(this);
		HUDViewModel->OnRecipesChangedNative.RemoveAll(this);
		HUDViewModel->OnPotProgressChangedNative.RemoveAll(this);
		HUDViewModel->OnLocalDishRuinedNative.RemoveAll(this);
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

void UKCHUDWidget::HandlePotProgressChanged(int32 TeamId, const FKCPotProgressViewData& PotProgress)
{
	RefreshPotProgress(TeamId);
}

void UKCHUDWidget::HandleLocalDishRuined()
{
	if (WBP_HUDRecipeList)
	{
		WBP_HUDRecipeList->PlayListShake();
	}
}

void UKCHUDWidget::RefreshHUD()
{
	RefreshScore();
	ApplyMatchTimerText(HUDViewModel ? HUDViewModel->GetRemainingMatchSeconds() : 0);
	RefreshRecipes();
	RefreshPotProgresses();
}

void UKCHUDWidget::RefreshScore()
{
	if (!HUDViewModel)
	{
		return;
	}

	if (!Team1ScoreText || !Team2ScoreText)
		return;
	
	Team1ScoreText->SetText(HUDViewModel->GetTeamScoreText(0));
	Team2ScoreText->SetText(HUDViewModel->GetTeamScoreText(1));
	
}

void UKCHUDWidget::HandleMatchTimerChanged(int32 RemainingSeconds)
{
	ApplyMatchTimerText(RemainingSeconds);
}

void UKCHUDWidget::ApplyMatchTimerText(int32 RemainingSeconds)
{
	UTextBlock* TimerTextBlock = ResolveTimerTextBlock();
	if (!TimerTextBlock)
	{
		return;
	}

	(void)RemainingSeconds;
	TimerTextBlock->SetText(HUDViewModel
		? HUDViewModel->GetRemainingMatchTimeText()
		: FText::GetEmpty());
}

UTextBlock* UKCHUDWidget::ResolveTimerTextBlock()
{
	if (Timer)
	{
		return Timer.Get();
	}

	if (WidgetTree)
	{
		Timer = WidgetTree->FindWidget<UTextBlock>(TEXT("Timer"));
	}

	return Timer.Get();
}

void UKCHUDWidget::RefreshRecipes()
{
	if (!HUDViewModel)
	{
		return;
	}

	if (WBP_HUDRecipeList)
	{
		WBP_HUDRecipeList->SetRecipeViewModels(HUDViewModel->GetRecipeViewModels());
	}
}

void UKCHUDWidget::RefreshPotProgresses()
{
	RefreshPotProgress(0);
	RefreshPotProgress(1);
}

void UKCHUDWidget::RefreshPotProgress(int32 TeamId)
{
	if (!HUDViewModel)
	{
		return;
	}

	UKCHUDPotProgressWidget* PotProgressWidget = nullptr;
	if (TeamId == 0)
	{
		PotProgressWidget = WBP_Team1PotProgress.Get();
	}
	else if (TeamId == 1)
	{
		PotProgressWidget = WBP_Team2PotProgress.Get();
	}

	if (PotProgressWidget)
	{
		PotProgressWidget->SetPotProgress(HUDViewModel->GetPotProgress(TeamId));
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
