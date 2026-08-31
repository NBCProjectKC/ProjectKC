/**
 * @file KCLobbyPlayerState.cpp
 * @brief AKCLobbyPlayerState 구현부
 */

#include "ProjectKC/Lobby/KCLobbyPlayerState.h"
#include "ProjectKC/Lobby/KCSessionSubsystem.h"
#include "ProjectKC/ProjectKC.h"
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
	DOREPLIFETIME(AKCLobbyPlayerState, SlotIndex);
}

void AKCLobbyPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	if (AKCLobbyPlayerState* TargetPS = Cast<AKCLobbyPlayerState>(PlayerState))
	{
		UE_LOG(LogKCLobby, Log, TEXT("[KCLobbyPlayerState] CopyProperties: Copying TeamId=%d, SlotIndex=%d, HasTransform=%s to new PlayerState '%s'"),
			this->TeamId, this->SlotIndex, this->bHasSavedTransform ? TEXT("TRUE") : TEXT("FALSE"), *TargetPS->GetName());
		TargetPS->SetTeamId(this->TeamId);
		TargetPS->SetSlotIndex(this->SlotIndex);
		TargetPS->SavedTransform = this->SavedTransform;
		TargetPS->bHasSavedTransform = this->bHasSavedTransform;
	}
}

void AKCLobbyPlayerState::OverrideWith(APlayerState* PlayerState)
{
	Super::OverrideWith(PlayerState);

	if (AKCLobbyPlayerState* InactivePS = Cast<AKCLobbyPlayerState>(PlayerState))
	{
		UE_LOG(LogKCLobby, Log, TEXT("[KCLobbyPlayerState] OverrideWith: Restoring Inactive PlayerState '%s' (TeamId=%d, SlotIndex=%d, HasTransform=%s)"),
			*InactivePS->GetPlayerName(), InactivePS->TeamId, InactivePS->SlotIndex,
			InactivePS->bHasSavedTransform ? TEXT("TRUE") : TEXT("FALSE"));

		SetTeamId(InactivePS->TeamId);
		SetSlotIndex(InactivePS->SlotIndex);
		this->SavedTransform = InactivePS->SavedTransform;
		this->bHasSavedTransform = InactivePS->bHasSavedTransform;
	}
}

void AKCLobbyPlayerState::SavePlayerTransform(const FTransform& InTransform)
{
	SavedTransform = InTransform;
	bHasSavedTransform = true;
	UE_LOG(LogKCLobby, Log, TEXT("[KCLobbyPlayerState] '%s' SavePlayerTransform: %s"),
		*GetPlayerName(), *InTransform.ToString());
}

bool AKCLobbyPlayerState::GetSavedTransform(FTransform& OutTransform) const
{
	if (bHasSavedTransform)
	{
		OutTransform = SavedTransform;
		return true;
	}
	return false;
}

void AKCLobbyPlayerState::ClearSavedTransform()
{
	SavedTransform = FTransform::Identity;
	bHasSavedTransform = false;
	UE_LOG(LogKCLobby, Verbose, TEXT("[KCLobbyPlayerState] '%s' ClearSavedTransform"), *GetPlayerName());
}

void AKCLobbyPlayerState::SetSlotIndex(int32 InSlotIndex)
{
	if (HasAuthority() && SlotIndex != InSlotIndex)
	{
		UE_LOG(LogKCLobby, Verbose, TEXT("[KCLobbyPlayerState] '%s' SetSlotIndex: %d -> %d"), *GetPlayerName(), SlotIndex, InSlotIndex);
		SlotIndex = InSlotIndex;
		OnRep_SlotIndex();
		ForceNetUpdate();
	}
}

void AKCLobbyPlayerState::SetTeamId(int32 InTeamId)
{
	if (HasAuthority() && TeamId != InTeamId)
	{
		UE_LOG(LogKCLobby, Verbose, TEXT("[KCLobbyPlayerState] '%s' SetTeamId: %d -> %d"), *GetPlayerName(), TeamId, InTeamId);
		TeamId = InTeamId;
		OnRep_TeamId();
		ForceNetUpdate();
	}
}

void AKCLobbyPlayerState::SetIsReady(bool bNewReady)
{
	if (HasAuthority() && bReady != bNewReady)
	{
		UE_LOG(LogKCLobby, Log, TEXT("[KCLobbyPlayerState] '%s' SetIsReady: %s -> %s"),
			*GetPlayerName(), bReady ? TEXT("TRUE") : TEXT("FALSE"), bNewReady ? TEXT("TRUE") : TEXT("FALSE"));
		bReady = bNewReady;
		OnRep_Ready();
		ForceNetUpdate();
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
	UE_LOG(LogKCLobby, Verbose, TEXT("[KCLobbyPlayerState] OnRep_Ready - Player: '%s', Ready: %s"),
		*GetPlayerName(), bReady ? TEXT("TRUE") : TEXT("FALSE"));
	OnReadyStatusChanged.Broadcast(bReady);
}

void AKCLobbyPlayerState::OnRep_TeamId()
{
	UE_LOG(LogKCLobby, Verbose, TEXT("[KCLobbyPlayerState] OnRep_TeamId - Player: '%s', TeamId: %d"),
		*GetPlayerName(), TeamId);
	OnTeamIdChanged.Broadcast(TeamId);
}

void AKCLobbyPlayerState::OnRep_SlotIndex()
{
	UE_LOG(LogKCLobby, Verbose, TEXT("[KCLobbyPlayerState] OnRep_SlotIndex - Player: '%s', SlotIndex: %d"),
		*GetPlayerName(), SlotIndex);
	OnSlotIndexChanged.Broadcast(SlotIndex);
}

void AKCLobbyPlayerState::BeginPlay()
{
	Super::BeginPlay();
	TryRestoreSavedLobbyData();
}

void AKCLobbyPlayerState::OnRep_PlayerName()
{
	Super::OnRep_PlayerName();
	TryRestoreSavedLobbyData();
}

void AKCLobbyPlayerState::TryRestoreSavedLobbyData()
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
					UE_LOG(LogKCLobby, Log, TEXT("[KCLobbyPlayerState] TryRestoreSavedLobbyData: Successfully restored TeamId=%d, SlotIndex=%d for Player '%s' (UniqueId='%s')"),
						SavedTeamId, SavedSlotIndex, *GetPlayerName(), *MyNetIdStr);
					SetTeamId(SavedTeamId);
					SetSlotIndex(SavedSlotIndex);
				}
				else
				{
					UE_LOG(LogKCLobby, Verbose, TEXT("[KCLobbyPlayerState] TryRestoreSavedLobbyData: No saved lobby data found for UniqueId '%s'"), *MyNetIdStr);
				}
			}
		}
	}
}


