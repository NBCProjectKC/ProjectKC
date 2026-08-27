#include "ProjectKC/UI/Lobby/ViewModel/KCLobbyViewModel.h"

void UKCLobbyViewModel::SetPlayers(const TArray<FKCLobbyPlayerViewData>& NewPlayers)
{
	Players = NewPlayers;
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Players);
}

void UKCLobbyViewModel::SetInviteEnabled(bool bNewInviteEnabled)
{
	UE_MVVM_SET_PROPERTY_VALUE(bInviteEnabled, bNewInviteEnabled);
}

void UKCLobbyViewModel::SetPreviewData(const TArray<FKCLobbyPlayerViewData>& NewPlayers, bool bNewInviteEnabled)
{
	SetPlayers(NewPlayers);
	SetInviteEnabled(bNewInviteEnabled);
}
