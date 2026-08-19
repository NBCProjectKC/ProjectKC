#include "KCGameMode.h"
#include "KCGameState.h"
#include "KCGameSystemTags.h"
#include "KCIngredientSubmittedStruct.h"
#include "KCGamePhaseType.h"
#include "GameFramework/GameplayMessageSubsystem.h"

AKCGameMode::AKCGameMode()
{
}

void AKCGameMode::Debug_SubmitIngredient(int32 TeamId, int32 Count)
{
	FKCIngredientSubmittedStruct FakeMessage;
	FakeMessage.TeamId = TeamId;
	FakeMessage.SubmittedCount = Count;

	OnIngredientSubmitted(TAG_Event_Ingredient_Submitted, FakeMessage);
}

bool AKCGameMode::ReadyToStartMatch_Implementation()
{
	return GetNumPlayers() >= GetRequiredPlayerCount();
}

void AKCGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();
	
	KCGameState = GetGameState<AKCGameState>();

	IngredientSubmittedListenerHandle = UGameplayMessageSubsystem::Get(this).RegisterListener<FKCIngredientSubmittedStruct>(
		TAG_Event_Ingredient_Submitted,
		this,
		&AKCGameMode::OnIngredientSubmitted
	);

	if (KCGameState)
	{
		KCGameState->InitializeTeamCount(TeamCount);
		KCGameState->SetGamePhase(EKCGamePhaseType::Playing);
	}
}

void AKCGameMode::HandleMatchHasEnded()
{
	UGameplayMessageSubsystem::Get(this).UnregisterListener(IngredientSubmittedListenerHandle);

	Super::HandleMatchHasEnded();
}

void AKCGameMode::OnIngredientSubmitted(FGameplayTag Channel, const FKCIngredientSubmittedStruct& Message)
{
	if (!IsMatchInProgress())
	{
		return;
	}

	if (!KCGameState)
	{
		return;
	}

	const int32 CurrentScore = KCGameState->GetTeamScore(Message.TeamId);
	const int32 NewScore = CurrentScore + Message.SubmittedCount;

	KCGameState->SetTeamScore(Message.TeamId, NewScore);
	
	SubmittedIngredientCount += Message.SubmittedCount;

	CheckWinCondition();
}

bool AKCGameMode::IsTargetScoreReached(int32& OutWinningTeamId) const
{
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

bool AKCGameMode::IsColdGameTriggered(int32& OutWinningTeamId) const
{
	if (TeamCount < 2)
	{
		return false;
	}

	const int32 RemainingIngredientCount = TotalIngredientCount - SubmittedIngredientCount;
	
	int32 FirstPlaceTeamId = INDEX_NONE;
	int32 FirstPlaceScore = -1;
	int32 SecondPlaceScore = -1;

	for (int32 TeamId = 0; TeamId < TeamCount; ++TeamId)
	{
		const int32 Score = KCGameState->GetTeamScore(TeamId);
		if (Score > FirstPlaceScore)
		{
			SecondPlaceScore = FirstPlaceScore;
			FirstPlaceScore = Score;
			FirstPlaceTeamId = TeamId;
		}
		else if (Score > SecondPlaceScore)
		{
			SecondPlaceScore = Score;
		}
	}
	
	if (FirstPlaceScore - SecondPlaceScore > RemainingIngredientCount)
	{
		OutWinningTeamId = FirstPlaceTeamId;
		return true;
	}

	return false;
}

void AKCGameMode::CheckWinCondition()
{
	if (!KCGameState)
	{
		return;
	}

	int32 WinningTeamId = INDEX_NONE;
	
	if (IsTargetScoreReached(WinningTeamId))
	{
		EndGame(WinningTeamId);
		return;
	}
	
	if (IsColdGameTriggered(WinningTeamId))
	{
		EndGame(WinningTeamId);
		return;
	}
}

void AKCGameMode::EndGame(int32 WinningTeamId)
{
	if (KCGameState)
	{
		KCGameState->SetGamePhase(EKCGamePhaseType::Ended);
	}

	// TODO: 게임 종료 후 처리
	UE_LOG(LogTemp, Log, TEXT("Game Ended. Winning Team: %d"), WinningTeamId);
	
	EndMatch();
}
