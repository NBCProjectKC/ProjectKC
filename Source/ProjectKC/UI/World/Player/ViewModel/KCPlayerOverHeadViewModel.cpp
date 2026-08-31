#include "ProjectKC/UI/World/Player/ViewModel/KCPlayerOverHeadViewModel.h"

void UKCPlayerOverHeadViewModel::SetPlayerName(const FText& NewPlayerName)
{
	if (PlayerName.EqualTo(NewPlayerName))
	{
		return;
	}

	PlayerName = NewPlayerName;
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(PlayerName);
	OnPlayerNameChangedNative.Broadcast(PlayerName);
}

void UKCPlayerOverHeadViewModel::SetTeamId(int32 NewTeamId)
{
	if (TeamId == NewTeamId)
	{
		return;
	}

	TeamId = NewTeamId;
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(TeamId);
	OnTeamIdChangedNative.Broadcast(TeamId);
}

void UKCPlayerOverHeadViewModel::SetOverHeadVisible(bool bNewVisible)
{
	if (bOverHeadVisible == bNewVisible)
	{
		return;
	}

	bOverHeadVisible = bNewVisible;
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(bOverHeadVisible);
	OnVisibilityChangedNative.Broadcast(bOverHeadVisible);
}
