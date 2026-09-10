#include "KCGameMode.h"

#include "Engine/DataTable.h"
#include "KCGameState.h"
#include "KCGamePhaseType.h"
#include "Recipe/KCRecipeStruct.h"
#include "Recipe/KCRecipeCompletedStruct.h"
#include "Recipe/KCDishFinishedStruct.h"
#include "Recipe/KCDishRuinedStruct.h"
#include "Messages/KCGameplayTags.h"
#include "Messages/Struct/KCIngredientSubmittedStruct.h"
#include "ProjectKC/ProjectKC.h"
#include "ProjectKC/Player/KCPlayerState.h"
#include "ProjectKC/Lobby/KCSessionSubsystem.h"
#include "Player/KCPlayerCharacter.h"
#include "Player/KCPlayerController.h"
#include "KCLevelTypeLibrary.h"
#include "TimerManager.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "KCLobbyGameMode.h"


AKCGameMode::AKCGameMode()
{
	bUseSeamlessTravel = true;
	
	DefaultPawnClass = AKCPlayerCharacter::StaticClass();
	PlayerControllerClass = AKCPlayerController::StaticClass();
	PlayerStateClass = AKCPlayerState::StaticClass();
	HUDClass = nullptr;
}

// 매치 흐름
int32 AKCGameMode::GetRequiredPlayerCount() const
{
	if (const UGameInstance* GI = GetGameInstance())
	{
		if (const UKCSessionSubsystem* SessionSub = GI->GetSubsystem<UKCSessionSubsystem>())
		{
			const int32 ExpectedCount = SessionSub->GetExpectedPlayerCount();
			if (ExpectedCount > 0)
			{
				return ExpectedCount;
			}
		}
	}

	return TeamCount * PlayersPerTeam;
}

bool AKCGameMode::ReadyToStartMatch_Implementation()
{
	const int32 CurrentPlayers = GetNumPlayers();
	const int32 RequiredPlayers = GetRequiredPlayerCount();
	return CurrentPlayers >= RequiredPlayers;
}

void AKCGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	// 세션 설정 인원에 맞춰 팀당 인원수 동기화
	if (TeamCount > 0)
	{
		PlayersPerTeam = FMath::Max(1, GetRequiredPlayerCount() / TeamCount);
	}

	UE_LOG(LogKCGameSystem, Warning, TEXT("[Match] HandleMatchHasStarted 진입 - 접속 인원: %d, 요구 인원: %d, 팀당 인원: %d"),
		GetNumPlayers(), GetRequiredPlayerCount(), PlayersPerTeam);

	KCGameState = GetGameState<AKCGameState>();

	IngredientSubmittedListenerHandle = UGameplayMessageSubsystem::Get(this).RegisterListener<FKCIngredientSubmittedStruct>(
		KCGameplayTags::Message_Ingredient_Submitted, this, &AKCGameMode::OnIngredientSubmitted);

	DishFinishedListenerHandle = UGameplayMessageSubsystem::Get(this).RegisterListener<FKCDishFinishedStruct>(
		KCGameplayTags::Message_Dish_Finished, this, &AKCGameMode::OnDishFinished);

	if (KCGameState)
	{
		KCGameState->InitializeTeamCount(TeamCount);
		KCGameState->SetActiveRecipes(SelectActiveRecipes()); // 그 판의 레시피 룰렛
		KCGameState->SetGamePhase(EKCGamePhaseType::Playing); // phase 변경
		
		// KCSessionSubsystem에서 로비 설정 매치 시간 복원
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UKCSessionSubsystem* SessionSub = GI->GetSubsystem<UKCSessionSubsystem>())
			{
				if (SessionSub->GetMatchDurationSeconds() > 0.0f)
				{
					MatchDurationSeconds = SessionSub->GetMatchDurationSeconds();
				}
			}
		}
			
		// TODO 임시 코드
		// GameState의 서버시간 설정
		const float ServerNow = GetWorld()->GetTimeSeconds();
		const float SafeMatchDuration = FMath::Max(1.0f, MatchDurationSeconds);
		KCGameState->SetMatchStartServerTime(ServerNow); // 매치 시작 후 타이머 세팅
		KCGameState->SetMatchEndServerTime(ServerNow + SafeMatchDuration);
	}

	// TODO 임시 코드
	// Timer 끝날 때 게임 종료 처리 
	GetWorldTimerManager().SetTimer(
		MatchTimerHandle,
		this,
		&AKCGameMode::HandleMatchTimeExpired,
		FMath::Max(1.0f, MatchDurationSeconds),
		false);
}

void AKCGameMode::HandleMatchHasEnded()
{
	GetWorldTimerManager().ClearTimer(MatchTimerHandle);
	UGameplayMessageSubsystem::Get(this).UnregisterListener(IngredientSubmittedListenerHandle);
	UGameplayMessageSubsystem::Get(this).UnregisterListener(DishFinishedListenerHandle);

	Super::HandleMatchHasEnded();
}

// 레시피 선정 로직
TArray<FName> AKCGameMode::SelectActiveRecipes() const
{
	// 디버깅용 특정 레시피 고정
	if (bUseFixedRecipeList && FixedRecipeList.Num() > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("SelectActiveRecipes: 고정 레시피 목록 사용 (%d개)"), FixedRecipeList.Num());
		return FixedRecipeList;
	}

	if (!KCGameState)
	{
		UE_LOG(LogTemp, Error, TEXT("SelectActiveRecipes: KCGameState가 없습니다."));
		return TArray<FName>();
	}

	TArray<FName> Candidates = KCGameState->GetAllRecipeRowNames();
	if (Candidates.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("SelectActiveRecipes: 레시피 DataTable이 비어있습니다."));
		return TArray<FName>();
	}

	// Random
	const int32 PickCount = FMath::Min(ActiveRecipeCount, Candidates.Num());
	TArray<FName> Result;
	for (int32 i = 0; i < PickCount; ++i)
	{
		const int32 RandomIndex = FMath::RandRange(0, Candidates.Num() - 1);
		Result.Add(Candidates[RandomIndex]);
		Candidates.RemoveAtSwap(RandomIndex);
	}
	return Result;
}

// 재료 투입 시 판정(요리시작/대기/망)
void AKCGameMode::OnIngredientSubmitted(FGameplayTag Channel, const FKCIngredientSubmittedStruct& Message)
{
	if (!IsMatchInProgress())
	{
		return;
	}

	ProcessIngredientSubmission(Message.TeamId, Message.IngredientId);
}

void AKCGameMode::ProcessIngredientSubmission(int32 TeamId, const FGameplayTag& IngredientId)
{
	if (!KCGameState)
	{
		return;
	}
	
	// 덮개 열리기 전 파밍 못하게
	if (!KCGameState->IsFarmingOpen())
	{
		UE_LOG(LogTemp, Warning, TEXT("덮개가 열리기 전 재료 습득 시도 감지"));
		return;
	}

	// 지금까지 투입된 재료에 이번 재료를 투입
	FGameplayTagContainer CurrentIngredients = KCGameState->GetPotIngredients(TeamId);
	CurrentIngredients.AddTag(IngredientId);
	
	// 1. 일치할 수 있는 레시피가 없으면 망한요리
	if (!HasAnyViableRecipe(CurrentIngredients))
	{
		KCGameState->SetPotIngredients(TeamId, FGameplayTagContainer());

		// TODO: 페널티(점수 차감 등) 확장 시 여기서

		Multicast_NotifyDishRuined(TeamId);

		UE_LOG(LogTemp, Log, TEXT("Team %d: 요리 실패 (유효한 레시피 없음)"), TeamId);
		return;
	}

	// 2. 완성되는 레시피가 있으면 -> 냄비에 요리 시작 Broadcast
	FName CompletedRecipeRowName;
	if (FindCompletedRecipe(CurrentIngredients, CompletedRecipeRowName))
	{
		KCGameState->SetPotIngredients(TeamId, FGameplayTagContainer());

		Multicast_NotifyRecipeCompleted(TeamId, CompletedRecipeRowName);

		UE_LOG(LogTemp, Log, TEXT("Team %d: 레시피 '%s' 완성, 조리 시작"), TeamId, *CompletedRecipeRowName.ToString());
		return;
	}

	// 3. 그 외 -> 재료 유지하고 계속 대기
	KCGameState->SetPotIngredients(TeamId, CurrentIngredients);
}

// 요리 망했는지 체크
bool AKCGameMode::HasAnyViableRecipe(const FGameplayTagContainer& CurrentIngredients) const
{
	if (!KCGameState)
	{
		return false;
	}

	for (const FName& RowName : KCGameState->GetActiveRecipes())
	{
		const FKCRecipeStruct* Recipe = KCGameState->FindRecipeByRowName(RowName);
		if (!Recipe)
		{
			continue;
		}

		FGameplayTagContainer RequiredTags;
		for (const FGameplayTag& Tag : Recipe->RequiredIngredients)
		{
			RequiredTags.AddTag(Tag);
		}

		if (RequiredTags.HasAll(CurrentIngredients))
		{
			return true;
		}
	}

	return false;
}

// 이번에 투입된 재료로 완성되는 레시피가 무엇인지 체크
bool AKCGameMode::FindCompletedRecipe(const FGameplayTagContainer& CurrentIngredients, FName& OutRecipeRowName) const
{
	if (!KCGameState)
	{
		return false;
	}

	for (const FName& RowName : KCGameState->GetActiveRecipes())
	{
		const FKCRecipeStruct* Recipe = KCGameState->FindRecipeByRowName(RowName);
		if (!Recipe)
		{
			continue;
		}

		// 재료는 종류당 1개씩이라는 사실로부터 거름망
		if (Recipe->RequiredIngredients.Num() != CurrentIngredients.Num())
		{
			continue;
		}

		FGameplayTagContainer RequiredTags;
		for (const FGameplayTag& Tag : Recipe->RequiredIngredients)
		{
			RequiredTags.AddTag(Tag);
		}

		if (CurrentIngredients.HasAll(RequiredTags))
		{
			OutRecipeRowName = RowName;
			return true;
		}
	}

	return false;
}

// 요리 완성 이벤트 받아서 점수 반영
void AKCGameMode::OnDishFinished(FGameplayTag Channel, const FKCDishFinishedStruct& Message)
{
	if (!IsMatchInProgress() || !KCGameState)
	{
		return;
	}

	const FKCRecipeStruct* Recipe = KCGameState->FindRecipeByRowName(Message.RecipeRowName);
	if (!Recipe)
	{
		UE_LOG(LogTemp, Warning, TEXT("OnDishFinished: 레시피 '%s'를 찾을 수 없습니다."), *Message.RecipeRowName.ToString());
		return;
	}

	const int32 NewScore = KCGameState->GetTeamScore(Message.TeamId) + Recipe->GetScoreValue();
	KCGameState->SetTeamScore(Message.TeamId, NewScore);

	UE_LOG(LogTemp, Log, TEXT("Team %d: 요리 완성 (+%d점, 총 %d점)"), Message.TeamId, Recipe->GetScoreValue(), NewScore);
	// 승리 체크
	CheckWinCondition();
}

// 승리 체크
bool AKCGameMode::IsTargetScoreReached(int32& OutWinningTeamId) const
{
	if (!KCGameState)
	{
		return false;
	}

	for (int32 TeamId = 0; TeamId < TeamCount; ++TeamId)
	{
		if (KCGameState->GetTeamScore(TeamId) >= TargetScore)
		{
			OutWinningTeamId = TeamId;
			return true;
		}
	}

	return false;
}

void AKCGameMode::CheckWinCondition()
{
	// TODO: 콜드게임 조건이 애매해짐

	int32 WinningTeamId = INDEX_NONE;

	if (IsTargetScoreReached(WinningTeamId))
	{
		EndGame(WinningTeamId);
	}
}

void AKCGameMode::EndGame(int32 WinningTeamId)
{
	// 결과화면 시작할 때 스킵 누른 인원 목록 초기화
	SkippedResultScreenPlayers.Reset();
	
	if (KCGameState)
	{
		KCGameState->SetGamePhase(EKCGamePhaseType::Ending);
	}

	// 게임 승리 로그
	UE_LOG(LogTemp, Log, TEXT("Game Ended. Winning Team: %d"), WinningTeamId);

	EndMatch();
	
	GetWorldTimerManager().SetTimer(
		ResultScreenTimerHandle,
		this,
		&AKCGameMode::TravelBackToLobby,
		ResultScreenDuration,
		false
	);
}

// TODO: 임시코드
// 게임 끝내기 로직
void AKCGameMode::HandleMatchTimeExpired()
{
	if (!IsMatchInProgress())
	{
		return;
	}

	EndGame(GetLeadingTeamId());
}

// TODO: 임시코드
// 게임 끝날 때 이긴 팀 계산을 위한 로직
// 팀이 여러개라는 가정하에 코드 작성
int32 AKCGameMode::GetLeadingTeamId() const
{
	if (!KCGameState)
	{
		return INDEX_NONE;
	}

	int32 LeadingTeamId = INDEX_NONE;
	int32 LeadingScore = MIN_int32;
	bool bTie = false;

	for (int32 TeamId = 0; TeamId < TeamCount; ++TeamId)
	{
		const int32 Score = KCGameState->GetTeamScore(TeamId);
		if (Score > LeadingScore)
		{
			LeadingScore = Score;
			LeadingTeamId = TeamId;
			bTie = false;
		}
		else if (Score == LeadingScore)
		{
			bTie = true;
		}
	}

	return bTie ? INDEX_NONE : LeadingTeamId;
}

void AKCGameMode::TravelBackToLobby()
{
	// 트래블 직전, 아직 로딩화면 안 뜬 사람들(전원)한테 방송
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AKCPlayerController* KCPC = Cast<AKCPlayerController>(It->Get()))
		{
			KCPC->Client_ShowResultToLobbyLoadingScreen();
		}
	}
	GetWorld()->ServerTravel(UKCLevelTypeLibrary::GetLevelName(EKCLevelType::LobbyLevel).ToString());
}

void AKCGameMode::Multicast_NotifyDishRuined_Implementation(int32 TeamId)
{
	FKCDishRuinedStruct Message;
	Message.TeamId = TeamId;
	UGameplayMessageSubsystem::Get(this).BroadcastMessage(KCGameplayTags::Message_Dish_Ruined, Message);
}

void AKCGameMode::Multicast_NotifyRecipeCompleted_Implementation(int32 TeamId, FName RecipeRowName)
{
	FKCRecipeCompletedStruct Message;
	Message.TeamId = TeamId;
	Message.RecipeRowName = RecipeRowName;
	UGameplayMessageSubsystem::Get(this).BroadcastMessage(KCGameplayTags::Message_Recipe_Completed, Message);
}

// 플레이어 이탈 시 팀 및 슬롯 정보 백업
void AKCGameMode::Logout(AController* Exiting)
{
	if (Exiting)
	{
		if (AKCPlayerState* KCPS = Exiting->GetPlayerState<AKCPlayerState>())
		{
			// 세션 서브시스템에 팀/슬롯 영구 백업 (UniqueNetId 키 기반)
			if (UKCSessionSubsystem* SessionSub = GetGameInstance()->GetSubsystem<UKCSessionSubsystem>())
			{
				SessionSub->SaveLobbyPlayerData(KCPS->GetUniquePlayerIdString(), KCPS->GetPlayerName(), KCPS->GetTeamId(), KCPS->GetSlotIndex());
				UE_LOG(LogKCLobby, Warning, TEXT("[GasRange] 플레이어 이탈: UniqueId='%s', Name='%s', TeamId=%d, SlotIndex=%d (정보 백업 완료)"),
					*KCPS->GetUniquePlayerIdString(), *KCPS->GetPlayerName(), KCPS->GetTeamId(), KCPS->GetSlotIndex());
			}
		}
	}

	Super::Logout(Exiting);
}

void AKCGameMode::Debug_SubmitIngredient(int32 TeamId, FString IngredientTagName)
{
	const FGameplayTag IngredientTag = FGameplayTag::RequestGameplayTag(FName(*IngredientTagName), false);
	if (!IngredientTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Debug_SubmitIngredient: '%s'는 유효한 태그가 아닙니다."), *IngredientTagName);
		return;
	}

	ProcessIngredientSubmission(TeamId, IngredientTag);
}

void AKCGameMode::Debug_FinishDish(int32 TeamId, FName RecipeRowName)
{
	FKCDishFinishedStruct FakeMessage;
	FakeMessage.TeamId = TeamId;
	FakeMessage.RecipeRowName = RecipeRowName;

	OnDishFinished(KCGameplayTags::Message_Dish_Finished, FakeMessage);
}

void AKCGameMode::Debug_WinMatch(int32 WinningTeamId)
{
	UE_LOG(LogTemp, Log, TEXT("[Debug] Team %d 즉시 승리 처리"), WinningTeamId);
	EndGame(WinningTeamId);
}
// 테스트용 레시피 고정 옵션 구현함수
TArray<FName> AKCGameMode::GetRecipeRowNameOptions() const
{
	return DebugRecipeDataTable ? DebugRecipeDataTable->GetRowNames() : TArray<FName>();
}

void AKCGameMode::RequestEarlyTravelToLobby(AKCPlayerState* RequestingPlayer)
{
	if (!GetWorldTimerManager().IsTimerActive(ResultScreenTimerHandle))
	{
		return;   // 결과화면 자체가 안 떠있는 상태 (이미 트래블됐거나 아직 시작하지 않음)
	}

	if (!RequestingPlayer)
	{
		return; // PlayerState 유효성 검사
	}

	SkippedResultScreenPlayers.Add(RequestingPlayer); // 누른 플레이어를 배열에 추가
	
	// 스킵을 누른 그 사람한테만 즉시 로딩화면 표시
	if (AKCPlayerController* RequestingPC = Cast<AKCPlayerController>(RequestingPlayer->GetOwningController()))
	{
		RequestingPC->Client_ShowResultToLobbyLoadingScreen();
	}

	// 지금 접속해있는 전원(스펙테이터 제외하고 싶으면 조건 추가 가능)이 다 스킵했는지 확인
	const int32 ConnectedPlayerCount = GetNumPlayers(); // 현재 서버에 접속한 인원 수
	if (SkippedResultScreenPlayers.Num() >= ConnectedPlayerCount) // 스킵 누른 사람 수 == 접속 인원 수
	{
		GetWorldTimerManager().ClearTimer(ResultScreenTimerHandle); // 10초 타이머 취소
		TravelBackToLobby(); // 트래블 실행
	}
	// 안 누른 사람이 한 명이라도 있으면 그대로 함수 종료 -> 10초 타이머가 알아서 트래블
}


void AKCGameMode::RestoreSlotDataForController(AController* Controller)
{
	AKCPlayerState* KCPS = Controller ? Controller->GetPlayerState<AKCPlayerState>() : nullptr;
	if (!KCPS)
	{
		UE_LOG(LogKCLobby, Warning, TEXT("[Spawn] RestoreSlotDataForController 중단 - KCPS가 null. Controller=%s"),
			Controller ? *Controller->GetName() : TEXT("null"));
		return;
	}

	UKCSessionSubsystem* SessionSub = GetGameInstance()->GetSubsystem<UKCSessionSubsystem>();
	if (!SessionSub)
	{
		UE_LOG(LogKCLobby, Warning, TEXT("[Spawn] RestoreSlotDataForController 중단 - SessionSub가 null."));
		return;
	}

	const FString NetIdStr = KCPS->GetUniquePlayerIdString();
	const FString QueryKey = !NetIdStr.IsEmpty() ? NetIdStr : KCPS->GetPlayerName();

	UE_LOG(LogKCLobby, Warning, TEXT("[Spawn] RestoreSlotDataForController 진행 중 - NetIdStr='%s', PlayerName='%s', QueryKey='%s'"),
		*NetIdStr, *KCPS->GetPlayerName(), *QueryKey);

	if (QueryKey.IsEmpty())
	{
		UE_LOG(LogKCLobby, Warning, TEXT("[Spawn] RestoreSlotDataForController 중단 - QueryKey가 비어있음."));
		return;
	}

	FString SavedPlayerName;
	int32 SavedTeamId = 0;
	int32 SavedSlotIndex = INDEX_NONE;

	if (SessionSub->GetSavedLobbyPlayerData(QueryKey, SavedPlayerName, SavedTeamId, SavedSlotIndex))
	{
		KCPS->SetGamePlayerName(SavedPlayerName);
		KCPS->SetTeamId(SavedTeamId);
		KCPS->SetSlotIndex(SavedSlotIndex);
		UE_LOG(LogKCLobby, Log, TEXT("[GasRange][spawn] 슬롯 데이터 복원: Key='%s', Name='%s', TeamId=%d, SlotIndex=%d"),
			*QueryKey, *SavedPlayerName, SavedTeamId, SavedSlotIndex);
	}
	else
	{
		UE_LOG(LogKCLobby, Log, TEXT("[GasRange][spawn] 세션 백업 데이터 없음: Key='%s'"), *QueryKey);
	}
}

bool AKCGameMode::UpdatePlayerStartSpot(AController* Player, const FString& Portal, FString& OutErrorMessage)
{
	UE_LOG(LogKCLobby, Warning, TEXT("[Spawn] UpdatePlayerStartSpot 진입, Player=%s"), Player ? *Player->GetName() : TEXT("null"));

	RestoreSlotDataForController(Player);

	UE_LOG(LogKCLobby, Warning, TEXT("[Spawn] UpdatePlayerStartSpot 완료, Player=%s"), Player ? *Player->GetName() : TEXT("null"));

	return Super::UpdatePlayerStartSpot(Player, Portal, OutErrorMessage);
}

void AKCGameMode::InitSeamlessTravelPlayer(AController* NewController)
{
	UE_LOG(LogKCLobby, Warning, TEXT("[KC_DEBUG6] InitSeamlessTravelPlayer 진입, Player=%s"),
		NewController ? *NewController->GetName() : TEXT("null"));

	RestoreSlotDataForController(NewController); // slot -1 playerid 261 teamid -1 상태로 restore~ 실행

	UE_LOG(LogKCLobby, Warning, TEXT("[KC_DEBUG6] InitSeamlessTravelPlayer 완료, Player=%s"),
		NewController ? *NewController->GetName() : TEXT("null"));

	Super::InitSeamlessTravelPlayer(NewController);
}

AActor* AKCGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	const AKCPlayerState* KCPlayerState = Player ? Player->GetPlayerState<AKCPlayerState>() : nullptr;

	UE_LOG(LogKCLobby, Warning, TEXT("[KC_DEBUG2] ChoosePlayerStart 진입, Player=%s, PlayerState 주소: %p"),
		Player ? *Player->GetName() : TEXT("null"), KCPlayerState);

	if (!KCPlayerState)
	{
		UE_LOG(LogKCLobby, Warning, TEXT("[Spawn] PlayerState 없음. 기본 시작점 선택."));
		return Super::ChoosePlayerStart_Implementation(Player);
	}

	const int32 SlotIndex = KCPlayerState->GetSlotIndex();
	if (SlotIndex < 0 || SlotIndex >= AKCLobbyGameMode::REGULAR_SLOT_COUNT)
	{
		UE_LOG(LogKCLobby, Warning, TEXT("[Spawn] Player='%s', 유효하지 않은 SlotIndex=%d. 기본 시작점 선택."),
			*KCPlayerState->GetPlayerName(), SlotIndex);
		return Super::ChoosePlayerStart_Implementation(Player);
	}

	const FName TargetStartTag(*FString::Printf(TEXT("Slot_%d"), SlotIndex));

	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		APlayerStart* Start = *It;
		if (Start->PlayerStartTag == TargetStartTag)
		{
			UE_LOG(LogKCLobby, Log, TEXT("[Spawn] Player='%s', SlotIndex=%d, PlayerStart='%s', Tag='%s'"),
				*KCPlayerState->GetPlayerName(), SlotIndex, *Start->GetName(), *TargetStartTag.ToString());
			return Start;
		}
	}

	UE_LOG(LogKCLobby, Warning, TEXT("[Spawn] PlayerStartTag='%s' 없음. 기본 시작점 선택."), *TargetStartTag.ToString());
	return Super::ChoosePlayerStart_Implementation(Player);
}