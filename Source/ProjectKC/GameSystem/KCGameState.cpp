#include "KCGameState.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Messages/Struct/KCGamePhaseChangedStruct.h"
#include "Messages/Struct/KCScoreChangedStruct.h"
#include "Messages/KCGameplayTags.h"

AKCGameState::AKCGameState()
{
}

void AKCGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AKCGameState, CurrentPhase);
	DOREPLIFETIME(AKCGameState, TeamScores);
}

void AKCGameState::InitializeTeamCount(int32 InTeamCount)
{
	if (!HasAuthority())
	{
		return;
	}

	TeamScores.Init(0, InTeamCount);
}

void AKCGameState::SetGamePhase(EKCGamePhaseType NewPhase)
{
	if (!HasAuthority())
	{
		return;
	}

	if (CurrentPhase == NewPhase)
	{
		return;
	}

	CurrentPhase = NewPhase;
	
	OnRep_CurrentPhase();
}

void AKCGameState::SetTeamScore(int32 TeamId, int32 NewScore)
{
	if (!HasAuthority())
	{
		return;
	}

	if (!TeamScores.IsValidIndex(TeamId))
	{
		return;
	}

	TeamScores[TeamId] = NewScore;
	OnRep_TeamScores();
}

int32 AKCGameState::GetTeamScore(int32 TeamId) const
{
	return TeamScores.IsValidIndex(TeamId) ? TeamScores[TeamId] : 0;
}

void AKCGameState::OnRep_CurrentPhase()
{
	FKCGamePhaseChangedStruct Message;
	Message.NewPhase = CurrentPhase;

	UGameplayMessageSubsystem::Get(this).BroadcastMessage(KCGameplayTags::Message_Game_PhaseChanged, Message);
}

void AKCGameState::OnRep_TeamScores()
{
	for (int32 TeamId = 0; TeamId < TeamScores.Num(); ++TeamId)
	{
		FKCScoreChangedStruct Message;
		Message.TeamId = TeamId;
		Message.NewScore = TeamScores[TeamId];

		UGameplayMessageSubsystem::Get(this).BroadcastMessage(KCGameplayTags::Message_Game_ScoreChanged, Message);
	}
}
