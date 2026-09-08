#include "ProjectKC/UI/Result/Screen/KCResultScreen.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "GameSystem/KCGameState.h"
#include "Player/KCPlayerController.h"
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
	if (AKCPlayerController* PlayerController = Cast<AKCPlayerController>(GetOwningPlayer()))
	{
		PlayerController->RequestSkipResultScreen();
	}
}

void UKCResultScreen::RefreshResultScreen()
{
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
	ApplyBackToLobbyText();
}

void UKCResultScreen::ApplyBackToLobbyText()
{
	if (BackToLobbySecondText && ResultViewModel)
	{
		BackToLobbySecondText->SetText(ResultViewModel->GetRemainingBackToLobbyText());
	}
}
