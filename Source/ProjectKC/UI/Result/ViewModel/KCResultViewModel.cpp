#include "ProjectKC/UI/Result/ViewModel/KCResultViewModel.h"

void UKCResultViewModel::SetTeams(const TArray<FKCResultTeamViewData>& NewTeams)
{
	Teams = NewTeams;
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Teams);
}

void UKCResultViewModel::SetRemainingBackToLobbySeconds(int32 NewRemainingSeconds)
{
	NewRemainingSeconds = FMath::Max(0, NewRemainingSeconds);
	if (RemainingBackToLobbySeconds == NewRemainingSeconds && !RemainingBackToLobbySecondText.IsEmpty())
	{
		return;
	}

	RemainingBackToLobbySeconds = NewRemainingSeconds;
	RemainingBackToLobbySecondText = MakeRemainingSecondText(RemainingBackToLobbySeconds);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(RemainingBackToLobbySeconds);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(RemainingBackToLobbySecondText);
}

void UKCResultViewModel::SetWinningTeamId(int32 NewWinningTeamId)
{
	if (WinningTeamId == NewWinningTeamId && !WinningTeamText.IsEmpty())
	{
		return;
	}

	WinningTeamId = NewWinningTeamId;
	WinningTeamText = MakeWinningTeamText(WinningTeamId);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(WinningTeamId);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(WinningTeamText);
}

void UKCResultViewModel::SetPreviewData(const TArray<FKCResultTeamViewData>& NewTeams)
{
	SetTeams(NewTeams);
}

FText UKCResultViewModel::MakeRemainingSecondText(int32 RemainingSeconds)
{
	return FText::AsNumber(RemainingSeconds);
}

FText UKCResultViewModel::MakeWinningTeamText(int32 InWinningTeamId)
{
	if (InWinningTeamId == INDEX_NONE)
	{
		return NSLOCTEXT("KCResultViewModel", "DrawResult", "Draw");
	}

	return FText::Format(NSLOCTEXT("KCResultViewModel", "WinningTeamFormat", "Team {0} Win"), InWinningTeamId + 1);
}
