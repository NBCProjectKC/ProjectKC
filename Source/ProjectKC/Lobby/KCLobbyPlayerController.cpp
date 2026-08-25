/**
 * @file KCLobbyPlayerController.cpp
 * @brief AKCLobbyPlayerController 구현부
 */

#include "ProjectKC/Lobby/KCLobbyPlayerController.h"
#include "ProjectKC/Lobby/KCLobbyPlayerState.h"
#include "ProjectKC/Lobby/UI/KCLobbyWidget.h"
#include "ProjectKC/GameSystem/KCLobbyGameMode.h"

AKCLobbyPlayerController::AKCLobbyPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void AKCLobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalPlayerController())
	{
		// 마우스 커서 표시 및 Game & UI 입력 모드 설정
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);

		bShowMouseCursor = true;

		// 로비 메인 위젯 클래스 동적 로드 (CDO 순환 참조 방지)
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

		// 서버에 초기 플레이어 정보 동기화 요청
		ROS_UpdatePlayerInfo();
	}
}

void AKCLobbyPlayerController::ROS_ToggleReadyStatus_Implementation()
{
	if (AKCLobbyPlayerState* LobbyPS = GetPlayerState<AKCLobbyPlayerState>())
	{
		LobbyPS->ToggleReady();
	}

	if (UWorld* World = GetWorld())
	{
		if (AKCLobbyGameMode* GM = World->GetAuthGameMode<AKCLobbyGameMode>())
		{
			GM->UpdatePlayerSlots();
		}
	}
}

void AKCLobbyPlayerController::ROS_UpdatePlayerInfo_Implementation()
{
	if (UWorld* World = GetWorld())
	{
		if (AKCLobbyGameMode* GM = World->GetAuthGameMode<AKCLobbyGameMode>())
		{
			GM->UpdatePlayerSlots();
		}
	}
}

void AKCLobbyPlayerController::Client_OnMatchBegin_Implementation()
{
	DisableInput(this);

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
