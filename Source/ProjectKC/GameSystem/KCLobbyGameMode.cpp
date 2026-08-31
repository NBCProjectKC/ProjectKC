/**
 * @file KCLobbyGameMode.cpp
 * @brief AKCLobbyGameMode 구현부 (슬롯 고정형 배정 및 개별 자리 이동)
 */

#include "ProjectKC/GameSystem/KCLobbyGameMode.h"
#include "ProjectKC/Lobby/KCLobbyPlayerState.h"
#include "ProjectKC/Lobby/KCLobbyPlayerController.h"
#include "ProjectKC/Lobby/KCPlayerSlotActor.h"
#include "ProjectKC/Lobby/KCSessionSubsystem.h"
#include "ProjectKC/ProjectKC.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

AKCLobbyGameMode::AKCLobbyGameMode()
{
	bUseSeamlessTravel = true;

	PlayerControllerClass = AKCLobbyPlayerController::StaticClass();
	PlayerStateClass = AKCLobbyPlayerState::StaticClass();
	DefaultPawnClass = nullptr;

	RequiredPlayerCount = 6;
	BattleLevelName = TEXT("L_GasRange");
}

void AKCLobbyGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	UE_LOG(LogKCLobby, Log, TEXT("[KCLobbyGameMode] InitGame - MapName: %s, Options: %s"), *MapName, *Options);

	FString PlayersOption = UGameplayStatics::ParseOption(Options, TEXT("Players"));
	if (PlayersOption.IsEmpty())
	{
		PlayersOption = UGameplayStatics::ParseOption(Options, TEXT("MaxPlayers"));
	}

	if (!PlayersOption.IsEmpty())
	{
		const int32 ParsedCount = FCString::Atoi(*PlayersOption);
		if (ParsedCount > 0)
		{
			SetRequiredPlayerCount(ParsedCount);
		}
	}
	else
	{
		UE_LOG(LogKCLobby, Log, TEXT("[KCLobbyGameMode] No Players option found. Defaulting RequiredPlayerCount to %d"), RequiredPlayerCount);
	}
}

void AKCLobbyGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);

	if (!ErrorMessage.IsEmpty())
	{
		return;
	}

	// 현재 접속자 수 검사 (최대 필요 인원수 도달 시 접속 거부)
	const int32 CurrentPlayerCount = GetNumPlayers();
	if (CurrentPlayerCount >= RequiredPlayerCount)
	{
		ErrorMessage = TEXT("LOBBY_FULL: Maximum player capacity reached for this lobby session.");
		UE_LOG(LogKCLobby, Warning, TEXT("[KCLobbyGameMode] PreLogin Rejected: Lobby is full! (Current: %d / %d, Address: %s)"),
			CurrentPlayerCount, RequiredPlayerCount, *Address);
		return;
	}

	UE_LOG(LogKCLobby, Log, TEXT("[KCLobbyGameMode] PreLogin Accepted: (Current: %d / %d, Address: %s)"),
		CurrentPlayerCount, RequiredPlayerCount, *Address);
}

AKCPlayerSlotActor* AKCLobbyGameMode::FindSlotByIndex(int32 SlotIndex) const
{
	if (SlotIndex < 0 || SlotIndex >= 6)
	{
		UE_LOG(LogKCLobby, Warning, TEXT("[KCLobbyGameMode] FindSlotByIndex: SlotIndex %d out of bounds (0-5)"), SlotIndex);
		return nullptr;
	}

	for (AKCPlayerSlotActor* Slot : LobbySlots)
	{
		if (Slot && Slot->GetSlotIndex() == SlotIndex)
		{
			return Slot;
		}
	}

	return nullptr;
}

void AKCLobbyGameMode::AssignPlayerToSlot(AKCPlayerSlotActor* TargetSlot, AKCLobbyPlayerState* PS)
{
	if (!TargetSlot || !PS)
	{
		UE_LOG(LogKCLobby, Warning, TEXT("[KCLobbyGameMode] AssignPlayerToSlot Failed: TargetSlot (%s) or PlayerState (%s) is null"),
			TargetSlot ? *TargetSlot->GetName() : TEXT("null"),
			PS ? *PS->GetName() : TEXT("null"));
		return;
	}

	const FKCPlayerInfoStruct Info(PS->GetPlayerName(), PS->IsReady(), PS);
	TargetSlot->AssignPlayer(Info);
	PS->SetSlotIndex(TargetSlot->GetSlotIndex());
	PS->SetTeamId(TargetSlot->GetSlotTeamId());

	UE_LOG(LogKCLobby, Log, TEXT("[KCLobbyGameMode] Assigned Player '%s' to Slot %d (Team %d)"),
		*PS->GetPlayerName(), TargetSlot->GetSlotIndex(), TargetSlot->GetSlotTeamId());
}

void AKCLobbyGameMode::EnsureSlotsCollected()
{
	if (LobbySlots.Num() > 0)
	{
		return;
	}

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(this, AKCPlayerSlotActor::StaticClass(), FoundActors);

	FoundActors.Sort([](const AActor& A, const AActor& B)
	{
		const AKCPlayerSlotActor* SlotA = Cast<AKCPlayerSlotActor>(&A);
		const AKCPlayerSlotActor* SlotB = Cast<AKCPlayerSlotActor>(&B);
		if (SlotA && SlotB && SlotA->GetSlotIndex() != SlotB->GetSlotIndex())
		{
			return SlotA->GetSlotIndex() < SlotB->GetSlotIndex();
		}
		return A.GetName() < B.GetName();
	});

	LobbySlots.Reset(FoundActors.Num());
	for (AActor* Actor : FoundActors)
	{
		if (AKCPlayerSlotActor* Slot = Cast<AKCPlayerSlotActor>(Actor))
		{
			LobbySlots.Add(Slot);
		}
	}

	UE_LOG(LogKCLobby, Log, TEXT("[KCLobbyGameMode] EnsureSlotsCollected: Found and collected %d Slot Actors in level"), LobbySlots.Num());
	if (LobbySlots.Num() != 6)
	{
		UE_LOG(LogKCLobby, Warning, TEXT("[KCLobbyGameMode] Expected 6 Slot Actors, but found %d. Please check the lobby map placement."), LobbySlots.Num());
	}

	ApplySlotOpenCloseRules();
}

void AKCLobbyGameMode::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogKCLobby, Log, TEXT("[KCLobbyGameMode] BeginPlay started. RequiredPlayerCount: %d"), RequiredPlayerCount);

	// 1. 슬롯 액터 수집 보장
	EnsureSlotsCollected();

	// 2. 이미 접속해 있는 모든 PlayerController(방장 등)를 슬롯에 배정
	if (UWorld* World = GetWorld())
	{
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			if (APlayerController* PC = It->Get())
			{
				AssignPlayerToAvailableSlot(PC);
			}
		}
	}

	UpdateLobbyReadyState();
}

void AKCLobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	const FString PlayerName = (NewPlayer && NewPlayer->PlayerState) ? NewPlayer->PlayerState->GetPlayerName() : TEXT("Unknown");
	UE_LOG(LogKCLobby, Log, TEXT("[KCLobbyGameMode] PostLogin - Player: %s (Controller: %s)"),
		*PlayerName, NewPlayer ? *NewPlayer->GetName() : TEXT("null"));

	// 1. 슬롯 수집 보장 (PostLogin이 BeginPlay보다 먼저 불릴 때 대비)
	EnsureSlotsCollected();

	// 2. 신규 접속자 배정
	AssignPlayerToAvailableSlot(NewPlayer);
}

void AKCLobbyGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	const FString PlayerName = (Exiting && Exiting->PlayerState) ? Exiting->PlayerState->GetPlayerName() : TEXT("Unknown");
	UE_LOG(LogKCLobby, Log, TEXT("[KCLobbyGameMode] Logout - Player: %s (Controller: %s)"),
		*PlayerName, Exiting ? *Exiting->GetName() : TEXT("null"));

	// 퇴장한 플레이어의 슬롯만 비움 (다른 플레이어는 자리 유지)
	RemovePlayerFromSlot(Exiting);
}

void AKCLobbyGameMode::SetRequiredPlayerCount(int32 InCount)
{
	const int32 OldCount = RequiredPlayerCount;
	RequiredPlayerCount = FMath::Clamp((InCount / 2) * 2, 2, 6);

	UE_LOG(LogKCLobby, Log, TEXT("[KCLobbyGameMode] SetRequiredPlayerCount: %d -> %d (Input: %d)"),
		OldCount, RequiredPlayerCount, InCount);

	ApplySlotOpenCloseRules();
	UpdateLobbyReadyState();
}

void AKCLobbyGameMode::Debug_SetLobbyPlayers(int32 InCount)
{
	SetRequiredPlayerCount(InCount);
	UE_LOG(LogKCLobby, Warning, TEXT("[KCLobbyGameMode] Debug_SetLobbyPlayers Executed: RequiredPlayerCount set to %d"), RequiredPlayerCount);
}

void AKCLobbyGameMode::ApplySlotOpenCloseRules()
{
	const int32 PlayersPerTeam = RequiredPlayerCount / 2;

	for (AKCPlayerSlotActor* Slot : LobbySlots)
	{
		if (!Slot)
		{
			continue;
		}

		const int32 SlotIdx = Slot->GetSlotIndex();
		bool bShouldOpen = false;

		if (SlotIdx >= 0 && SlotIdx < 3)
		{
			// Team 0 슬롯 (0, 1, 2)
			bShouldOpen = (SlotIdx < PlayersPerTeam);
		}
		else if (SlotIdx >= 3 && SlotIdx < 6)
		{
			// Team 1 슬롯 (3, 4, 5)
			bShouldOpen = ((SlotIdx - 3) < PlayersPerTeam);
		}

		Slot->SetSlotClosed(!bShouldOpen);
		UE_LOG(LogKCLobby, Verbose, TEXT("[KCLobbyGameMode] Slot %d (Team %d) -> %s"),
			SlotIdx, Slot->GetSlotTeamId(), bShouldOpen ? TEXT("OPEN") : TEXT("CLOSED"));
	}
}

void AKCLobbyGameMode::AssignPlayerToAvailableSlot(APlayerController* NewPlayer)
{
	if (!NewPlayer)
	{
		UE_LOG(LogKCLobby, Warning, TEXT("[KCLobbyGameMode] AssignPlayerToAvailableSlot Failed: NewPlayer is null"));
		return;
	}

	AKCLobbyPlayerState* NewPS = NewPlayer->GetPlayerState<AKCLobbyPlayerState>();
	if (!NewPS)
	{
		UE_LOG(LogKCLobby, Verbose, TEXT("[KCLobbyGameMode] PlayerState not ready yet for %s. Retrying in 0.05s..."), *NewPlayer->GetName());
		FTimerHandle RetryTimerHandle;
		TWeakObjectPtr<AKCLobbyGameMode> WeakThis(this);
		TWeakObjectPtr<APlayerController> WeakPlayer(NewPlayer);
		GetWorldTimerManager().SetTimer(
			RetryTimerHandle,
			[WeakThis, WeakPlayer]()
			{
				if (WeakThis.IsValid() && WeakPlayer.IsValid())
				{
					WeakThis->AssignPlayerToAvailableSlot(WeakPlayer.Get());
				}
			},
			0.05f,
			false
		);
		return;
	}

	// 이미 슬롯에 배정되어 있다면 중복 배정 방지 및 정보 동기화
	if (NewPS->GetSlotIndex() != INDEX_NONE)
	{
		if (AKCPlayerSlotActor* Slot = FindSlotByIndex(NewPS->GetSlotIndex()))
		{
			if (!Slot->IsOccupied())
			{
				const FKCPlayerInfoStruct Info(NewPS->GetPlayerName(), NewPS->IsReady(), NewPS);
				Slot->AssignPlayer(Info);
				UE_LOG(LogKCLobby, Log, TEXT("[KCLobbyGameMode] Player '%s' already had SlotIndex %d. Synchronized slot data."),
					*NewPS->GetPlayerName(), NewPS->GetSlotIndex());
			}
			return;
		}
	}

	const int32 PlayersPerTeam = RequiredPlayerCount / 2;

	// 현재 Team 0과 Team 1의 점유 인원수 카운트
	int32 Team0Count = 0;
	int32 Team1Count = 0;
	for (AKCPlayerSlotActor* Slot : LobbySlots)
	{
		if (Slot && Slot->IsOccupied())
		{
			if (Slot->GetSlotTeamId() == 0) Team0Count++;
			else if (Slot->GetSlotTeamId() == 1) Team1Count++;
		}
	}

	// 열려있고 비어있는 슬롯 탐색
	AKCPlayerSlotActor* ChosenSlot = nullptr;

	// Team 0 우선 탐색
	if (Team0Count <= Team1Count && Team0Count < PlayersPerTeam)
	{
		for (AKCPlayerSlotActor* Slot : LobbySlots)
		{
			if (Slot && Slot->GetSlotTeamId() == 0 && !Slot->IsClosed() && !Slot->IsOccupied())
			{
				ChosenSlot = Slot;
				break;
			}
		}
	}

	// Team 1 탐색
	if (!ChosenSlot && Team1Count < PlayersPerTeam)
	{
		for (AKCPlayerSlotActor* Slot : LobbySlots)
		{
			if (Slot && Slot->GetSlotTeamId() == 1 && !Slot->IsClosed() && !Slot->IsOccupied())
			{
				ChosenSlot = Slot;
				break;
			}
		}
	}

	// 슬롯에 배정
	if (ChosenSlot)
	{
		AssignPlayerToSlot(ChosenSlot, NewPS);
	}
	else
	{
		UE_LOG(LogKCLobby, Warning, TEXT("[KCLobbyGameMode] Could not find an available slot for Player '%s' (Team0: %d, Team1: %d, MaxPerTeam: %d)"),
			*NewPS->GetPlayerName(), Team0Count, Team1Count, PlayersPerTeam);
	}

	UpdateLobbyReadyState();
}

void AKCLobbyGameMode::RemovePlayerFromSlot(AController* Exiting)
{
	if (!Exiting)
	{
		return;
	}

	AKCLobbyPlayerState* ExitingPS = Exiting->GetPlayerState<AKCLobbyPlayerState>();
	if (!ExitingPS)
	{
		UE_LOG(LogKCLobby, Verbose, TEXT("[KCLobbyGameMode] RemovePlayerFromSlot: PlayerState is null for %s"), *Exiting->GetName());
		return;
	}

	const int32 SlotIdx = ExitingPS->GetSlotIndex();
	if (AKCPlayerSlotActor* Slot = FindSlotByIndex(SlotIdx))
	{
		Slot->ClearSlot();
		UE_LOG(LogKCLobby, Log, TEXT("[KCLobbyGameMode] Cleared Slot %d for exiting player '%s'"), SlotIdx, *ExitingPS->GetPlayerName());
	}

	ExitingPS->SetSlotIndex(INDEX_NONE);
	UpdateLobbyReadyState();
}

bool AKCLobbyGameMode::MovePlayerToSlot(AController* Controller, int32 TargetSlotIndex)
{
	if (!Controller)
	{
		UE_LOG(LogKCLobby, Warning, TEXT("[KCLobbyGameMode] MovePlayerToSlot Failed: Controller is null"));
		return false;
	}

	if (TargetSlotIndex < 0 || TargetSlotIndex >= 6)
	{
		UE_LOG(LogKCLobby, Warning, TEXT("[KCLobbyGameMode] MovePlayerToSlot Failed: Invalid SlotIndex %d"), TargetSlotIndex);
		return false;
	}

	AKCLobbyPlayerState* PS = Controller->GetPlayerState<AKCLobbyPlayerState>();
	if (!PS)
	{
		UE_LOG(LogKCLobby, Warning, TEXT("[KCLobbyGameMode] MovePlayerToSlot Failed: PlayerState is null for %s"), *Controller->GetName());
		return false;
	}
	
	// 플레이어 상태가 준비 상태면 이동 불가
	if (PS->IsReady())
	{
		UE_LOG(LogKCLobby, Warning, TEXT("[KCLobbyGameMode] MovePlayerToSlot Rejected: Player '%s' is Ready"), *PS->GetPlayerName());
		return false;
	}

	EnsureSlotsCollected();

	// 1. 목표 슬롯 탐색 (SlotIndex 매칭)
	AKCPlayerSlotActor* TargetSlot = FindSlotByIndex(TargetSlotIndex);
	if (!TargetSlot)
	{
		UE_LOG(LogKCLobby, Warning, TEXT("[KCLobbyGameMode] Move Failed: Slot %d not found!"), TargetSlotIndex);
		return false;
	}

	if (TargetSlot->IsClosed())
	{
		UE_LOG(LogKCLobby, Warning, TEXT("[KCLobbyGameMode] Move Failed: Slot %d is Closed!"), TargetSlotIndex);
		return false;
	}

	if (TargetSlot->IsOccupied())
	{
		UE_LOG(LogKCLobby, Warning, TEXT("[KCLobbyGameMode] Move Failed: Slot %d is Occupied!"), TargetSlotIndex);
		return false;
	}

	// 2. 기존 슬롯 비우기
	const int32 PrevSlotIdx = PS->GetSlotIndex();
	if (AKCPlayerSlotActor* PrevSlot = FindSlotByIndex(PrevSlotIdx))
	{
		PrevSlot->ClearSlot();
	}

	// 3. 새 슬롯에 배정
	AssignPlayerToSlot(TargetSlot, PS);

	UE_LOG(LogKCLobby, Log, TEXT("[KCLobbyGameMode] Player '%s' moved from Slot %d to Slot %d (Team %d)"),
		*PS->GetPlayerName(), PrevSlotIdx, TargetSlotIndex, TargetSlot->GetSlotTeamId());

	UpdateLobbyReadyState();
	return true;
}

void AKCLobbyGameMode::HandlePlayerReadyToggled(AController* Controller)
{
	if (!Controller)
	{
		UE_LOG(LogKCLobby, Warning, TEXT("[KCLobbyGameMode] HandlePlayerReadyToggled Failed: Controller is null"));
		return;
	}

	AKCLobbyPlayerState* PS = Controller->GetPlayerState<AKCLobbyPlayerState>();
	if (!PS)
	{
		UE_LOG(LogKCLobby, Warning, TEXT("[KCLobbyGameMode] HandlePlayerReadyToggled Failed: PlayerState is null for %s"), *Controller->GetName());
		return;
	}

	// 레디 상태 토글
	PS->ToggleReady();

	UE_LOG(LogKCLobby, Log, TEXT("[KCLobbyGameMode] Player '%s' Ready status changed to: %s"),
		*PS->GetPlayerName(), PS->IsReady() ? TEXT("READY") : TEXT("NOT READY"));

	// 해당 플레이어가 앉아있는 슬롯 정보만 안전하게 갱신
	const int32 SlotIdx = PS->GetSlotIndex();
	if (AKCPlayerSlotActor* Slot = FindSlotByIndex(SlotIdx))
	{
		const FKCPlayerInfoStruct Info(PS->GetPlayerName(), PS->IsReady(), PS);
		Slot->AssignPlayer(Info);
	}

	UpdateLobbyReadyState();
}

bool AKCLobbyGameMode::CheckAllPlayersReady() const
{
	if (ConnectedPlayers.Num() != RequiredPlayerCount || RequiredPlayerCount == 0)
	{
		return false;
	}

	for (const FKCPlayerInfoStruct& Info : ConnectedPlayers)
	{
		if (!Info.bReady)
		{
			return false;
		}
	}
	return true;
}

void AKCLobbyGameMode::UpdateLobbyReadyState()
{
	if (!GameState)
	{
		return;
	}

	ConnectedPlayers.Reset(GameState->PlayerArray.Num());

	int32 ReadyCount = 0;
	for (APlayerState* PS : GameState->PlayerArray)
	{
		if (AKCLobbyPlayerState* LobbyPS = Cast<AKCLobbyPlayerState>(PS))
		{
			ConnectedPlayers.Emplace(LobbyPS->GetPlayerName(), LobbyPS->IsReady(), LobbyPS);
			if (LobbyPS->IsReady())
			{
				ReadyCount++;
			}
		}
	}

	const bool bAllReady = CheckAllPlayersReady();

	UE_LOG(LogKCLobby, Log, TEXT("[KCLobbyGameMode] UpdateLobbyReadyState - Connected: %d / %d, Ready: %d, AllReady: %s"),
		ConnectedPlayers.Num(), RequiredPlayerCount, ReadyCount, bAllReady ? TEXT("TRUE") : TEXT("FALSE"));

	// 방장에게 StartGame 버튼 활성화 전달
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* HostPC = World->GetFirstPlayerController())
		{
			if (AKCLobbyPlayerController* LobbyPC = Cast<AKCLobbyPlayerController>(HostPC))
			{
				LobbyPC->Client_SetStartGameButtonEnabled(bAllReady);
			}
		}
	}
}

void AKCLobbyGameMode::StartGame()
{
	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		UE_LOG(LogKCLobby, Error, TEXT("[KCLobbyGameMode] StartGame Failed: GameInstance is null"));
		return;
	}

	// 1. KCSessionSubsystem에 로비 플레이어 데이터(TeamId, SlotIndex, 인원수) 영구 보존
	if (UKCSessionSubsystem* Subsystem = GI->GetSubsystem<UKCSessionSubsystem>())
	{
		Subsystem->ClearSavedLobbyData();

		int32 ActiveCount = 0;
		if (GameState)
		{
			for (APlayerState* PS : GameState->PlayerArray)
			{
				if (AKCLobbyPlayerState* LobbyPS = Cast<AKCLobbyPlayerState>(PS))
				{
					if (LobbyPS->GetSlotIndex() != INDEX_NONE)
					{
						Subsystem->SaveLobbyPlayerData(LobbyPS->GetUniquePlayerIdString(), LobbyPS->GetPlayerName(), LobbyPS->GetTeamId(), LobbyPS->GetSlotIndex());
						ActiveCount++;
					}
				}
			}
		}
		Subsystem->SetExpectedPlayerCount(ActiveCount);
		UE_LOG(LogKCLobby, Log, TEXT("[KCLobbyGameMode] StartGame: Saved %d active players data into KCSessionSubsystem"), ActiveCount);
	}

	// 2. 클라이언트들에게 매치 시작 알림
	if (GameState)
	{
		for (APlayerState* PS : GameState->PlayerArray)
		{
			if (PS)
			{
				if (AKCLobbyPlayerController* LobbyPC = Cast<AKCLobbyPlayerController>(PS->GetOwner()))
				{
					LobbyPC->Client_OnMatchBegin();
				}
			}
		}
	}

	// 3. 인게임 레벨로 ServerTravel 이동
	FTimerHandle TravelTimerHandle;
	TWeakObjectPtr<AKCLobbyGameMode> WeakThis(this);
	GetWorldTimerManager().SetTimer(
		TravelTimerHandle,
		[WeakThis]()
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			if (UWorld* World = WeakThis->GetWorld())
			{
				FString TravelURL = WeakThis->BattleLevelName.IsEmpty() ? TEXT("L_GasRange") : WeakThis->BattleLevelName;
				TravelURL += TEXT("?listen");
				UE_LOG(LogKCLobby, Log, TEXT("[KCLobbyGameMode] Executing ServerTravel to: %s"), *TravelURL);
				World->ServerTravel(TravelURL);
			}
		},
		1.0f,
		false
	);
}
