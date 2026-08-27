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
	SetupLobbyUI();
}

void AKCLobbyPlayerController::PostSeamlessTravel()
{
	Super::PostSeamlessTravel();
	SetupLobbyUI();
}

void AKCLobbyPlayerController::SetupLobbyUI()
{
	if (!IsLocalPlayerController())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// L_LobbyLevel 레벨에 있을 때만 로비 UI 생성
	const FString MapName = World->GetMapName();
	if (!MapName.Contains(TEXT("L_LobbyLevel")))
	{
		return;
	}

	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	bShowMouseCursor = true;

	if (!LobbyWidgetClass)
	{
		LobbyWidgetClass = StaticLoadClass(UKCLobbyWidget::StaticClass(), nullptr, TEXT("/Game/KC/SteamLobbySystem/Blueprints/UI/WBP_LobbyUI.WBP_LobbyUI_C"));
	}

	if (LobbyWidgetClass && (!LobbyWidgetInstance || !LobbyWidgetInstance->IsInViewport()))
	{
		LobbyWidgetInstance = CreateWidget<UKCLobbyWidget>(this, LobbyWidgetClass);
		if (LobbyWidgetInstance)
		{
			LobbyWidgetInstance->AddToViewport();
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
