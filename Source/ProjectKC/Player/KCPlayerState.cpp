/**
 * @file KCPlayerState.cpp
 * @brief AKCPlayerState 구현부
 */

#include "ProjectKC/Player/KCPlayerState.h"
#include "ProjectKC/Lobby/KCSessionSubsystem.h"
#include "ProjectKC/ProjectKC.h"
#include "Net/UnrealNetwork.h"

AKCPlayerState::AKCPlayerState()
{
	bReplicates = true;
}

void AKCPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AKCPlayerState, bReady);
	DOREPLIFETIME(AKCPlayerState, TeamId);
	DOREPLIFETIME(AKCPlayerState, SlotIndex);
}

void AKCPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	if (AKCPlayerState* TargetPS = Cast<AKCPlayerState>(PlayerState))
	{
		UE_LOG(LogKCLobby, Log, TEXT("[KCPlayerState] CopyProperties: Copying TeamId=%d, SlotIndex=%d to new PlayerState '%s'"),
			this->TeamId, this->SlotIndex, *TargetPS->GetName());
		TargetPS->SetTeamId(this->TeamId);
		TargetPS->SetSlotIndex(this->SlotIndex);
	}
}

void AKCPlayerState::OverrideWith(APlayerState* PlayerState)
{
	Super::OverrideWith(PlayerState);

	if (AKCPlayerState* InactivePS = Cast<AKCPlayerState>(PlayerState))
	{
		UE_LOG(LogKCLobby, Log, TEXT("[KCPlayerState] OverrideWith: Restoring Inactive PlayerState '%s' (TeamId=%d, SlotIndex=%d)"),
			*InactivePS->GetPlayerName(), InactivePS->TeamId, InactivePS->SlotIndex);

		SetTeamId(InactivePS->TeamId);
		SetSlotIndex(InactivePS->SlotIndex);
	}
}

void AKCPlayerState::SetSlotIndex(int32 InSlotIndex)
{
	if (HasAuthority() && SlotIndex != InSlotIndex)
	{
		UE_LOG(LogKCLobby, Verbose, TEXT("[KCPlayerState] '%s' SetSlotIndex: %d -> %d"), *GetPlayerName(), SlotIndex, InSlotIndex);
		SlotIndex = InSlotIndex;
		OnRep_SlotIndex();
		ForceNetUpdate();
	}
}

void AKCPlayerState::SetTeamId(int32 InTeamId)
{
	if (HasAuthority() && TeamId != InTeamId)
	{
		UE_LOG(LogKCLobby, Verbose, TEXT("[KCPlayerState] '%s' SetTeamId: %d -> %d"), *GetPlayerName(), TeamId, InTeamId);
		TeamId = InTeamId;
		OnRep_TeamId();
		ForceNetUpdate();
	}
}

void AKCPlayerState::SetIsReady(bool bNewReady)
{
	if (HasAuthority() && bReady != bNewReady)
	{
		UE_LOG(LogKCLobby, Log, TEXT("[KCPlayerState] '%s' SetIsReady: %s -> %s"),
			*GetPlayerName(), bReady ? TEXT("TRUE") : TEXT("FALSE"), bNewReady ? TEXT("TRUE") : TEXT("FALSE"));
		bReady = bNewReady;
		OnRep_Ready();
		ForceNetUpdate();
	}
}

void AKCPlayerState::ToggleReady()
{
	if (HasAuthority())
	{
		SetIsReady(!bReady);
	}
}

void AKCPlayerState::OnRep_Ready()
{
	UE_LOG(LogKCLobby, Verbose, TEXT("[KCPlayerState] OnRep_Ready - Player: '%s', Ready: %s"),
		*GetPlayerName(), bReady ? TEXT("TRUE") : TEXT("FALSE"));
	OnReadyStatusChanged.Broadcast(bReady);
}

void AKCPlayerState::OnRep_TeamId()
{
	UE_LOG(LogKCLobby, Verbose, TEXT("[KCPlayerState] OnRep_TeamId - Player: '%s', TeamId: %d"),
		*GetPlayerName(), TeamId);
	OnTeamIdChanged.Broadcast(TeamId);
}

void AKCPlayerState::OnRep_SlotIndex()
{
	UE_LOG(LogKCLobby, Verbose, TEXT("[KCPlayerState] OnRep_SlotIndex - Player: '%s', SlotIndex: %d"),
		*GetPlayerName(), SlotIndex);
	OnSlotIndexChanged.Broadcast(SlotIndex);
}

void AKCPlayerState::BeginPlay()
{
	Super::BeginPlay();
	TryRestoreSavedLobbyData();
}

void AKCPlayerState::OnRep_PlayerName()
{
	Super::OnRep_PlayerName();
	TryRestoreSavedLobbyData();
}

void AKCPlayerState::TryRestoreSavedLobbyData()
{
	if (HasAuthority())
	{
		const FString MyNetIdStr = GetUniquePlayerIdString();
		if (MyNetIdStr.IsEmpty())
		{
			return;
		}

		if (UGameInstance* GI = GetGameInstance())
		{
			if (UKCSessionSubsystem* Subsystem = GI->GetSubsystem<UKCSessionSubsystem>())
			{
				int32 SavedTeamId = 0;
				int32 SavedSlotIndex = INDEX_NONE;
				if (Subsystem->GetSavedLobbyPlayerData(MyNetIdStr, SavedTeamId, SavedSlotIndex))
				{
					UE_LOG(LogKCLobby, Log, TEXT("[KCPlayerState] TryRestoreSavedLobbyData: Successfully restored TeamId=%d, SlotIndex=%d for Player '%s' (UniqueId='%s')"),
						SavedTeamId, SavedSlotIndex, *GetPlayerName(), *MyNetIdStr);
					SetTeamId(SavedTeamId);
					SetSlotIndex(SavedSlotIndex);
				}
				else
				{
					UE_LOG(LogKCLobby, Verbose, TEXT("[KCPlayerState] TryRestoreSavedLobbyData: No saved lobby data found for UniqueId '%s'"), *MyNetIdStr);
				}
			}
		}
	}
}


