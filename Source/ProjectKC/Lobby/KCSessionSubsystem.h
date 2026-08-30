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

	/** 친구에게 세션 초대 전송 */
	UFUNCTION(BlueprintCallable, Category = "KC|Session")
	bool SendSessionInviteToFriend(const FString& FriendUniqueNetIdStr);

	/** 로비에서 매치 시작 시 플레이어 팀/슬롯 정보 저장 */
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby")
	void SaveLobbyPlayerData(const FString& PlayerName, int32 InTeamId, int32 InSlotIndex);

	/** 인게임에서 플레이어 팀/슬롯 정보 조회 */
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby")
	bool GetSavedLobbyPlayerData(const FString& PlayerName, int32& OutTeamId, int32& OutSlotIndex) const;

	/** 저장된 로비 플레이어 데이터 초기화 */
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby")
	void ClearSavedLobbyData();

	/** 예상 플레이어 총 인원수 설정 및 조회 */
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby")
	void SetExpectedPlayerCount(int32 Count) { ExpectedPlayerCount = Count; }

	UFUNCTION(BlueprintPure, Category = "KC|Lobby")
	int32 GetExpectedPlayerCount() const { return ExpectedPlayerCount; }

public:
	UPROPERTY(BlueprintAssignable, Category = "KC|Session|Events")
	FOnKCCreateSessionCompleteDelegate OnCreateSessionComplete;

	UPROPERTY(BlueprintAssignable, Category = "KC|Session|Events")
	FOnKCJoinSessionCompleteDelegate OnJoinSessionComplete;

	UPROPERTY(BlueprintAssignable, Category = "KC|Session|Events")
	FOnKCDestroySessionCompleteDelegate OnDestroySessionComplete;

	UPROPERTY(BlueprintAssignable, Category = "KC|Session|Events")
	FOnKCSessionInviteAcceptedDelegate OnSessionInviteAccepted;

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

	// Pending Join State (이전 세션 정리 후 자동 참가를 위한 상태값)
	FBlueprintSessionResult PendingSessionToJoin;
	bool bJoiningPendingSessionAfterDestroy = false;

	// Internal Callbacks
	void HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleSessionUserInviteAccepted(const bool bWasSuccessful, const int32 ControllerId, FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& InviteResult);
};
