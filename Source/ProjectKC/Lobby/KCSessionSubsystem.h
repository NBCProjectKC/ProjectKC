/**
 * @file KCSessionSubsystem.h
 * @brief 스팀/온라인 세션 관리 및 로비-인게임 데이터 보존 서브시스템 정의
 */

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "FindSessionsCallbackProxy.h"
#include "ProjectKC/Lobby/Struct/KCLobbySavedPlayerDataStruct.h"
#include "KCSessionSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnKCCreateSessionCompleteDelegate, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnKCJoinSessionCompleteDelegate, bool, bWasSuccessful, const FString&, ConnectString);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnKCDestroySessionCompleteDelegate, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnKCSessionInviteAcceptedDelegate, bool, bWasSuccessful, const FBlueprintSessionResult&, SessionToJoin);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnKCSessionTerminatedByHostDelegate, const FString&, Reason);

/**
 * @brief 기존 BP_GameInstance의 세션 관리(생성/참가/파괴/초대) 및 로비<->인게임 영구 데이터 보존을 전담하는 서브시스템
 */
UCLASS()
class PROJECTKC_API UKCSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UKCSessionSubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** 세션 생성 */
	UFUNCTION(BlueprintCallable, Category = "KC|Session")
	void CreateSession(int32 NumPublicConnections = 6, bool bIsLANMatch = false);

	/** 세션 참가 (FBlueprintSessionResult 이용) */
	UFUNCTION(BlueprintCallable, Category = "KC|Session")
	void JoinSession(const FBlueprintSessionResult& SessionResult);

	/** 현재 세션 파괴 */
	UFUNCTION(BlueprintCallable, Category = "KC|Session")
	void DestroySession();

	/**
	 * @brief 방장 권한으로 현재 세션을 종료하고 모든 플레이어를 메인 메뉴로 복귀시킵니다.
	 * 접속 중인 모든 클라이언트에게 종료 알림을 보낸 뒤 세션을 파괴하고 메인 메뉴로 이동합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "KC|Session")
	void EndSession();

	/**
	 * @brief 세션을 정리하고 메인 메뉴 레벨(L_MainMeun)로 이동합니다.
	 * 클라이언트가 자체적으로 방을 나가거나 방장이 세션을 종료했을 때 호출합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "KC|Session")
	void ReturnToMainMenu();

	/**
	 * @brief 외부(RPC 또는 NetworkFailure)에서 호출하여 '방장이 세션을 종료했음'을 알리는 함수
	 */
	UFUNCTION(BlueprintCallable, Category = "KC|Session")
	void NotifySessionTerminatedByHost(const FString& Reason = TEXT("Host has terminated the session."));

	/** 친구에게 세션 초대 전송 */
	UFUNCTION(BlueprintCallable, Category = "KC|Session")
	bool SendSessionInviteToFriend(const FString& FriendUniqueNetIdStr);

	/** 로비에서 매치 시작 시 또는 인게임 이탈 시 플레이어 팀/슬롯 정보 저장 */
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby")
	void SaveLobbyPlayerData(const FString& UniqueNetId, const FString& PlayerName, int32 InTeamId, int32 InSlotIndex);

	/** 인게임에서 플레이어 이름/팀/슬롯 정보 조회 */
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby")
	bool GetSavedLobbyPlayerData(const FString& UniqueNetId, FString& OutPlayerName, int32& OutTeamId, int32& OutSlotIndex) const;

	/** 저장된 로비 플레이어 데이터 초기화 */
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby")
	void ClearSavedLobbyData();

	/** 예상 플레이어 총 인원수 설정 및 조회 */
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby")
	void SetExpectedPlayerCount(int32 Count) { ExpectedPlayerCount = Count; }

	UFUNCTION(BlueprintPure, Category = "KC|Lobby")
	int32 GetExpectedPlayerCount() const { return ExpectedPlayerCount; }

	/** 현재 로비가 만석(정원 초과)인지 확인 */
	UFUNCTION(BlueprintPure, Category = "KC|Lobby")
	bool IsLobbyFull() const;

	/** 세션 결과 캐싱 (재접속 지원용) */
	UFUNCTION(BlueprintCallable, Category = "KC|Session|Reconnect")
	void CacheSessionResult(const FBlueprintSessionResult& SessionResult);

	/** 캐싱된 유효한 세션이 있는지 여부 */
	UFUNCTION(BlueprintPure, Category = "KC|Session|Reconnect")
	bool HasCachedSession() const { return bHasCachedSession; }

	/** 캐싱된 세션 검색 결과 반환 */
	UFUNCTION(BlueprintPure, Category = "KC|Session|Reconnect")
	FBlueprintSessionResult GetCachedSessionResult() const { return CachedLastSessionResult; }

	/** 캐싱된 직전 세션으로 다이렉트 재접속 시도 */
	UFUNCTION(BlueprintCallable, Category = "KC|Session|Reconnect")
	void RejoinLastSession();

	/** 캐싱된 세션 정보 삭제 */
	UFUNCTION(BlueprintCallable, Category = "KC|Session|Reconnect")
	void ClearCachedSession();

public:
	UPROPERTY(BlueprintAssignable, Category = "KC|Session|Events")
	FOnKCCreateSessionCompleteDelegate OnCreateSessionComplete;

	UPROPERTY(BlueprintAssignable, Category = "KC|Session|Events")
	FOnKCJoinSessionCompleteDelegate OnJoinSessionComplete;

	UPROPERTY(BlueprintAssignable, Category = "KC|Session|Events")
	FOnKCDestroySessionCompleteDelegate OnDestroySessionComplete;

	UPROPERTY(BlueprintAssignable, Category = "KC|Session|Events")
	FOnKCSessionInviteAcceptedDelegate OnSessionInviteAccepted;

	/**
	 * @brief 방장이 세션을 종료했거나 연결이 끊어졌을 때 클라이언트 화면에 팝업을 띄우기 위한 델리게이트
	 * UI 위젯에서 이 델리게이트를 바인딩하여 팝업을 출력합니다.
	 */
	UPROPERTY(BlueprintAssignable, Category = "KC|Session|Events")
	FOnKCSessionTerminatedByHostDelegate OnSessionTerminatedByHost;

private:
	UPROPERTY()
	TMap<FString, FKCLobbySavedPlayerDataStruct> SavedLobbyPlayers;

	UPROPERTY()
	int32 ExpectedPlayerCount = 0;

	IOnlineSessionPtr SessionInterface;
	TSharedPtr<FOnlineSessionSettings> LastSessionSettings;

	// OnlineSubsystem Delegate Handles
	FDelegateHandle CreateSessionCompleteDelegateHandle;
	FDelegateHandle JoinSessionCompleteDelegateHandle;
	FDelegateHandle DestroySessionCompleteDelegateHandle;
	FDelegateHandle SessionUserInviteAcceptedDelegateHandle;
	FDelegateHandle NetworkFailureDelegateHandle;

	// Pending Termination Actions
	bool bPendingReturnToMainMenu = false;
	bool bSessionTerminationNotified = false;

	void PerformReturnToMainMenu();
	void BroadcastSessionTerminatedToClients(const FString& Reason);
	void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);

	// Pending Join State (이전 세션 정리 후 자동 참가를 위한 상태값)
	FBlueprintSessionResult PendingSessionToJoin;
	bool bJoiningPendingSessionAfterDestroy = false;

	// Cached Session for Direct Reconnect
	UPROPERTY()
	FBlueprintSessionResult CachedLastSessionResult;

	UPROPERTY()
	bool bHasCachedSession = false;

	// Internal Callbacks
	void HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleSessionUserInviteAccepted(const bool bWasSuccessful, const int32 ControllerId, FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& InviteResult);
};
