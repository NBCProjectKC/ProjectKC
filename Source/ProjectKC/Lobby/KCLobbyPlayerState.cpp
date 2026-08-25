/**
 * @file KCLobbyPlayerState.cpp
 * @brief AKCLobbyPlayerState 구현부
 */

#include "ProjectKC/Lobby/KCLobbyPlayerState.h"
#include "Net/UnrealNetwork.h"

AKCLobbyPlayerState::AKCLobbyPlayerState()
{
	bReplicates = true;
}

void AKCLobbyPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AKCLobbyPlayerState, bReady);
	DOREPLIFETIME(AKCLobbyPlayerState, TeamId);
}

void AKCLobbyPlayerState::SetTeamId(int32 InTeamId)
{
	if (HasAuthority())
	{
		TeamId = InTeamId;
		OnRep_TeamId();
	}
}

void AKCLobbyPlayerState::SetIsReady(bool bNewReady)
{
	if (HasAuthority())
	{
		bReady = bNewReady;
		OnRep_Ready();
	}
}

void AKCLobbyPlayerState::ToggleReady()
{
	if (HasAuthority())
	{
		SetIsReady(!bReady);
	}
}

void AKCLobbyPlayerState::OnRep_Ready()
{
	OnReadyStatusChanged.Broadcast(bReady);
}

void AKCLobbyPlayerState::OnRep_TeamId()
{
	OnTeamIdChanged.Broadcast(TeamId);
}
