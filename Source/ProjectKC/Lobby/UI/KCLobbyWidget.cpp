/**
 * @file KCLobbyWidget.cpp
 * @brief UKCLobbyWidget 구현부
 */

#include "ProjectKC/Lobby/UI/KCLobbyWidget.h"
#include "ProjectKC/Lobby/UI/KCCustomizationWidget.h"
#include "ProjectKC/Lobby/UI/KCFriendListWidget.h"
#include "ProjectKC/Lobby/KCLobbyPlayerController.h"
#include "ProjectKC/Player/KCPlayerState.h"
#include "ProjectKC/GameSystem/KCLobbyGameMode.h"
#include "ProjectKC/ProjectKC.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Animation/WidgetAnimation.h"
#include "Kismet/KismetSystemLibrary.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "KCLobbyWidget"

void UKCLobbyWidget::NativeConstruct()
{
	Super::NativeConstruct();

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
		CustomizationWidgetClass = StaticLoadClass(
			UKCCustomizationWidget::StaticClass(),
			nullptr,
			TEXT("/Game/KC/SteamLobbySystem/Blueprints/UI/WBP_Customization.WBP_Customization_C"));
	}
	if (!CustomizationWidgetClass)
	{
		LobbyPlayerController->CancelCustomizationEditing();
		UE_LOG(LogKCLobby, Error,
			TEXT("[KCLobbyWidget] WBP_Customization was not found. Create it at the default path or set CustomizationWidgetClass."));
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

void UKCLobbyWidget::OnReadyStatusUpdated(bool bIsReady)
{
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

#undef LOCTEXT_NAMESPACE
