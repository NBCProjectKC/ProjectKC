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
#include "ProjectKC/Lobby/KCLobbyPlayerState.h"
#include "Player/KCPlayerCharacter.h"
#include "Player/KCPlayerController.h"
#include "KCLevelTypeLibrary.h"
#include "ProjectKC/Lobby/KCSessionSubsystem.h"
#include "ProjectKC/Lobby/KCLobbyPlayerState.h"


AKCGameMode::AKCGameMode()
{
	bUseSeamlessTravel = true;
	
	DefaultPawnClass = AKCPlayerCharacter::StaticClass();
	PlayerControllerClass = AKCPlayerController::StaticClass();
	PlayerStateClass = AKCLobbyPlayerState::StaticClass();
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
		KCGameState->SetActiveRecipes(SelectActiveRecipes());
		KCGameState->SetGamePhase(EKCGamePhaseType::Playing);
	}
}

void AKCGameMode::HandleMatchHasEnded()
{
	UGameplayMessageSubsystem::Get(this).UnregisterListener(IngredientSubmittedListenerHandle);
	UGameplayMessageSubsystem::Get(this).UnregisterListener(DishFinishedListenerHandle);

	Super::HandleMatchHasEnded();
}

// 레시피 선정 로직
TArray<FName> AKCGameMode::SelectActiveRecipes() const
{
	TArray<FName> Result;

	if (!RecipeDataTable)
	{
		UE_LOG(LogTemp, Error, TEXT("SelectActiveRecipes: RecipeDataTable이 설정되지 않았습니다."));
		return Result;
	}

	TArray<FName> Candidates = RecipeDataTable->GetRowNames();
	if (Candidates.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("SelectActiveRecipes: RecipeDataTable이 비어있습니다."));
		return Result;
	}
	
	// Debug
	const int32 Seed = bUseFixedRecipeSeed
		? FixedRecipeSeed
		: static_cast<int32>(FDateTime::Now().GetTicks());
	FRandomStream RandomStream(Seed);
	
	// random
	const int32 PickCount = FMath::Min(ActiveRecipeCount, Candidates.Num());
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
		const FKCRecipeStruct* Recipe = FindRecipeByRowName(RowName);
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
		const FKCRecipeStruct* Recipe = FindRecipeByRowName(RowName);
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

	const FKCRecipeStruct* Recipe = FindRecipeByRowName(Message.RecipeRowName);
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


const FKCRecipeStruct* AKCGameMode::FindRecipeByRowName(FName RowName) const
{
	if (!RecipeDataTable)
	{
		return nullptr;
	}

	return RecipeDataTable->FindRow<FKCRecipeStruct>(RowName, TEXT("FindRecipeByRowName"));
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

// 플레이어 이탈 시 정보 백업
void AKCGameMode::Logout(AController* Exiting)
{
	if (Exiting)
	{
		if (AKCLobbyPlayerState* KCPS = Exiting->GetPlayerState<AKCLobbyPlayerState>())
		{
			if (UKCSessionSubsystem* SessionSub = GetGameInstance()->GetSubsystem<UKCSessionSubsystem>())
			{
				SessionSub->SaveLobbyPlayerData(KCPS->GetPlayerName(), KCPS->GetTeamId(), KCPS->GetSlotIndex());
				UE_LOG(LogTemp, Warning, TEXT("[GasRange] 플레이어 이탈: %s, TeamId=%d, SlotIndex=%d 자리 보존"),
					*KCPS->GetPlayerName(), KCPS->GetTeamId(), KCPS->GetSlotIndex());
			}
		}
	}

	Super::Logout(Exiting);
}
// 재접속 시 정보 복원
void AKCGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (!NewPlayer)
	{
		return;
	}

	AKCLobbyPlayerState* KCPS = NewPlayer->GetPlayerState<AKCLobbyPlayerState>();
	if (!KCPS)
	{
		return;
	}

	if (UKCSessionSubsystem* SessionSub = GetGameInstance()->GetSubsystem<UKCSessionSubsystem>())
	{
		int32 SavedTeamId = 0;
		int32 SavedSlotIndex = INDEX_NONE;
		if (SessionSub->GetSavedLobbyPlayerData(KCPS->GetPlayerName(), SavedTeamId, SavedSlotIndex))
		{
			KCPS->SetTeamId(SavedTeamId);
			KCPS->SetSlotIndex(SavedSlotIndex);
			UE_LOG(LogTemp, Log, TEXT("[GasRange] 플레이어 재접속: %s, TeamId=%d, SlotIndex=%d 복원 (명시적 백업)"),
				*KCPS->GetPlayerName(), SavedTeamId, SavedSlotIndex);
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("[GasRange] 플레이어 접속: %s, 저장된 데이터 없음 (CopyProperties 자동복원에 의존)"),
				*KCPS->GetPlayerName());
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
