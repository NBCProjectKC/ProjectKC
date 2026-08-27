#include "ProjectKC/Lobby/KCSessionSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Kismet/GameplayStatics.h"

UKCSessionSubsystem::UKCSessionSubsystem()
{
}

void UKCSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (Subsystem)
	{
		SessionInterface = Subsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			CreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
				FOnCreateSessionCompleteDelegate::CreateUObject(this, &UKCSessionSubsystem::HandleCreateSessionComplete)
			);
			JoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
				FOnJoinSessionCompleteDelegate::CreateUObject(this, &UKCSessionSubsystem::HandleJoinSessionComplete)
			);
			DestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
				FOnDestroySessionCompleteDelegate::CreateUObject(this, &UKCSessionSubsystem::HandleDestroySessionComplete)
			);
			SessionUserInviteAcceptedDelegateHandle = SessionInterface->AddOnSessionUserInviteAcceptedDelegate_Handle(
				FOnSessionUserInviteAcceptedDelegate::CreateUObject(this, &UKCSessionSubsystem::HandleSessionUserInviteAccepted)
			);
		}
	}
}

void UKCSessionSubsystem::Deinitialize()
{
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
		SessionInterface->ClearOnSessionUserInviteAcceptedDelegate_Handle(SessionUserInviteAcceptedDelegateHandle);
	}

	Super::Deinitialize();
}

void UKCSessionSubsystem::CreateSession(int32 NumPublicConnections, bool bIsLANMatch)
{
	if (!SessionInterface.IsValid())
	{
		OnCreateSessionComplete.Broadcast(false);
		return;
	}

	FNamedOnlineSession* ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
	if (ExistingSession)
	{
		SessionInterface->DestroySession(NAME_GameSession);
	}

	LastSessionSettings = MakeShareable(new FOnlineSessionSettings());
	LastSessionSettings->bIsLANMatch = bIsLANMatch;
	LastSessionSettings->NumPublicConnections = NumPublicConnections;
	LastSessionSettings->bAllowJoinInProgress = true;
	LastSessionSettings->bAllowInvites = true;
	LastSessionSettings->bShouldAdvertise = true;
	LastSessionSettings->bUsesPresence = true;
	LastSessionSettings->bAllowJoinViaPresence = true;
	LastSessionSettings->bAllowJoinViaPresenceFriendsOnly = false;
	LastSessionSettings->bUseLobbiesIfAvailable = true;

	ULocalPlayer* LocalPlayer = GetGameInstance()->GetFirstGamePlayer();
	if (!LocalPlayer)
	{
		OnCreateSessionComplete.Broadcast(false);
		return;
	}

	const bool bSuccess = SessionInterface->CreateSession(LocalPlayer->GetControllerId(), NAME_GameSession, *LastSessionSettings);
	if (!bSuccess)
	{
		OnCreateSessionComplete.Broadcast(false);
	}
}

void UKCSessionSubsystem::JoinSession(const FBlueprintSessionResult& SessionResult)
{
	if (!SessionInterface.IsValid())
	{
		OnJoinSessionComplete.Broadcast(false, FString());
		return;
	}

	ULocalPlayer* LocalPlayer = GetGameInstance()->GetFirstGamePlayer();
	if (!LocalPlayer)
	{
		OnJoinSessionComplete.Broadcast(false, FString());
		return;
	}

	const bool bSuccess = SessionInterface->JoinSession(LocalPlayer->GetControllerId(), NAME_GameSession, SessionResult.OnlineResult);
	if (!bSuccess)
	{
		OnJoinSessionComplete.Broadcast(false, FString());
	}
}

void UKCSessionSubsystem::DestroySession()
{
	if (!SessionInterface.IsValid())
	{
		OnDestroySessionComplete.Broadcast(false);
		return;
	}

	const bool bSuccess = SessionInterface->DestroySession(NAME_GameSession);
	if (!bSuccess)
	{
		OnDestroySessionComplete.Broadcast(false);
	}
}

bool UKCSessionSubsystem::SendSessionInviteToFriend(const FString& FriendUniqueNetIdStr)
{
	if (!SessionInterface.IsValid())
	{
		return false;
	}

	ULocalPlayer* LocalPlayer = GetGameInstance()->GetFirstGamePlayer();
	if (!LocalPlayer)
	{
		return false;
	}

	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (!Subsystem)
	{
		return false;
	}

	IOnlineIdentityPtr IdentityInterface = Subsystem->GetIdentityInterface();
	if (!IdentityInterface.IsValid())
	{
		return false;
	}

	FUniqueNetIdPtr FriendNetId = IdentityInterface->CreateUniquePlayerId(FriendUniqueNetIdStr);
	if (!FriendNetId.IsValid())
	{
		return false;
	}

	return SessionInterface->SendSessionInviteToFriend(LocalPlayer->GetControllerId(), NAME_GameSession, *FriendNetId);
}

void UKCSessionSubsystem::HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		if (UWorld* World = GetWorld())
		{
			World->ServerTravel(TEXT("/Game/KC/SteamLobbySystem/Levels/L_LobbyLevel?listen"));
		}
	}

	OnCreateSessionComplete.Broadcast(bWasSuccessful);
}

void UKCSessionSubsystem::HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	FString ConnectString;
	bool bSuccess = (Result == EOnJoinSessionCompleteResult::Success);

	if (bSuccess && SessionInterface.IsValid())
	{
		SessionInterface->GetResolvedConnectString(SessionName, ConnectString);
	}

	OnJoinSessionComplete.Broadcast(bSuccess, ConnectString);
}

void UKCSessionSubsystem::HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	OnDestroySessionComplete.Broadcast(bWasSuccessful);
}

void UKCSessionSubsystem::HandleSessionUserInviteAccepted(const bool bWasSuccessful, const int32 ControllerId, FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& InviteResult)
{
	FBlueprintSessionResult Result;
	Result.OnlineResult = InviteResult;
	OnSessionInviteAccepted.Broadcast(bWasSuccessful, Result);
}

void UKCSessionSubsystem::SaveLobbyPlayerData(const FString& PlayerName, int32 InTeamId, int32 InSlotIndex)
{
	FKCLobbySavedPlayerDataStruct Data;
	Data.PlayerName = PlayerName;
	Data.TeamId = InTeamId;
	Data.SlotIndex = InSlotIndex;

	SavedLobbyPlayers.Add(PlayerName, Data);

	UE_LOG(LogTemp, Log, TEXT("[KCSessionSubsystem] Saved Lobby Player Data: Name='%s', TeamId=%d, SlotIndex=%d"),
		*PlayerName, InTeamId, InSlotIndex);
}

bool UKCSessionSubsystem::GetSavedLobbyPlayerData(const FString& PlayerName, int32& OutTeamId, int32& OutSlotIndex) const
{
	if (const FKCLobbySavedPlayerDataStruct* FoundData = SavedLobbyPlayers.Find(PlayerName))
	{
		OutTeamId = FoundData->TeamId;
		OutSlotIndex = FoundData->SlotIndex;
		return true;
	}
	return false;
}

void UKCSessionSubsystem::ClearSavedLobbyData()
{
	SavedLobbyPlayers.Empty();
	ExpectedPlayerCount = 0;
	UE_LOG(LogTemp, Log, TEXT("[KCSessionSubsystem] Cleared Saved Lobby Player Data"));
}
