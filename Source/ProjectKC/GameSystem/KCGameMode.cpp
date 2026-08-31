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


AKCGameMode::AKCGameMode()
{
	bUseSeamlessTravel = true;
	
	DefaultPawnClass = AKCPlayerCharacter::StaticClass();
	PlayerControllerClass = AKCPlayerController::StaticClass();
	PlayerStateClass = AKCPlayerState::StaticClass();
	HUDClass = nullptr;
}

// 매치 흐름
bool AKCGameMode::ReadyToStartMatch_Implementation()
{
	// 인원 재확인
	return GetNumPlayers() >= GetRequiredPlayerCount();
}

void AKCGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

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
	if (KCGameState)
	{
		KCGameState->SetGamePhase(EKCGamePhaseType::Ending);
	}

	// TODO: 게임 종료 후 처리(결과 화면, 로비 복귀)
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

// 재접속 시 팀 및 슬롯 정보 복원
void AKCGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (!NewPlayer)
	{
		return;
	}

	AKCPlayerState* KCPS = NewPlayer->GetPlayerState<AKCPlayerState>();
	if (!KCPS)
	{
		return;
	}

	// UniqueNetId 우선 조회 (스팀 접속 첫 프레임부터 100% 즉시 복원)
	if (UKCSessionSubsystem* SessionSub = GetGameInstance()->GetSubsystem<UKCSessionSubsystem>())
	{
		const FString NetIdStr = KCPS->GetUniquePlayerIdString();
		const FString QueryKey = !NetIdStr.IsEmpty() ? NetIdStr : KCPS->GetPlayerName();

		FString SavedPlayerName;
		int32 SavedTeamId = 0;
		int32 SavedSlotIndex = INDEX_NONE;

		if (!QueryKey.IsEmpty() && SessionSub->GetSavedLobbyPlayerData(QueryKey, SavedPlayerName, SavedTeamId, SavedSlotIndex))
		{
			KCPS->SetGamePlayerName(SavedPlayerName);
			KCPS->SetTeamId(SavedTeamId);
			KCPS->SetSlotIndex(SavedSlotIndex);
			UE_LOG(LogKCLobby, Log, TEXT("[GasRange] 플레이어 재접속: Key='%s', Name='%s', TeamId=%d, SlotIndex=%d 즉시 복원 성공"),
				*QueryKey, *SavedPlayerName, SavedTeamId, SavedSlotIndex);
		}
		else
		{
			UE_LOG(LogKCLobby, Log, TEXT("[GasRange] 플레이어 접속: Key='%s', 세션 백업 데이터 없음 (신규 난입)"),
				*QueryKey);
		}
	}
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
