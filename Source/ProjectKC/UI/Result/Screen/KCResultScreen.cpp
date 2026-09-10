#include "ProjectKC/UI/Result/Screen/KCResultScreen.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "GameSystem/KCGameState.h"
#include "Player/KCPlayerController.h"
#include "ProjectKC/UI/Common/Style/KCColorStyle.h"
#include "ProjectKC/UI/Result/ViewModel/KCResultViewModel.h"

void UKCResultScreen::NativeConstruct()
{
	Super::NativeConstruct();

	SetIsFocusable(true);
	SetKeyboardFocus();

	if (BackToLobbyButton)
	{
		BackToLobbyButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleBackToLobbyButtonClicked);
	}

	if (!ResultViewModel)
	{
		ResultViewModel = NewObject<UKCResultViewModel>(this);
	}

	RefreshResultScreen();
	if (ShowResult)
	{
		PlayAnimation(ShowResult);
	}
	StartBackToLobbyTimer();
}

void UKCResultScreen::NativeDestruct()
{
	if (BackToLobbyButton)
	{
		BackToLobbyButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleBackToLobbyButtonClicked);
	}

	StopBackToLobbyTimer();

	Super::NativeDestruct();
}

void UKCResultScreen::HandleBackToLobbyButtonClicked()
{
	if (!ResultViewModel)
	{
		ResultViewModel = NewObject<UKCResultViewModel>(this);
	}

	if (BackToLobbyButton)
	{
		BackToLobbyButton->SetIsEnabled(false);
	}

	if (AKCPlayerController* PlayerController = Cast<AKCPlayerController>(GetOwningPlayer()))
	{
		PlayerController->RequestSkipResultScreen();
	}
}

void UKCResultScreen::NativeApplyColorStyle(const UKCColorStyle* InColorStyle)
{
	Super::NativeApplyColorStyle(InColorStyle);

	ApplyWinningTeamColor();
}

void UKCResultScreen::RefreshResultScreen()
{
	UpdateWinningTeam();
	UpdateBackToLobbyTimer();
}

void UKCResultScreen::StartBackToLobbyTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			BackToLobbyTimerHandle,
			this,
			&ThisClass::UpdateBackToLobbyTimer,
			0.2f,
			true);
	}
}

void UKCResultScreen::StopBackToLobbyTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BackToLobbyTimerHandle);
	}
}

void UKCResultScreen::UpdateBackToLobbyTimer()
{
	const UWorld* World = GetWorld();
	const AKCGameState* GameState = World ? World->GetGameState<AKCGameState>() : nullptr;
	const AKCPlayerController* PlayerController = Cast<AKCPlayerController>(GetOwningPlayer());
	const int32 RemainingSeconds = GameState && PlayerController
		? GameState->GetRemainingResultScreenSeconds(PlayerController->GetServerTime())
		: 0;

	if (!ResultViewModel)
	{
		ResultViewModel = NewObject<UKCResultViewModel>(this);
	}

	ResultViewModel->SetRemainingBackToLobbySeconds(RemainingSeconds);
	ApplySecondCountText();
	UpdateWinningTeam();
}

void UKCResultScreen::ApplySecondCountText()
{
	if (SecondCountText && ResultViewModel)
	{
		SecondCountText->SetText(ResultViewModel->GetRemainingBackToLobbySecondText());
	}
}

void UKCResultScreen::UpdateWinningTeam()
{
	if (!ResultViewModel)
	{
		ResultViewModel = NewObject<UKCResultViewModel>(this);
	}

	const UWorld* World = GetWorld();
	const AKCGameState* GameState = World ? World->GetGameState<AKCGameState>() : nullptr;
	ResultViewModel->SetWinningTeamId(GameState ? GameState->GetWinningTeamId() : INDEX_NONE);
	ApplyWinningTeamText();
}

void UKCResultScreen::ApplyWinningTeamText()
{
	if (TeamText && ResultViewModel)
	{
		TeamText->SetText(ResultViewModel->GetWinningTeamText());
		ApplyWinningTeamColor();
	}
}

void UKCResultScreen::ApplyWinningTeamColor()
{
	if (!TeamText || !ResultViewModel)
	{
		return;
	}

	const int32 WinningTeamId = ResultViewModel->GetWinningTeamId();
	const UKCColorStyle* CurrentColorStyle = GetColorStyle();
	if (WinningTeamId != INDEX_NONE && CurrentColorStyle && CurrentColorStyle->TeamColors.IsValidIndex(WinningTeamId))
	{
		TeamText->SetColorAndOpacity(FSlateColor(CurrentColorStyle->TeamColors[WinningTeamId]));
		return;
	}

	TeamText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
}
