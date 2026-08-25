/**
 * @file KCLobbyPlayerState.h
 * @brief 로비 내 플레이어의 준비(Ready) 상태 및 팀 정보 동기화를 담당하는 PlayerState 클래스
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "KCLobbyPlayerState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnKCReadyStatusChangedDelegate, bool, bNewReady);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnKCTeamIdChangedDelegate, int32, NewTeamId);

/**
 * @class AKCLobbyPlayerState
 * @brief 로비 레벨에서 각 플레이어의 레디 여부, 팀 ID를 관리하고 변경 시 UI에 브로드캐스트하는 PlayerState
 */
UCLASS()
class PROJECTKC_API AKCLobbyPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AKCLobbyPlayerState();

	//~APlayerState interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~End of APlayerState interface

	/** @brief 현재 설정된 TeamId를 반환합니다. (0부터 시작) */
	UFUNCTION(BlueprintPure, Category = "KC|Lobby")
	int32 GetTeamId() const { return TeamId; }

	/** @brief 서버 권한으로 TeamId를 변경합니다. */
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby")
	void SetTeamId(int32 InTeamId);

	/** @brief 현재 플레이어의 준비(Ready) 완료 여부를 반환합니다. */
	UFUNCTION(BlueprintPure, Category = "KC|Lobby")
	bool IsReady() const { return bReady; }

	/** @brief 서버 권한으로 준비 완료 여부를 설정합니다. */
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby")
	void SetIsReady(bool bNewReady);

	/** @brief 서버 권한으로 준비 완료 상태를 토글(Ready <-> Cancel)합니다. */
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby")
	void ToggleReady();

public:
	/** @brief 준비 상태가 변경되었을 때 브로드캐스트되는 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category = "KC|Lobby|Events")
	FOnKCReadyStatusChangedDelegate OnReadyStatusChanged;

	/** @brief 팀 ID가 변경되었을 때 브로드캐스트되는 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category = "KC|Lobby|Events")
	FOnKCTeamIdChangedDelegate OnTeamIdChanged;

protected:
	/** @brief 플레이어 준비 완료 상태 (네트워크 복제) */
	UPROPERTY(ReplicatedUsing = OnRep_Ready, VisibleAnywhere, BlueprintReadOnly, Category = "KC|Lobby")
	bool bReady = false;

	/** @brief 플레이어 팀 ID (네트워크 복제) */
	UPROPERTY(ReplicatedUsing = OnRep_TeamId, VisibleAnywhere, BlueprintReadOnly, Category = "KC|Lobby")
	int32 TeamId = 0;

	/** @brief bReady 프로퍼티 복제 수신 시 호출 */
	UFUNCTION()
	virtual void OnRep_Ready();

	/** @brief TeamId 프로퍼티 복제 수신 시 호출 */
	UFUNCTION()
	virtual void OnRep_TeamId();
};
