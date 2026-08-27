/**
 * @file KCLobbyPlayerController.cpp
 * @brief AKCLobbyPlayerController 구현부
 */

#include "ProjectKC/Lobby/KCLobbyPlayerController.h"
#include "ProjectKC/Lobby/UI/KCLobbyWidget.h"
#include "ProjectKC/Lobby/KCLobbyPlayerState.h"
#include "ProjectKC/GameSystem/KCLobbyGameMode.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"

AKCLobbyPlayerController::AKCLobbyPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void AKCLobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 오직 로컬 플레이어 화면에서만 UI 위젯 생성 및 마우스 포커스 설정
	if (IsLocalPlayerController())
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);

		if (!LobbyWidgetClass)
		{
			LobbyWidgetClass = StaticLoadClass(UKCLobbyWidget::StaticClass(), nullptr, TEXT("/Game/KC/SteamLobbySystem/Blueprints/UI/WBP_LobbyUI.WBP_LobbyUI_C"));
		}

		if (LobbyWidgetClass)
		{
			LobbyWidgetInstance = CreateWidget<UKCLobbyWidget>(this, LobbyWidgetClass);
			if (LobbyWidgetInstance)
			{
				LobbyWidgetInstance->AddToViewport();
			}
		}
	}
}

void AKCLobbyPlayerController::ROS_ToggleReadyStatus_Implementation()
{
	if (UWorld* World = GetWorld())
	{
		if (AKCLobbyGameMode* GM = World->GetAuthGameMode<AKCLobbyGameMode>())
		{
			GM->HandlePlayerReadyToggled(this);
		}
	}
}

void AKCLobbyPlayerController::ROS_RequestMoveToSlot_Implementation(int32 TargetSlotIndex)
{
	if (UWorld* World = GetWorld())
	{
		if (AKCLobbyGameMode* GM = World->GetAuthGameMode<AKCLobbyGameMode>())
		{
			GM->MovePlayerToSlot(this, TargetSlotIndex);
		}
	}
}

void AKCLobbyPlayerController::ROS_UpdatePlayerInfo_Implementation()
{
	if (UWorld* World = GetWorld())
	{
		if (AKCLobbyGameMode* GM = World->GetAuthGameMode<AKCLobbyGameMode>())
		{
			GM->UpdateLobbyReadyState();
		}
	}
}

void AKCLobbyPlayerController::Client_OnMatchBegin_Implementation()
{
	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);

	if (LobbyWidgetInstance)
	{
		LobbyWidgetInstance->PlayMatchStartAnim();
	}
}

void AKCLobbyPlayerController::Client_SetStartGameButtonEnabled_Implementation(bool bEnabled)
{
	if (LobbyWidgetInstance)
	{
		LobbyWidgetInstance->SetStartGameButtonEnabled(bEnabled);
	}
}
