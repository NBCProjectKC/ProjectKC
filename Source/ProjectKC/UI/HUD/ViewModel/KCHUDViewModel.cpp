#include "ProjectKC/UI/HUD/ViewModel/KCHUDViewModel.h"

#include "ProjectKC/Messages/KCGameplayTags.h"
#include "ProjectKC/Messages/Struct/KCGamePhaseChangedStruct.h"
#include "ProjectKC/Messages/Struct/KCScoreChangedStruct.h"

void UKCHUDViewModel::StartListening(UObject* WorldContextObject)
{
	StopListening();

	if (!WorldContextObject)
	{
		return;
	}

	ListeningWorldContext = WorldContextObject;
	UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(WorldContextObject);

	ScoreChangedHandle = MessageSystem.RegisterListener<FKCScoreChangedStruct>(
		KCGameplayTags::Message_Game_ScoreChanged,
		this,
		&ThisClass::HandleScoreChanged);

	PhaseChangedHandle = MessageSystem.RegisterListener<FKCGamePhaseChangedStruct>(
		KCGameplayTags::Message_Game_PhaseChanged,
		this,
		&ThisClass::HandleGamePhaseChanged);
}

void UKCHUDViewModel::StopListening()
{
	ScoreChangedHandle.Unregister();
	PhaseChangedHandle.Unregister();
	ListeningWorldContext.Reset();
}

int32 UKCHUDViewModel::GetTeamScore(int32 TeamId) const
{
	return TeamScores.IsValidIndex(TeamId) ? TeamScores[TeamId] : 0;
}

void UKCHUDViewModel::HandleScoreChanged(FGameplayTag Channel, const FKCScoreChangedStruct& Message)
{
	if (Message.TeamId >= TeamScores.Num())
	{
		TeamScores.SetNum(Message.TeamId + 1);
	}

	TeamScores[Message.TeamId] = Message.NewScore;
	OnScoreChanged(Message.TeamId, Message.NewScore);
}

void UKCHUDViewModel::HandleGamePhaseChanged(FGameplayTag Channel, const FKCGamePhaseChangedStruct& Message)
{
	CurrentPhase = Message.NewPhase;
	OnGamePhaseChanged(CurrentPhase);
}
