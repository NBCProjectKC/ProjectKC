/**
 * @file KCSessionSubsystem.cpp
 * @brief UKCSessionSubsystem 구현부
 */

#include "ProjectKC/Lobby/KCSessionSubsystem.h"
#include "ProjectKC/ProjectKC.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "GameFramework/GameStateBase.h"
#include "GameSystem/KCLevelTypeLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "ProjectKC/Lobby/KCLobbyPlayerController.h"
#include "Engine/Engine.h"

UKCSessionSubsystem::UKCSessionSubsystem()
{
}

void UKCSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (Subsystem)
	{
		UE_LOG(LogKCSession, Log, TEXT("[KCSessionSubsystem] OnlineSubsystem initialized (%s)"), *Subsystem->GetSubsystemName().ToString());
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
			UE_LOG(LogKCSession, Log, TEXT("[KCSessionSubsystem] Successfully bound OnlineSession delegates"));
		}
		else
		{
			UE_LOG(LogKCSession, Error, TEXT("[KCSessionSubsystem] Initialize Failed: SessionInterface is null!"));
		}
	}
	else
	{
		UE_LOG(LogKCSession, Error, TEXT("[KCSessionSubsystem] Initialize Failed: OnlineSubsystem::Get() returned null!"));
	}

	if (GEngine)
	{
		NetworkFailureDelegateHandle = GEngine->OnNetworkFailure().AddUObject(this, &UKCSessionSubsystem::HandleNetworkFailure);
	}
}

void UKCSessionSubsystem::Deinitialize()
{
	UE_LOG(LogKCSession, Log, TEXT("[KCSessionSubsystem] Deinitializing KCSessionSubsystem..."));

	if (GEngine && NetworkFailureDelegateHandle.IsValid())
	{
		GEngine->OnNetworkFailure().Remove(NetworkFailureDelegateHandle);
	}

	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
		SessionInterface->ClearOnSessionUserInviteAcceptedDelegate_Handle(SessionUserInviteAcceptedDelegateHandle);
	}

	ClearSavedLobbyData();
	Super::Deinitialize();
}

void UKCSessionSubsystem::CreateSession(int32 NumPublicConnections, bool bIsLANMatch)
{
	UE_LOG(LogKCSession, Log, TEXT("[KCSessionSubsystem] CreateSession requested: NumPublicConnections=%d, bIsLANMatch=%s"),
		NumPublicConnections, bIsLANMatch ? TEXT("TRUE") : TEXT("FALSE"));

	bSessionTerminationNotified = false;

	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogKCSession, Error, TEXT("[KCSessionSubsystem] CreateSession Failed: SessionInterface is invalid"));
		OnCreateSessionComplete.Broadcast(false);
		return;
	}

	FNamedOnlineSession* ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
	if (ExistingSession)
	{
		UE_LOG(LogKCSession, Log, TEXT("[KCSessionSubsystem] Existing session found. Destroying before creating new session..."));
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
		UE_LOG(LogKCSession, Error, TEXT("[KCSessionSubsystem] CreateSession Failed: LocalPlayer is null"));
		OnCreateSessionComplete.Broadcast(false);
		return;
	}

	const bool bSuccess = SessionInterface->CreateSession(LocalPlayer->GetControllerId(), NAME_GameSession, *LastSessionSettings);
	if (!bSuccess)
	{
		UE_LOG(LogKCSession, Error, TEXT("[KCSessionSubsystem] CreateSession returned false immediately"));
		OnCreateSessionComplete.Broadcast(false);
	}
	else
	{
		UE_LOG(LogKCSession, Log, TEXT("[KCSessionSubsystem] CreateSession request dispatched to OnlineSubsystem"));
	}
}

void UKCSessionSubsystem::JoinSession(const FBlueprintSessionResult& SessionResult)
{
	UE_LOG(LogKCSession, Log, TEXT("[KCSessionSubsystem] JoinSession requested"));

	bSessionTerminationNotified = false;

	// 세션 정보 캐싱 (재접속 지원)
	CacheSessionResult(SessionResult);

	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogKCSession, Error, TEXT("[KCSessionSubsystem] JoinSession Failed: SessionInterface is invalid"));
		OnJoinSessionComplete.Broadcast(false, FString());
		return;
	}

	ULocalPlayer* LocalPlayer = GetGameInstance()->GetFirstGamePlayer();
	if (!LocalPlayer)
	{
		UE_LOG(LogKCSession, Error, TEXT("[KCSessionSubsystem] JoinSession Failed: LocalPlayer is null"));
		OnJoinSessionComplete.Broadcast(false, FString());
		return;
	}

	// 기존에 남아있는 세션이 있다면 먼저 파괴 후 대기열을 통해 안전하게 참가
	FNamedOnlineSession* ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
	if (ExistingSession)
	{
		UE_LOG(LogKCSession, Log, TEXT("[KCSessionSubsystem] Existing session found before Join. Destroying old session first..."));
		PendingSessionToJoin = SessionResult;
		bJoiningPendingSessionAfterDestroy = true;
		SessionInterface->DestroySession(NAME_GameSession);
		return;
	}

	const bool bSuccess = SessionInterface->JoinSession(LocalPlayer->GetControllerId(), NAME_GameSession, SessionResult.OnlineResult);
	if (!bSuccess)
	{
		UE_LOG(LogKCSession, Error, TEXT("[KCSessionSubsystem] JoinSession returned false immediately!"));
		OnJoinSessionComplete.Broadcast(false, FString());
	}
	else
	{
		UE_LOG(LogKCSession, Log, TEXT("[KCSessionSubsystem] JoinSession request dispatched to OnlineSubsystem"));
	}
}

void UKCSessionSubsystem::DestroySession()
{
	UE_LOG(LogKCSession, Log, TEXT("[KCSessionSubsystem] DestroySession requested"));

	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogKCSession, Error, TEXT("[KCSessionSubsystem] DestroySession Failed: SessionInterface is invalid"));
		OnDestroySessionComplete.Broadcast(false);
		if (bPendingReturnToMainMenu)
		{
			PerformReturnToMainMenu();
		}
		return;
	}

	const bool bSuccess = SessionInterface->DestroySession(NAME_GameSession);
	if (!bSuccess)
	{
		UE_LOG(LogKCSession, Error, TEXT("[KCSessionSubsystem] DestroySession returned false immediately"));
		OnDestroySessionComplete.Broadcast(false);
		if (bPendingReturnToMainMenu)
		{
			PerformReturnToMainMenu();
		}
	}
}

void UKCSessionSubsystem::EndSession()
{
	UE_LOG(LogKCSession, Log, TEXT("[KCSessionSubsystem] EndSession requested by Host"));
	BroadcastSessionTerminatedToClients(TEXT("Host has ended the session."));
	ReturnToMainMenu();
}

void UKCSessionSubsystem::ReturnToMainMenu()
{
	UE_LOG(LogKCSession, Log, TEXT("[KCSessionSubsystem] ReturnToMainMenu requested"));
	bPendingReturnToMainMenu = true;

	if (SessionInterface.IsValid() && SessionInterface->GetNamedSession(NAME_GameSession))
	{
		DestroySession();
	}
	else
	{
		PerformReturnToMainMenu();
	}
}

void UKCSessionSubsystem::PerformReturnToMainMenu()
{
	bPendingReturnToMainMenu = false;
	bSessionTerminationNotified = false;
	const FName MainMenuLevelName = UKCLevelTypeLibrary::GetLevelName(EKCLevelType::MainMenu);
	UE_LOG(LogKCSession, Log, TEXT("[KCSessionSubsystem] Opening MainMenu level: %s"), *MainMenuLevelName.ToString());

	UWorld* World = GetWorld();
	if (World)
	{
		UGameplayStatics::OpenLevel(World, MainMenuLevelName);
	}
	else
	{
		UGameplayStatics::OpenLevel(this, MainMenuLevelName);
	}
}

void UKCSessionSubsystem::NotifySessionTerminatedByHost(const FString& Reason)
{
	if (bSessionTerminationNotified)
	{
		UE_LOG(LogKCSession, Verbose, TEXT("[KCSessionSubsystem] NotifySessionTerminatedByHost skipped: Already notified (Reason: %s)"), *Reason);
		return;
	}
	bSessionTerminationNotified = true;

	UE_LOG(LogKCSession, Warning, TEXT("[KCSessionSubsystem] NotifySessionTerminatedByHost: %s"), *Reason);
	OnSessionTerminatedByHost.Broadcast(Reason);

	// 방장이 세션을 종료했으므로 선택지(팝업) 없이 즉시 메인 메뉴로 자동 복귀
	ReturnToMainMenu();
}

void UKCSessionSubsystem::BroadcastSessionTerminatedToClients(const FString& Reason)
{
	UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_Client)
	{
		return;
	}

	UE_LOG(LogKCSession, Log, TEXT("[KCSessionSubsystem] Broadcasting SessionTerminated to all remote clients..."));

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC || PC->IsLocalPlayerController())
		{
			continue;
		}

		if (AKCLobbyPlayerController* LobbyPC = Cast<AKCLobbyPlayerController>(PC))
		{
			LobbyPC->Client_NotifySessionTerminated(Reason);
		}
	}
}

void UKCSessionSubsystem::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	UE_LOG(LogKCSession, Warning, TEXT("[KCSessionSubsystem] HandleNetworkFailure: Type=%d, Error='%s'"),
		static_cast<int32>(FailureType), *ErrorString);

	// 클라이언트 환경에서 방장과의 연결이 끊겼을 때 (방장이 강제 종료했거나 네트워크가 단절된 경우)
	if (World && World->GetNetMode() == NM_Client)
	{
		NotifySessionTerminatedByHost(TEXT("Connection to host has been lost."));
	}
}

bool UKCSessionSubsystem::SendSessionInviteToFriend(const FString& FriendUniqueNetIdStr)
{
	UE_LOG(LogKCSession, Log, TEXT("[KCSessionSubsystem] SendSessionInviteToFriend: FriendNetId='%s'"), *FriendUniqueNetIdStr);

	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogKCSession, Error, TEXT("[KCSessionSubsystem] SendSessionInviteToFriend Failed: SessionInterface is invalid"));
		return false;
	}

	// 세션 정원 초과 검사
	FNamedOnlineSession* Session = SessionInterface->GetNamedSession(NAME_GameSession);
	if (Session && Session->NumOpenPublicConnections <= 0)
	{
		UE_LOG(LogKCSession, Warning, TEXT("[KCSessionSubsystem] SendSessionInviteToFriend Rejected: No open connections remaining in session!"));
		return false;
	}

	ULocalPlayer* LocalPlayer = GetGameInstance()->GetFirstGamePlayer();
	if (!LocalPlayer)
	{
		UE_LOG(LogKCSession, Error, TEXT("[KCSessionSubsystem] SendSessionInviteToFriend Failed: LocalPlayer is null"));
		return false;
	}

	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (!Subsystem)
	{
		UE_LOG(LogKCSession, Error, TEXT("[KCSessionSubsystem] SendSessionInviteToFriend Failed: OnlineSubsystem is null"));
		return false;
	}

	IOnlineIdentityPtr IdentityInterface = Subsystem->GetIdentityInterface();
	if (!IdentityInterface.IsValid())
	{
		UE_LOG(LogKCSession, Error, TEXT("[KCSessionSubsystem] SendSessionInviteToFriend Failed: IdentityInterface is invalid"));
		return false;
	}

	FUniqueNetIdPtr FriendNetId = IdentityInterface->CreateUniquePlayerId(FriendUniqueNetIdStr);
	if (!FriendNetId.IsValid())
	{
		UE_LOG(LogKCSession, Warning, TEXT("[KCSessionSubsystem] SendSessionInviteToFriend Failed: Invalid FriendNetId '%s'"), *FriendUniqueNetIdStr);
		return false;
	}

	const bool bSuccess = SessionInterface->SendSessionInviteToFriend(LocalPlayer->GetControllerId(), NAME_GameSession, *FriendNetId);
	UE_LOG(LogKCSession, Log, TEXT("[KCSessionSubsystem] SendSessionInviteToFriend result: %s"), bSuccess ? TEXT("TRUE") : TEXT("FALSE"));
	return bSuccess;
}

void UKCSessionSubsystem::HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogKCSession, Log, TEXT("[KCSessionSubsystem] HandleCreateSessionComplete - Session: %s, Success: %s"),
		*SessionName.ToString(), bWasSuccessful ? TEXT("TRUE") : TEXT("FALSE"));

	if (bWasSuccessful)
	{
		if (UWorld* World = GetWorld())
		{
			const FString LobbyURL = UKCLevelTypeLibrary::GetLevelName(EKCLevelType::LobbyLevel).ToString() + TEXT("?listen");
			UE_LOG(LogKCSession, Log, TEXT("[KCSessionSubsystem] Host ServerTravel to Lobby level: %s"), *LobbyURL);
			World->ServerTravel(LobbyURL);
		}
	}
	else
	{
		UE_LOG(LogKCSession, Error, TEXT("[KCSessionSubsystem] CreateSession failed on OnlineSubsystem"));
	}

	OnCreateSessionComplete.Broadcast(bWasSuccessful);
}

void UKCSessionSubsystem::HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	FString ConnectString;
	bool bSuccess = (Result == EOnJoinSessionCompleteResult::Success);

	UE_LOG(LogKCSession, Log, TEXT("[KCSessionSubsystem] HandleJoinSessionComplete - Session: %s, Result: %d (Success: %s)"),
		*SessionName.ToString(), static_cast<int32>(Result), bSuccess ? TEXT("TRUE") : TEXT("FALSE"));

	if (bSuccess && SessionInterface.IsValid())
	{
		if (SessionInterface->GetResolvedConnectString(SessionName, ConnectString))
		{
			if (APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController())
			{
				UE_LOG(LogKCSession, Log, TEXT("[KCSessionSubsystem] Join Session Success! ClientTravel to: %s"), *ConnectString);
				PC->ClientTravel(ConnectString, ETravelType::TRAVEL_Absolute);
			}
		}
		else
		{
			UE_LOG(LogKCSession, Error, TEXT("[KCSessionSubsystem] GetResolvedConnectString failed for session %s"), *SessionName.ToString());
		}
	}
	else if (!bSuccess)
	{
		UE_LOG(LogKCSession, Error, TEXT("[KCSessionSubsystem] JoinSession failed on OnlineSubsystem (Result: %d)"), static_cast<int32>(Result));
	}

	OnJoinSessionComplete.Broadcast(bSuccess, ConnectString);
}

void UKCSessionSubsystem::HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogKCSession, Log, TEXT("[KCSessionSubsystem] HandleDestroySessionComplete - Session: %s, Success: %s"),
		*SessionName.ToString(), bWasSuccessful ? TEXT("TRUE") : TEXT("FALSE"));

	if (bJoiningPendingSessionAfterDestroy)
	{
		bJoiningPendingSessionAfterDestroy = false;
		UE_LOG(LogKCSession, Log, TEXT("[KCSessionSubsystem] Old session destroyed. Now joining pending session..."));
		JoinSession(PendingSessionToJoin);
		return;
	}

	OnDestroySessionComplete.Broadcast(bWasSuccessful);

	if (bPendingReturnToMainMenu)
	{
		PerformReturnToMainMenu();
		return;
	}
}

void UKCSessionSubsystem::HandleSessionUserInviteAccepted(const bool bWasSuccessful, const int32 ControllerId, FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& InviteResult)
{
	UE_LOG(LogKCSession, Log, TEXT("[KCSessionSubsystem] HandleSessionUserInviteAccepted - Success: %s, ControllerId: %d"),
		bWasSuccessful ? TEXT("TRUE") : TEXT("FALSE"), ControllerId);

	FBlueprintSessionResult Result;
	Result.OnlineResult = InviteResult;
	OnSessionInviteAccepted.Broadcast(bWasSuccessful, Result);

	if (bWasSuccessful)
	{
		UE_LOG(LogKCSession, Log, TEXT("[KCSessionSubsystem] Session invite accepted! Automatically joining session..."));
		JoinSession(Result);
	}
	else
	{
		UE_LOG(LogKCSession, Warning, TEXT("[KCSessionSubsystem] Session invite accepted callback indicated failure"));
	}
}

void UKCSessionSubsystem::SaveLobbyPlayerData(const FString& UniqueNetId, const FString& PlayerName, int32 InTeamId, int32 InSlotIndex)
{
	if (UniqueNetId.IsEmpty())
	{
		UE_LOG(LogKCSession, Warning, TEXT("[KCSessionSubsystem] SaveLobbyPlayerData Warning: UniqueNetId is empty for '%s'"), *PlayerName);
		return;
	}

	const FKCLobbySavedPlayerDataStruct Data(UniqueNetId, PlayerName, InTeamId, InSlotIndex);
	SavedLobbyPlayers.Add(UniqueNetId, Data);

	UE_LOG(LogKCSession, Log, TEXT("[KCSessionSubsystem] Saved Lobby Player Data: UniqueId='%s', Name='%s', TeamId=%d, SlotIndex=%d"),
		*UniqueNetId, *PlayerName, InTeamId, InSlotIndex);
}

bool UKCSessionSubsystem::GetSavedLobbyPlayerData(
	const FString& UniqueNetId,
	FString& OutPlayerName,
	int32& OutTeamId,
	int32& OutSlotIndex) const
{
	if (UniqueNetId.IsEmpty())
	{
		return false;
	}

	if (const FKCLobbySavedPlayerDataStruct* FoundData = SavedLobbyPlayers.Find(UniqueNetId))
	{
		OutPlayerName = FoundData->PlayerName;
		OutTeamId = FoundData->TeamId;
		OutSlotIndex = FoundData->SlotIndex;
		UE_LOG(LogKCSession, Log, TEXT("[KCSessionSubsystem] GetSavedLobbyPlayerData: Found data for UniqueId='%s' (Name='%s', TeamId=%d, SlotIndex=%d)"),
			*UniqueNetId, *OutPlayerName, OutTeamId, OutSlotIndex);
		return true;
	}

	return false;
}

void UKCSessionSubsystem::ClearSavedLobbyData()
{
	SavedLobbyPlayers.Empty();
	ExpectedPlayerCount = 0;
	UE_LOG(LogKCSession, Log, TEXT("[KCSessionSubsystem] Cleared Saved Lobby Player Data"));
}

bool UKCSessionSubsystem::IsLobbyFull() const
{
	const UWorld* World = GetWorld();
	const AGameStateBase* GS = World ? World->GetGameState() : nullptr;
	if (!GS)
	{
		return false;
	}

	const int32 MaxPlayers = (ExpectedPlayerCount > 0) ? ExpectedPlayerCount : 6;
	return GS->PlayerArray.Num() >= MaxPlayers;
}

void UKCSessionSubsystem::CacheSessionResult(const FBlueprintSessionResult& SessionResult)
{
	CachedLastSessionResult = SessionResult;
	bHasCachedSession = SessionResult.OnlineResult.IsValid();
	UE_LOG(LogKCSession, Log, TEXT("[KCSessionSubsystem] Cached Last Session Result: Valid=%s, Ping=%d ms"),
		bHasCachedSession ? TEXT("TRUE") : TEXT("FALSE"), SessionResult.OnlineResult.PingInMs);
}

void UKCSessionSubsystem::RejoinLastSession()
{
	if (!bHasCachedSession || !CachedLastSessionResult.OnlineResult.IsValid())
	{
		UE_LOG(LogKCSession, Warning, TEXT("[KCSessionSubsystem] RejoinLastSession Failed: No valid cached session found!"));
		OnJoinSessionComplete.Broadcast(false, FString());
		return;
	}

	UE_LOG(LogKCSession, Log, TEXT("[KCSessionSubsystem] Rejoining cached session directly..."));
	JoinSession(CachedLastSessionResult);
}

void UKCSessionSubsystem::ClearCachedSession()
{
	CachedLastSessionResult = FBlueprintSessionResult();
	bHasCachedSession = false;
	UE_LOG(LogKCSession, Log, TEXT("[KCSessionSubsystem] Cleared Cached Session"));
}

