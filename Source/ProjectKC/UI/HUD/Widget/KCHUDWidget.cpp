#include "ProjectKC/UI/HUD/Widget/KCHUDWidget.h"

#include "Animation/WidgetAnimation.h"
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
	HUDViewModel->OnTeamScoreAddedNative.AddUObject(this, &ThisClass::HandleTeamScoreAdded);
	HUDViewModel->OnMatchTimerChangedNative.AddUObject(this, &ThisClass::HandleMatchTimerChanged);
	HUDViewModel->OnRecipesChangedNative.AddUObject(this, &ThisClass::HandleRecipesChanged);
	HUDViewModel->OnPotProgressChangedNative.AddUObject(this, &ThisClass::HandlePotProgressChanged);
	HUDViewModel->OnLocalDishRuinedNative.AddUObject(this, &ThisClass::HandleLocalDishRuined);

	if (Team1ScoreUp)
	{
		FWidgetAnimationDynamicEvent FinishedEvent;
		FinishedEvent.BindDynamic(this, &ThisClass::HandleTeam1ScoreUpFinished);
		BindToAnimationFinished(Team1ScoreUp, FinishedEvent);
	}

	if (Team2ScoreUp)
	{
		FWidgetAnimationDynamicEvent FinishedEvent;
		FinishedEvent.BindDynamic(this, &ThisClass::HandleTeam2ScoreUpFinished);
		BindToAnimationFinished(Team2ScoreUp, FinishedEvent);
	}

	if (WBP_Team1PotProgress)
	{
		WBP_Team1PotProgress->SetTeamId(0);
	}

	if (WBP_Team2PotProgress)
	{
		WBP_Team2PotProgress->SetTeamId(1);
	}

	RefreshHUD();
}

void UKCHUDWidget::NativeDestruct()
{
	if (HUDViewModel)
	{
		HUDViewModel->OnTeamScoresChangedNative.RemoveAll(this);
		HUDViewModel->OnTeamScoreAddedNative.RemoveAll(this);
		HUDViewModel->OnMatchTimerChangedNative.RemoveAll(this);
		HUDViewModel->OnRecipesChangedNative.RemoveAll(this);
		HUDViewModel->OnPotProgressChangedNative.RemoveAll(this);
		HUDViewModel->OnLocalDishRuinedNative.RemoveAll(this);

		if (Team1ScoreUp)
		{
			UnbindAllFromAnimationFinished(Team1ScoreUp);
		}

		if (Team2ScoreUp)
		{
			UnbindAllFromAnimationFinished(Team2ScoreUp);
		}

		HUDViewModel->StopListening();
	}

	Super::NativeDestruct();
}

void UKCHUDWidget::HandleTeamScoresChanged(const TArray<int32>& TeamScores)
{
	RefreshScore();
}

void UKCHUDWidget::HandleTeamScoreAdded(int32 TeamId, int32 AddedScore)
{
	PlayTeamScoreUp(TeamId, AddedScore);
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

void UKCHUDWidget::PlayTeamScoreUp(int32 TeamId, int32 AddedScore)
{
	if (AddedScore <= 0)
	{
		return;
	}

	UTextBlock* PlusText = nullptr;
	UWidgetAnimation* ScoreUpAnimation = nullptr;
	if (TeamId == 0)
	{
		PlusText = Team1PlusText.Get();
		ScoreUpAnimation = Team1ScoreUp.Get();
	}
	else if (TeamId == 1)
	{
		PlusText = Team2PlusText.Get();
		ScoreUpAnimation = Team2ScoreUp.Get();
	}

	if (!PlusText)
	{
		return;
	}

	PlusText->SetText(FText::FromString(FString::Printf(TEXT("+%d"), AddedScore)));
	PlusText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (ScoreUpAnimation)
	{
		StopAnimation(ScoreUpAnimation);
		PlayAnimation(ScoreUpAnimation);
	}
}

void UKCHUDWidget::HandleTeam1ScoreUpFinished()
{
	if (Team1PlusText)
	{
		Team1PlusText->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UKCHUDWidget::HandleTeam2ScoreUpFinished()
{
	if (Team2PlusText)
	{
		Team2PlusText->SetVisibility(ESlateVisibility::Hidden);
	}
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
	Super::NativeApplyColorStyle(InColorStyle);

	if (!InColorStyle)
	{
		return;
	}

	if (Team1ScoreText && InColorStyle->TeamColors.IsValidIndex(0))
	{
		Team1ScoreText->SetColorAndOpacity(FSlateColor(InColorStyle->TeamColors[0]));
	}

	if (Team1PlusText && InColorStyle->TeamColors.IsValidIndex(0))
	{
		Team1PlusText->SetColorAndOpacity(FSlateColor(InColorStyle->TeamColors[0]));
	}

	if (Team2ScoreText && InColorStyle->TeamColors.IsValidIndex(1))
	{
		Team2ScoreText->SetColorAndOpacity(FSlateColor(InColorStyle->TeamColors[1]));
	}

	if (Team2PlusText && InColorStyle->TeamColors.IsValidIndex(1))
	{
		Team2PlusText->SetColorAndOpacity(FSlateColor(InColorStyle->TeamColors[1]));
	}
}
