/**
 * @file KCLobbyGameMode.cpp
 * @brief AKCLobbyGameMode 구현부
 */

#include "ProjectKC/GameSystem/KCLobbyGameMode.h"
#include "ProjectKC/Lobby/KCLobbyPlayerState.h"
#include "ProjectKC/Lobby/KCLobbyPlayerController.h"
#include "ProjectKC/Lobby/KCPlayerSlotActor.h"
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
}

void AKCLobbyGameMode::BeginPlay()
{
	Super::BeginPlay();

	// 에디터나 레벨에서 LobbySlots가 설정되지 않은 경우에만 자동 탐색 및 SlotIndex 순 정렬
	if (LobbySlots.Num() == 0)
	{
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
	}
}

void AKCLobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	UpdatePlayerSlots();
}

void AKCLobbyGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	UpdatePlayerSlots();
}

void AKCLobbyGameMode::SetRequiredPlayerCount(int32 InCount)
{
	RequiredPlayerCount = InCount;
}

bool AKCLobbyGameMode::CheckAllPlayersReady() const
{
	if (ConnectedPlayers.Num() == 0)
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

void AKCLobbyGameMode::UpdatePlayerSlots()
{
	if (!GameState)
	{
		return;
	}

	TArray<TObjectPtr<APlayerState>> PlayerArray = GameState->PlayerArray;
	ConnectedPlayers.Empty();

	for (int32 i = 0; i < PlayerArray.Num(); ++i)
	{
		if (AKCLobbyPlayerState* LobbyPS = Cast<AKCLobbyPlayerState>(PlayerArray[i]))
		{
			FKCPlayerInfoStruct Info;
			Info.PlayerName = LobbyPS->GetPlayerName();
			Info.bReady = LobbyPS->IsReady();
			Info.PlayerState = LobbyPS;
			ConnectedPlayers.Add(Info);
		}
	}

	// 각 슬롯에 플레이어 배정 또는 비우기 (최대 6개 슬롯 지원)
	for (int32 SlotIdx = 0; SlotIdx < LobbySlots.Num(); ++SlotIdx)
	{
		if (AKCPlayerSlotActor* Slot = LobbySlots[SlotIdx])
		{
			if (ConnectedPlayers.IsValidIndex(SlotIdx))
			{
				Slot->AssignPlayer(ConnectedPlayers[SlotIdx]);
			}
			else
			{
				Slot->ClearSlot();
			}
		}
	}

	// 모든 접속 플레이어 준비 완료 여부 검사
	const bool bAllReady = CheckAllPlayersReady();

	// 방장(호스트)에게 StartGame 버튼 활성화 상태 전달
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
	// 모든 접속 플레이어 컨트롤러에 시작 연출 RPC 전송
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

	// 1초 뒤 서버 트래블로 인게임 레벨 진입
	FTimerHandle TravelTimerHandle;
	GetWorldTimerManager().SetTimer(
		TravelTimerHandle,
		[this]()
		{
			FString TravelURL = BattleLevelName.IsEmpty() ? TEXT("Lvl_Main") : BattleLevelName;
			TravelURL += TEXT("?listen");
			GetWorld()->ServerTravel(TravelURL);
		},
		1.0f,
		false
	);
}