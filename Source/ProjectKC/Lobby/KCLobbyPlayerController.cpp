/**
 * @file KCLobbyPlayerController.cpp
 * @brief AKCLobbyPlayerController 구현부
 */

#include "ProjectKC/Lobby/KCLobbyPlayerController.h"
#include "ProjectKC/Customization/KCCustomizationNetworkComponent.h"
#include "ProjectKC/Lobby/KCLobbyCharacter.h"
#include "ProjectKC/Lobby/UI/KCLobbyWidget.h"
#include "ProjectKC/Player/KCPlayerState.h"
#include "ProjectKC/GameSystem/KCLobbyGameMode.h"
#include "ProjectKC/ProjectKC.h"
#include "Blueprint/UserWidget.h"
#include "Core/LoadingScreen/KCLoadingScreenSubsystem.h"
#include "GameSystem/KCLevelTypeLibrary.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"

AKCLobbyPlayerController::AKCLobbyPlayerController()
{
	CustomizationNetworkComponent = CreateDefaultSubobject<UKCCustomizationNetworkComponent>(
		TEXT("CustomizationNetwork"));
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void AKCLobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();
	SetupLobbyUI();
	RefreshLobbyCustomizationPresentations();
}

void AKCLobbyPlayerController::PostSeamlessTravel()
{
	Super::PostSeamlessTravel();
	SetupLobbyUI();
	RefreshLobbyCustomizationPresentations();
}

void AKCLobbyPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	if (IsLocalPlayerController() && LobbyWidgetInstance)
	{
		LobbyWidgetInstance->TryBindPlayerState();
	}

	RefreshLobbyCustomizationPresentations();
}

void AKCLobbyPlayerController::RefreshLobbyCustomizationPresentations()
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

	for (TActorIterator<AKCLobbyCharacter> CharacterIterator(World);
		CharacterIterator;
		++CharacterIterator)
	{
		CharacterIterator->RefreshCustomizationPresentation();
	}
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
	if (UKCLevelTypeLibrary::GetLevelTypeFromWorld(World) != EKCLevelType::LobbyLevel)
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
	
	// GasRange 진입 준비: 로딩화면 표시 + 에셋 프리로드 시작점

	// [임시 디버그 로그] "[KC_LOADING_DEBUG]" 붙은 줄들은 호스트 GasRange 진입 멈춤
	// 현상 원인 추적용. 원인 찾으면 이 파일에서도 지우면 됨.
	UE_LOG(LogTemp, Warning, TEXT("[KC_LOADING_DEBUG] Client_OnMatchBegin_Implementation 진입"));
	if (UGameInstance* GI = GetGameInstance())
	{
		UE_LOG(LogTemp, Warning, TEXT("[KC_LOADING_DEBUG] GameInstance 확보됨, LoadingScreenSubsystem 탐색 중"));
		if (UKCLoadingScreenSubsystem* LoadingScreenSubsystem = GI->GetSubsystem<UKCLoadingScreenSubsystem>())
		{
			UE_LOG(LogTemp, Warning, TEXT("[KC_LOADING_DEBUG] LoadingScreenSubsystem 확보됨, BeginPreload 호출 직전"));
			LoadingScreenSubsystem->BeginPreload(EKCLevelType::GasRange, GasRangePreloadAssetTypes, GasRangeLoadingScreenClass, LoadingTipsAsset);
			UE_LOG(LogTemp, Warning, TEXT("[KC_LOADING_DEBUG] BeginPreload 호출에서 정상 리턴함"));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[KC_LOADING_DEBUG] LoadingScreenSubsystem이 NULL임!"));
		}
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

/* =========================================================================
 *  로비 채팅 시스템 구현부 (Lobby Chat System Implementation)
 * ========================================================================= */

void AKCLobbyPlayerController::SendChatMessage(const FString& Message)
{
	// 1. 공백 및 빈 문자열 로컬 사전 차단
	const FString TrimmedMessage = Message.TrimStartAndEnd();
	if (TrimmedMessage.IsEmpty())
	{
		return;
	}

	// 2. 글자 수 제한 초과 검사
	if (TrimmedMessage.Len() > MaxChatMessageLength)
	{
		UE_LOG(LogKCLobby, Warning, TEXT("[KCLobbyPlayerController] SendChatMessage Failed: Message exceeds max length (%d > %d)"),
			TrimmedMessage.Len(), MaxChatMessageLength);
		return;
	}

	// 3. 도배 방지 (쿨타임 검사)
	const double CurrentTime = FPlatformTime::Seconds();
	if (CurrentTime - LastChatMessageTimeSeconds < ChatCooldownSeconds)
	{
		UE_LOG(LogKCLobby, Warning, TEXT("[KCLobbyPlayerController] SendChatMessage Rejected: Cooldown active (%.2fs remaining)"),
			ChatCooldownSeconds - (CurrentTime - LastChatMessageTimeSeconds));
		return;
	}

	LastChatMessageTimeSeconds = CurrentTime;

	// 4. 서버로 전송
	Server_SendChatMessage(TrimmedMessage);
}

void AKCLobbyPlayerController::SendChat(const FString& Message)
{
	SendChatMessage(Message);
}

bool AKCLobbyPlayerController::Server_SendChatMessage_Validate(const FString& Message)
{
	// 서버 측 유효성 검증
	const FString Trimmed = Message.TrimStartAndEnd();
	return !Trimmed.IsEmpty() && Trimmed.Len() <= MaxChatMessageLength;
}

void AKCLobbyPlayerController::Server_SendChatMessage_Implementation(const FString& Message)
{
	const FString TrimmedMessage = Message.TrimStartAndEnd();

	// 1. 발신자 닉네임 가져오기 (스팀 프로필 닉네임 자동 반영)
	FString SenderName = TEXT("Unknown");
	if (PlayerState)
	{
		SenderName = PlayerState->GetPlayerName();
	}

	UE_LOG(LogKCLobby, Log, TEXT("[Server Chat] Broadcast from '%s': %s"), *SenderName, *TrimmedMessage);

	// 2. 현재 월드에 접속 중인 모든 PlayerController를 찾아 Client RPC 브로드캐스트
	if (UWorld* World = GetWorld())
	{
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			if (AKCLobbyPlayerController* LobbyPC = Cast<AKCLobbyPlayerController>(It->Get()))
			{
				LobbyPC->Client_ReceiveChatMessage(SenderName, TrimmedMessage);
			}
		}
	}
}

void AKCLobbyPlayerController::Client_ReceiveChatMessage_Implementation(const FString& SenderName, const FString& Message)
{
	// 1. 출력 로그창(Output Log) 출력
	UE_LOG(LogKCLobby, Log, TEXT("[Chat] [%s]: %s"), *SenderName, *Message);

	// 2. [추후 UI 연동용] 승재님의 위젯이 수신할 수 있도록 델리게이트 브로드캐스트
	OnChatMessageReceived.Broadcast(SenderName, Message);
}

