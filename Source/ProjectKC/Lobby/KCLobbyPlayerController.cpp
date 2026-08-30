/**
 * @file KCLobbyPlayerController.cpp
 * @brief AKCLobbyPlayerController 구현부
 */

#include "ProjectKC/Lobby/KCLobbyPlayerController.h"
#include "ProjectKC/Lobby/UI/KCLobbyWidget.h"
#include "ProjectKC/Lobby/KCLobbyPlayerState.h"
#include "ProjectKC/GameSystem/KCLobbyGameMode.h"
#include "ProjectKC/ProjectKC.h"
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
		UE_LOG(LogKCLobby, Verbose, TEXT("[KCLobbyPlayerController] SetupLobbyUI skipped: Not in L_LobbyLevel (Current: %s)"), *MapName);
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
		if (!LobbyWidgetClass)
		{
			UE_LOG(LogKCLobby, Error, TEXT("[KCLobbyPlayerController] SetupLobbyUI Failed: Could not load WBP_LobbyUI"));
			return;
		}
	}

	if (LobbyWidgetClass && (!LobbyWidgetInstance || !LobbyWidgetInstance->IsInViewport()))
	{
		LobbyWidgetInstance = CreateWidget<UKCLobbyWidget>(this, LobbyWidgetClass);
		if (LobbyWidgetInstance)
		{
			LobbyWidgetInstance->AddToViewport();
			UE_LOG(LogKCLobby, Log, TEXT("[KCLobbyPlayerController] SetupLobbyUI: Created and added WBP_LobbyUI to viewport"));
		}
		else
		{
			UE_LOG(LogKCLobby, Error, TEXT("[KCLobbyPlayerController] SetupLobbyUI Failed: Failed to create LobbyWidgetInstance"));
		}
	}
}

void AKCLobbyPlayerController::ROS_ToggleReadyStatus_Implementation()
{
	const FString PlayerName = PlayerState ? PlayerState->GetPlayerName() : GetName();
	UE_LOG(LogKCLobby, Log, TEXT("[KCLobbyPlayerController] ROS_ToggleReadyStatus received from Player '%s'"), *PlayerName);

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
	const FString PlayerName = PlayerState ? PlayerState->GetPlayerName() : GetName();

	if (TargetSlotIndex < 0 || TargetSlotIndex >= 6)
	{
		UE_LOG(LogKCLobby, Warning, TEXT("[KCLobbyPlayerController] ROS_RequestMoveToSlot: Invalid SlotIndex %d from Player '%s'"),
			TargetSlotIndex, *PlayerName);
		return;
	}

	UE_LOG(LogKCLobby, Log, TEXT("[KCLobbyPlayerController] ROS_RequestMoveToSlot: Player '%s' requested move to Slot %d"),
		*PlayerName, TargetSlotIndex);

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
	UE_LOG(LogKCLobby, Verbose, TEXT("[KCLobbyPlayerController] ROS_UpdatePlayerInfo received from %s"), *GetName());

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
	UE_LOG(LogKCLobby, Log, TEXT("[KCLobbyPlayerController] Client_OnMatchBegin received. Playing match start animation and locking inputs."));

	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);

	if (LobbyWidgetInstance)
	{
		LobbyWidgetInstance->PlayMatchStartAnim();
	}
}

void AKCLobbyPlayerController::Client_SetStartGameButtonEnabled_Implementation(bool bEnabled)
{
	UE_LOG(LogKCLobby, Verbose, TEXT("[KCLobbyPlayerController] Client_SetStartGameButtonEnabled: %s"), bEnabled ? TEXT("TRUE") : TEXT("FALSE"));

	if (LobbyWidgetInstance)
	{
		LobbyWidgetInstance->SetStartGameButtonEnabled(bEnabled);
	}
}

