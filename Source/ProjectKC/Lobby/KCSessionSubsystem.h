#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "FindSessionsCallbackProxy.h"
#include "KCSessionSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnKCCreateSessionCompleteDelegate, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnKCJoinSessionCompleteDelegate, bool, bWasSuccessful, const FString&, ConnectString);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnKCDestroySessionCompleteDelegate, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnKCSessionInviteAcceptedDelegate, bool, bWasSuccessful, const FBlueprintSessionResult&, SessionToJoin);

/**
 * @brief 기존 BP_GameInstance의 세션 관리(생성/참가/파괴/초대) 로직을 전담하는 서브시스템
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
	void CreateSession(int32 NumPublicConnections = 4, bool bIsLANMatch = false);

	/** 세션 참가 (FBlueprintSessionResult 이용) */
	UFUNCTION(BlueprintCallable, Category = "KC|Session")
	void JoinSession(const FBlueprintSessionResult& SessionResult);

	/** 현재 세션 파괴 */
	UFUNCTION(BlueprintCallable, Category = "KC|Session")
	void DestroySession();

	/** 친구에게 세션 초대 전송 */
	UFUNCTION(BlueprintCallable, Category = "KC|Session")
	bool SendSessionInviteToFriend(const FString& FriendUniqueNetIdStr);

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
	IOnlineSessionPtr SessionInterface;
	TSharedPtr<FOnlineSessionSettings> LastSessionSettings;

	// OnlineSubsystem Delegate Handles
	FDelegateHandle CreateSessionCompleteDelegateHandle;
	FDelegateHandle JoinSessionCompleteDelegateHandle;
	FDelegateHandle DestroySessionCompleteDelegateHandle;
	FDelegateHandle SessionUserInviteAcceptedDelegateHandle;

	// Internal Callbacks
	void HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleSessionUserInviteAccepted(const bool bWasSuccessful, const int32 ControllerId, FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& InviteResult);
};
