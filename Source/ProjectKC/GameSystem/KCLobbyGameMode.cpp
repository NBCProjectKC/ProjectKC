/**
 * @file KCLobbyGameMode.cpp
 * @brief AKCLobbyGameMode 구현부 (슬롯 고정형 배정 및 개별 자리 이동)
 */

#include "ProjectKC/GameSystem/KCLobbyGameMode.h"
#include "ProjectKC/Lobby/KCLobbyPlayerState.h"
#include "ProjectKC/Lobby/KCLobbyPlayerController.h"
#include "ProjectKC/Lobby/KCPlayerSlotActor.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
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

	for (AActor* Actor : FoundActors)
	{
		if (AKCPlayerSlotActor* Slot = Cast<AKCPlayerSlotActor>(Actor))
		{
			LobbySlots.Add(Slot);
		}
	}

	ApplySlotOpenCloseRules();
}

void AKCLobbyGameMode::BeginPlay()
{
	Super::BeginPlay();

	// 1. 슬롯 액터 수집 보장
	EnsureSlotsCollected();

	// 2. 이미 접속해 있는 모든 PlayerController(방장 등)를 슬롯에 배정
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PC = It->Get())
		{
			AssignPlayerToAvailableSlot(PC);
		}
	}

	UpdateLobbyReadyState();
}

void AKCLobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// 1. 슬롯 수집 보장 (PostLogin이 BeginPlay보다 먼저 불릴 때 대비)
	EnsureSlotsCollected();

	// 2. 신규 접속자 배정
	AssignPlayerToAvailableSlot(NewPlayer);
}

void AKCLobbyGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	// 퇴장한 플레이어의 슬롯만 비움 (다른 플레이어는 자리 유지)
	RemovePlayerFromSlot(Exiting);
}

void AKCLobbyGameMode::SetRequiredPlayerCount(int32 InCount)
{
	RequiredPlayerCount = FMath::Clamp((InCount / 2) * 2, 2, 6);

	ApplySlotOpenCloseRules();
	UpdateLobbyReadyState();
}

void AKCLobbyGameMode::Debug_SetLobbyPlayers(int32 InCount)
{
	SetRequiredPlayerCount(InCount);
	UE_LOG(LogTemp, Warning, TEXT("Debug_SetLobbyPlayers: RequiredPlayerCount set to %d"), RequiredPlayerCount);
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
	}
}

void AKCLobbyGameMode::AssignPlayerToAvailableSlot(APlayerController* NewPlayer)
{
	if (!NewPlayer)
	{
		return;
	}

	AKCLobbyPlayerState* NewPS = NewPlayer->GetPlayerState<AKCLobbyPlayerState>();
	if (!NewPS)
	{
		FTimerHandle RetryTimerHandle;
		TWeakObjectPtr<APlayerController> WeakPlayer = NewPlayer;
		GetWorldTimerManager().SetTimer(
			RetryTimerHandle,
			[this, WeakPlayer]()
			{
				if (WeakPlayer.IsValid())
				{
					AssignPlayerToAvailableSlot(WeakPlayer.Get());
				}
			},
			0.05f,
			false
		);
		return;
	}

	// 이미 슬롯에 배정되어 있다면 중복 배정 방지
	if (NewPS->GetSlotIndex() != INDEX_NONE)
	{
		for (AKCPlayerSlotActor* Slot : LobbySlots)
		{
			if (Slot && Slot->GetSlotIndex() == NewPS->GetSlotIndex())
			{
				if (!Slot->IsOccupied())
				{
					FKCPlayerInfoStruct Info;
					Info.PlayerName = NewPS->GetPlayerName();
					Info.bReady = NewPS->IsReady();
					Info.PlayerState = NewPS;
					Slot->AssignPlayer(Info);
				}
				return;
			}
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
		FKCPlayerInfoStruct Info;
		Info.PlayerName = NewPS->GetPlayerName();
		Info.bReady = NewPS->IsReady();
		Info.PlayerState = NewPS;

		ChosenSlot->AssignPlayer(Info);
		NewPS->SetSlotIndex(ChosenSlot->GetSlotIndex());
		NewPS->SetTeamId(ChosenSlot->GetSlotTeamId());
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
		return;
	}

	const int32 SlotIdx = ExitingPS->GetSlotIndex();
	for (AKCPlayerSlotActor* Slot : LobbySlots)
	{
		if (Slot && Slot->GetSlotIndex() == SlotIdx)
		{
			Slot->ClearSlot();
			break;
		}
	}

	ExitingPS->SetSlotIndex(INDEX_NONE);
	UpdateLobbyReadyState();
}

bool AKCLobbyGameMode::MovePlayerToSlot(AController* Controller, int32 TargetSlotIndex)
{
	if (!Controller)
	{
		return false;
	}

	AKCLobbyPlayerState* PS = Controller->GetPlayerState<AKCLobbyPlayerState>();
	if (!PS)
	{
		return false;
	}

	EnsureSlotsCollected();

	// 1. 목표 슬롯 탐색 (SlotIndex 매칭)
	AKCPlayerSlotActor* TargetSlot = nullptr;
	for (AKCPlayerSlotActor* Slot : LobbySlots)
	{
		if (Slot && Slot->GetSlotIndex() == TargetSlotIndex)
		{
			TargetSlot = Slot;
			break;
		}
	}

	if (!TargetSlot)
	{
		UKismetSystemLibrary::PrintString(this, FString::Printf(TEXT("Move Failed: Slot %d not found!"), TargetSlotIndex), true, true, FLinearColor::Red, 2.5f);
		return false;
	}

	if (TargetSlot->IsClosed())
	{
		UKismetSystemLibrary::PrintString(this, FString::Printf(TEXT("Move Failed: Slot %d is Closed!"), TargetSlotIndex), true, true, FLinearColor::Red, 2.5f);
		return false;
	}

	if (TargetSlot->IsOccupied())
	{
		UKismetSystemLibrary::PrintString(this, FString::Printf(TEXT("Move Failed: Slot %d is Occupied!"), TargetSlotIndex), true, true, FLinearColor::Red, 2.5f);
		return false;
	}

	// 2. 기존 슬롯 비우기
	const int32 PrevSlotIdx = PS->GetSlotIndex();
	for (AKCPlayerSlotActor* Slot : LobbySlots)
	{
		if (Slot && Slot->GetSlotIndex() == PrevSlotIdx)
		{
			Slot->ClearSlot();
			break;
		}
	}

	// 3. 새 슬롯에 배정
	FKCPlayerInfoStruct Info;
	Info.PlayerName = PS->GetPlayerName();
	Info.bReady = PS->IsReady();
	Info.PlayerState = PS;

	TargetSlot->AssignPlayer(Info);
	PS->SetSlotIndex(TargetSlotIndex);
	PS->SetTeamId(TargetSlot->GetSlotTeamId());

	UKismetSystemLibrary::PrintString(this, FString::Printf(TEXT("Moved to Slot %d (Team %d)!"), TargetSlotIndex, TargetSlot->GetSlotTeamId()), true, true, FLinearColor::Green, 2.5f);

	UpdateLobbyReadyState();
	return true;
}

void AKCLobbyGameMode::HandlePlayerReadyToggled(AController* Controller)
{
	if (!Controller)
	{
		return;
	}

	AKCLobbyPlayerState* PS = Controller->GetPlayerState<AKCLobbyPlayerState>();
	if (!PS)
	{
		return;
	}

	// 레디 상태 토글
	PS->ToggleReady();

	// 해당 플레이어가 앉아있는 슬롯 정보만 안전하게 갱신
	const int32 SlotIdx = PS->GetSlotIndex();
	for (AKCPlayerSlotActor* Slot : LobbySlots)
	{
		if (Slot && Slot->GetSlotIndex() == SlotIdx)
		{
			FKCPlayerInfoStruct Info;
			Info.PlayerName = PS->GetPlayerName();
			Info.bReady = PS->IsReady();
			Info.PlayerState = PS;
			Slot->AssignPlayer(Info);
			break;
		}
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

	ConnectedPlayers.Empty();

	for (APlayerState* PS : GameState->PlayerArray)
	{
		if (AKCLobbyPlayerState* LobbyPS = Cast<AKCLobbyPlayerState>(PS))
		{
			FKCPlayerInfoStruct Info;
			Info.PlayerName = LobbyPS->GetPlayerName();
			Info.bReady = LobbyPS->IsReady();
			Info.PlayerState = LobbyPS;
			ConnectedPlayers.Add(Info);
		}
	}

	const bool bAllReady = CheckAllPlayersReady();

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
	if (GameState)
	{
		for (APlayerState* PS : GameState->PlayerArray)
		{
			if (AKCLobbyPlayerController* LobbyPC = Cast<AKCLobbyPlayerController>(PS->GetOwner()))
			{
				LobbyPC->Client_OnMatchBegin();
			}
		}
	}

	FTimerHandle TravelTimerHandle;
	GetWorldTimerManager().SetTimer(
		TravelTimerHandle,
		[this]()
		{
			FString TravelURL = BattleLevelName.IsEmpty() ? TEXT("L_GasRange") : BattleLevelName;
			TravelURL += TEXT("?listen");
			GetWorld()->ServerTravel(TravelURL);
		},
		1.0f,
		false
	);
}