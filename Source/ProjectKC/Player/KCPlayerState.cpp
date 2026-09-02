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
	DOREPLIFETIME(AKCPlayerState, GamePlayerName);
	DOREPLIFETIME(AKCPlayerState, CustomizationDescriptor);
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
		TargetPS->SetGamePlayerName(this->GamePlayerName);
		TargetPS->CustomizationDescriptor = CustomizationDescriptor;
		TargetPS->CustomizationPayload = CustomizationPayload;
		TargetPS->OnRep_CustomizationDescriptor();
		TargetPS->ForceNetUpdate();
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
		SetGamePlayerName(InactivePS->GamePlayerName);
		CustomizationDescriptor = InactivePS->CustomizationDescriptor;
		CustomizationPayload = InactivePS->CustomizationPayload;
		OnRep_CustomizationDescriptor();
		ForceNetUpdate();
	}
}

bool AKCPlayerState::PublishCustomizationPayload(const TArray<uint8>& InPayload)
{
	if (!HasAuthority())
	{
		return false;
	}

	FRuntimeMeshPaintPatchHistory PaintHistory;
	bool bUseDefaultAppearance = true;
	if (!KCCustomizationNetwork::DeserializePayload(
		InPayload,
		PaintHistory,
		bUseDefaultAppearance))
	{
		return false;
	}

	const uint32 ContentHash = KCCustomizationNetwork::ComputePayloadHash(InPayload);
	if (CustomizationDescriptor.IsPublished() &&
		CustomizationDescriptor.ContentHash == ContentHash &&
		CustomizationDescriptor.TargetSchemaVersion == UKCCustomizationSaveGame::CurrentTargetSchemaVersion &&
		CustomizationDescriptor.bUseDefaultAppearance == bUseDefaultAppearance)
	{
		return true;
	}

	CustomizationPayload = InPayload;
	CustomizationDescriptor.ContentHash = ContentHash;
	CustomizationDescriptor.TargetSchemaVersion = UKCCustomizationSaveGame::CurrentTargetSchemaVersion;
	CustomizationDescriptor.bUseDefaultAppearance = bUseDefaultAppearance;
	++CustomizationDescriptor.Revision;
	if (CustomizationDescriptor.Revision == 0)
	{
		CustomizationDescriptor.Revision = 1;
	}

	OnRep_CustomizationDescriptor();
	ForceNetUpdate();
	return true;
}

bool AKCPlayerState::GetCustomizationPayload(
	const uint32 ExpectedRevision,
	const uint32 ExpectedHash,
	TArray<uint8>& OutPayload) const
{
	OutPayload.Reset();
	if (!HasAuthority() ||
		!CustomizationDescriptor.IsPublished() ||
		CustomizationDescriptor.Revision != ExpectedRevision ||
		CustomizationDescriptor.ContentHash != ExpectedHash ||
		CustomizationPayload.IsEmpty())
	{
		return false;
	}

	OutPayload = CustomizationPayload;
	return true;
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

FString AKCPlayerState::GetGamePlayerName() const
{
	return GamePlayerName.IsEmpty() ? GetPlayerName() : GamePlayerName;
}

void AKCPlayerState::SetGamePlayerName(const FString& InPlayerName)
{
	if (HasAuthority() && GamePlayerName != InPlayerName)
	{
		GamePlayerName = InPlayerName;
		OnRep_GamePlayerName();
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

void AKCPlayerState::OnRep_GamePlayerName()
{
	OnGamePlayerNameChanged.Broadcast(GetGamePlayerName());
}

void AKCPlayerState::OnRep_CustomizationDescriptor()
{
	OnCustomizationDescriptorChanged.Broadcast(CustomizationDescriptor);
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
				FString SavedPlayerName;
				int32 SavedTeamId = 0;
				int32 SavedSlotIndex = INDEX_NONE;
				if (Subsystem->GetSavedLobbyPlayerData(MyNetIdStr, SavedPlayerName, SavedTeamId, SavedSlotIndex))
				{
					UE_LOG(LogKCLobby, Log, TEXT("[KCPlayerState] TryRestoreSavedLobbyData: Successfully restored Name='%s', TeamId=%d, SlotIndex=%d for Player '%s' (UniqueId='%s')"),
						*SavedPlayerName, SavedTeamId, SavedSlotIndex, *GetPlayerName(), *MyNetIdStr);
					SetGamePlayerName(SavedPlayerName);
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


