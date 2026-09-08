#include "ProjectKC/UI/Result/ViewModel/KCResultViewModel.h"

void UKCResultViewModel::SetTeams(const TArray<FKCResultTeamViewData>& NewTeams)
{
	Teams = NewTeams;
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Teams);
}

void UKCResultViewModel::SetRemainingBackToLobbySeconds(int32 NewRemainingSeconds)
{
	NewRemainingSeconds = FMath::Max(0, NewRemainingSeconds);
	if (RemainingBackToLobbySeconds == NewRemainingSeconds && !RemainingBackToLobbyText.IsEmpty())
	{
		return;
	}

	RemainingBackToLobbySeconds = NewRemainingSeconds;
	RemainingBackToLobbyText = MakeBackToLobbyText(RemainingBackToLobbySeconds);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(RemainingBackToLobbySeconds);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(RemainingBackToLobbyText);
}

void UKCResultViewModel::SetPreviewData(const TArray<FKCResultTeamViewData>& NewTeams)
{
	SetTeams(NewTeams);
}

FText UKCResultViewModel::MakeBackToLobbyText(int32 RemainingSeconds)
{
	return FText::Format(NSLOCTEXT("KCResultViewModel", "BackToLobbySecondsFormat", "{0}s"), RemainingSeconds);
}
