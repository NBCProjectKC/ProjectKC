/**
 * @file KCLobbyWidget.cpp
 * @brief UKCLobbyWidget 구현부
 */

#include "ProjectKC/Lobby/UI/KCLobbyWidget.h"
#include "ProjectKC/Lobby/UI/KCCustomizationWidget.h"
#include "ProjectKC/Lobby/UI/KCFriendListWidget.h"
#include "ProjectKC/Lobby/UI/KCLobbyGameSettingsWidget.h"
#include "ProjectKC/Lobby/KCLobbyPlayerController.h"
#include "ProjectKC/Player/KCPlayerState.h"
#include "ProjectKC/GameSystem/KCLobbyGameMode.h"
#include "ProjectKC/Lobby/KCSessionSubsystem.h"
#include "UI/Common/Core/KCUISettings.h"
#include "ProjectKC/ProjectKC.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "Animation/WidgetAnimation.h"
#include "Kismet/KismetSystemLibrary.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "KCLobbyWidget"

void UKCLobbyWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);

	UE_LOG(LogKCLobby, Log, TEXT("[KCLobbyWidget] NativeConstruct"));

	if (Button_Socials)
	{
		Button_Socials->OnClicked.AddDynamic(this, &UKCLobbyWidget::OnSocialsClicked);
	}

	if (Button_Ready)
	{
		Button_Ready->OnClicked.AddDynamic(this, &UKCLobbyWidget::OnReadyClicked);
	}

	if (Button_StartGame)
	{
		Button_StartGame->OnClicked.AddDynamic(this, &UKCLobbyWidget::OnStartGameClicked);
		const bool bIsServer = UKismetSystemLibrary::IsServer(this);
		Button_StartGame->SetVisibility(bIsServer ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		Button_StartGame->SetIsEnabled(false);
	}

	if (Button_GameSettings)
	{
		Button_GameSettings->OnClicked.AddDynamic(this, &UKCLobbyWidget::OnGameSettingsClicked);
		const bool bIsServer = UKismetSystemLibrary::IsServer(this);
		Button_GameSettings->SetVisibility(bIsServer ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		Button_GameSettings->SetIsEnabled(true);
	}

	if (Button_Customization)
	{
		Button_Customization->OnClicked.AddUniqueDynamic(
			this, &UKCLobbyWidget::OnCustomizationClicked);
	}

	// 1. 즉시 PlayerState 바인딩 시도
	if (!TryBindPlayerState())
	{
		// 2. 네트워크 복제 딜레이 대비 0.05초 간격 경량 타이머로 바인딩 재시도
		if (UWorld* World = GetWorld())
		{
			BindRetryCount = 0;
			World->GetTimerManager().SetTimer(
				PlayerStateBindRetryTimerHandle,
				[this]()
				{
					BindRetryCount++;
					if (TryBindPlayerState() || BindRetryCount >= 200)
					{
						if (UWorld* CurrentWorld = GetWorld())
						{
							CurrentWorld->GetTimerManager().ClearTimer(PlayerStateBindRetryTimerHandle);
						}
					}
				},
				0.05f,
				true
			);
		}
	}
}

void UKCLobbyWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PlayerStateBindRetryTimerHandle);
	}
	if (Button_GameSettings)
	{
		Button_GameSettings->OnClicked.RemoveDynamic(
			this, &UKCLobbyWidget::OnGameSettingsClicked);
	}
	if (GameSettingsWidgetInstance)
	{
		GameSettingsWidgetInstance->CloseSettings();
		GameSettingsWidgetInstance = nullptr;
	}
	if (Button_Customization)
	{
		Button_Customization->OnClicked.RemoveDynamic(
			this, &UKCLobbyWidget::OnCustomizationClicked);
	}
	if (CustomizationWidgetInstance)
	{
		CustomizationWidgetInstance->RequestCancelAndClose();
		CustomizationWidgetInstance = nullptr;
	}

	Super::NativeDestruct();
}

void UKCLobbyWidget::NotifyCustomizationWidgetClosed(
	UKCCustomizationWidget* ClosedWidget)
{
	if (CustomizationWidgetInstance == ClosedWidget)
	{
		CustomizationWidgetInstance = nullptr;
	}
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

bool UKCLobbyWidget::TryBindPlayerState()
{
	if (bPlayerStateBound)
	{
		return true;
	}

	if (APlayerController* PC = GetOwningPlayer())
	{
		if (AKCPlayerState* PS = PC->GetPlayerState<AKCPlayerState>())
		{
			PS->OnReadyStatusChanged.AddUniqueDynamic(this, &UKCLobbyWidget::OnReadyStatusUpdated);
			PS->OnTeamIdChanged.AddUniqueDynamic(this, &UKCLobbyWidget::OnTeamIdUpdated);
			OnReadyStatusUpdated(PS->IsReady());

			if (PS->GetTeamId() != INDEX_NONE)
			{
				OnTeamIdUpdated(PS->GetTeamId());
			}

			bPlayerStateBound = true;
			UE_LOG(LogKCLobby, Log, TEXT("[KCLobbyWidget] Successfully bound to PlayerState '%s' (TeamId: %d)"),
				*PS->GetPlayerName(), PS->GetTeamId());
			return true;
		}
	}

	return false;
}

void UKCLobbyWidget::PlayMatchStartAnim()
{
	if (CustomizationWidgetInstance)
	{
		CustomizationWidgetInstance->RequestCancelAndClose();
	}

	if (MatchStartAnim)
	{
		PlayAnimation(MatchStartAnim);
	}

	SetIsEnabled(false);
}

void UKCLobbyWidget::SetStartGameButtonEnabled(bool bEnabled)
{
	if (Button_StartGame)
	{
		Button_StartGame->SetIsEnabled(bEnabled);
		UE_LOG(LogKCLobby, Log, TEXT("[KCLobbyWidget] SetStartGameButtonEnabled: %s"), bEnabled ? TEXT("TRUE") : TEXT("FALSE"));
	}
}

void UKCLobbyWidget::OnSocialsClicked()
{
	if (WBP_FriendList)
	{
		const ESlateVisibility CurrentVisibility = WBP_FriendList->GetVisibility();
		const ESlateVisibility NewVisibility = (CurrentVisibility == ESlateVisibility::Visible)
			? ESlateVisibility::Collapsed
			: ESlateVisibility::Visible;

		WBP_FriendList->SetVisibility(NewVisibility);
		UE_LOG(LogKCLobby, Verbose, TEXT("[KCLobbyWidget] OnSocialsClicked: Toggled FriendList visibility to %s"),
			NewVisibility == ESlateVisibility::Visible ? TEXT("Visible") : TEXT("Collapsed"));
	}
}

void UKCLobbyWidget::OnReadyClicked()
{
	if (AKCLobbyPlayerController* PC = Cast<AKCLobbyPlayerController>(GetOwningPlayer()))
	{
		PC->ROS_ToggleReadyStatus();
	}
}

void UKCLobbyWidget::OnStartGameClicked()
{
	UE_LOG(LogKCLobby, Log, TEXT("[KCLobbyWidget] OnStartGameClicked -> Triggering GameMode StartGame"));
	if (UWorld* World = GetWorld())
	{
		if (AKCLobbyGameMode* GM = World->GetAuthGameMode<AKCLobbyGameMode>())
		{
			GM->StartGame();
		}
	}
}

void UKCLobbyWidget::OnCustomizationClicked()
{
	if (CustomizationWidgetInstance &&
		CustomizationWidgetInstance->IsInViewport())
	{
		return;
	}

	AKCLobbyPlayerController* LobbyPlayerController =
		Cast<AKCLobbyPlayerController>(GetOwningPlayer());
	if (!LobbyPlayerController ||
		!LobbyPlayerController->BeginCustomizationEditing())
	{
		UE_LOG(LogKCLobby, Warning,
			TEXT("[KCLobbyWidget] Failed to begin customization editing."));
		return;
	}

	if (!CustomizationWidgetClass)
	{
		if (const UKCUISettings* UISettings = GetDefault<UKCUISettings>())
		{
			CustomizationWidgetClass =
				UISettings->CustomizationWidgetClass.LoadSynchronous();
		}
	}
	if (!CustomizationWidgetClass)
	{
		LobbyPlayerController->CancelCustomizationEditing();
		UE_LOG(LogKCLobby, Error,
			TEXT("[KCLobbyWidget] CustomizationWidgetClass is not set in WBP_LobbyUI or KCUISettings."));
		return;
	}

	CustomizationWidgetInstance =
		CreateWidget<UKCCustomizationWidget>(
			LobbyPlayerController,
			CustomizationWidgetClass);
	if (!CustomizationWidgetInstance)
	{
		LobbyPlayerController->CancelCustomizationEditing();
		UE_LOG(LogKCLobby, Error,
			TEXT("[KCLobbyWidget] Failed to create WBP_Customization."));
		return;
	}

	CustomizationWidgetInstance->InitializeLobbyWidget(this);
	CustomizationWidgetInstance->AddToViewport(20);
	SetVisibility(ESlateVisibility::Collapsed);
}

void UKCLobbyWidget::OnGameSettingsClicked()
{
	BP_OnGameSettingsClicked();

	if (GameSettingsWidgetInstance && GameSettingsWidgetInstance->IsInViewport())
	{
		return;
	}

	AKCLobbyPlayerController* LobbyPC = Cast<AKCLobbyPlayerController>(GetOwningPlayer());
	if (!LobbyPC)
	{
		return;
	}

	if (!LobbyGameSettingsWidgetClass)
	{
		if (const UKCUISettings* UISettings = GetDefault<UKCUISettings>())
		{
			LobbyGameSettingsWidgetClass = UISettings->LobbyGameSettingsWidgetClass.LoadSynchronous();
		}
	}

	if (!LobbyGameSettingsWidgetClass)
	{
		UE_LOG(LogKCLobby, Warning, TEXT("[KCLobbyWidget] LobbyGameSettingsWidgetClass is not set in WBP_LobbyUI or KCUISettings."));
		return;
	}

	GameSettingsWidgetInstance = CreateWidget<UKCLobbyGameSettingsWidget>(LobbyPC, LobbyGameSettingsWidgetClass);
	if (!GameSettingsWidgetInstance)
	{
		UE_LOG(LogKCLobby, Error, TEXT("[KCLobbyWidget] Failed to create WBP_GameSettings instance."));
		return;
	}

	int32 CurrentCount = 6;
	EKCLevelType CurrentMap = EKCLevelType::PortableGasStove;
	float CurrentDuration = 300.0f;

	if (const UWorld* World = GetWorld())
	{
		if (const AKCLobbyGameMode* GM = World->GetAuthGameMode<AKCLobbyGameMode>())
		{
			CurrentCount = GM->GetRequiredPlayerCount();
			CurrentMap = GM->GetSelectedMap();
			CurrentDuration = GM->GetMatchDuration();
		}
		else if (const UGameInstance* GI = World->GetGameInstance())
		{
			if (const UKCSessionSubsystem* SessionSub = GI->GetSubsystem<UKCSessionSubsystem>())
			{
				CurrentCount = SessionSub->GetExpectedPlayerCount() > 0 ? SessionSub->GetExpectedPlayerCount() : 6;
				CurrentMap = SessionSub->GetSelectedMapType();
				CurrentDuration = SessionSub->GetMatchDurationSeconds() > 0.0f ? SessionSub->GetMatchDurationSeconds() : 300.0f;
			}
		}
	}

	GameSettingsWidgetInstance->InitializeGameSettings(this, CurrentCount, CurrentMap, CurrentDuration);
	GameSettingsWidgetInstance->AddToViewport(25);
}

void UKCLobbyWidget::NotifyGameSettingsWidgetClosed(UKCLobbyGameSettingsWidget* ClosedWidget)
{
	if (GameSettingsWidgetInstance == ClosedWidget)
	{
		GameSettingsWidgetInstance = nullptr;
	}
}

void UKCLobbyWidget::OnReadyStatusUpdated(bool bIsReady)
{
	if (Button_GameSettings)
	{
		Button_GameSettings->SetIsEnabled(!bIsReady);
	}
	if (Button_Customization)
	{
		Button_Customization->SetIsEnabled(!bIsReady);
	}
	if (Text_Ready)
	{
		// 준비 완료 시: CANCEL READY (준비 취소), 미준비 시: READY (준비하기)
		Text_Ready->SetText(bIsReady ? LOCTEXT("CancelReadyText", "CANCEL READY") : LOCTEXT("ReadyText", "READY"));
		UE_LOG(LogKCLobby, Verbose, TEXT("[KCLobbyWidget] OnReadyStatusUpdated UI Text set to: %s"),
			bIsReady ? TEXT("CANCEL READY") : TEXT("READY"));
	}
}

void UKCLobbyWidget::OnTeamIdUpdated(int32 NewTeamId)
{
	if (Text_TeamName)
	{
		if (NewTeamId == 0)
		{
			Text_TeamName->SetText(LOCTEXT("TeamRed", "TEAM RED"));
		}
		else if (NewTeamId == 1)
		{
			Text_TeamName->SetText(LOCTEXT("TeamBlue", "TEAM BLUE"));
		}
		else
		{
			Text_TeamName->SetText(LOCTEXT("TeamAssigning", "ASSIGNING..."));
		}
		UE_LOG(LogKCLobby, Log, TEXT("[KCLobbyWidget] OnTeamIdUpdated UI Text set to: Team %d"), NewTeamId);
	}
}

void UKCLobbyWidget::FocusChatInput()
{
	if ((CustomizationWidgetInstance && CustomizationWidgetInstance->IsInViewport()) ||
		(GameSettingsWidgetInstance && GameSettingsWidgetInstance->IsInViewport()))
	{
		return;
	}

	if (WBP_ChatTest)
	{
		if (UWidget* InputWidget = WBP_ChatTest->GetWidgetFromName(TEXT("ChatInputBox")))
		{
			InputWidget->SetFocus();
		}
	}
}

void UKCLobbyWidget::ResetFocusToGame()
{
	if (AKCLobbyPlayerController* LobbyPC = Cast<AKCLobbyPlayerController>(GetOwningPlayer()))
	{
		LobbyPC->ResetFocusToGame();
	}
}

void UKCLobbyWidget::LeaveLobby()
{
	if (AKCLobbyPlayerController* LobbyPC = Cast<AKCLobbyPlayerController>(GetOwningPlayer()))
	{
		LobbyPC->LeaveLobby();
	}
}

#undef LOCTEXT_NAMESPACE
