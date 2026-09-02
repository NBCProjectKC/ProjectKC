/**
 * @file KCPlayerState.h
 * @brief 플레이어의 준비(Ready) 상태, 슬롯 인덱스(SlotIndex), 팀 정보(TeamId) 동기화를 담당하는 PlayerState 클래스
 */

#pragma once

#include "CoreMinimal.h"
#include "Customization/KCCustomizationNetworkTypes.h"
#include "GameFramework/PlayerState.h"
#include "KCPlayerState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnKCReadyStatusChangedDelegate, bool, bNewReady);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnKCTeamIdChangedDelegate, int32, NewTeamId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnKCSlotIndexChangedDelegate, int32, NewSlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnKCGamePlayerNameChangedDelegate, const FString&, NewPlayerName);
DECLARE_MULTICAST_DELEGATE_OneParam(
	FOnKCCustomizationDescriptorChangedDelegate,
	const FKCCustomizationDescriptor&);

/**
 * @class AKCPlayerState
 * @brief 로비 및 인게임에서 각 플레이어의 레디 여부, 슬롯 번호(0~5), 팀 ID를 관리하고 변경 시 UI에 브로드캐스트하는 PlayerState
 */
UCLASS()
class PROJECTKC_API AKCPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AKCPlayerState();

	//~APlayerState interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void CopyProperties(APlayerState* PlayerState) override;
	virtual void OverrideWith(APlayerState* PlayerState) override;
	//~End of APlayerState interface

	/** @brief 튕기기 전 마지막 캐릭터 위치/회전 저장 (재접속 리스폰용) */
	/** @brief 플레이어의 스팀 고유 ID 문자열 반환 */
	UFUNCTION(BlueprintPure, Category = "KC|Lobby|Reconnect")
	FString GetUniquePlayerIdString() const
	{
		return GetUniqueId().IsValid() ? GetUniqueId()->ToString() : TEXT("");
	}

	/** @brief 현재 배정된 슬롯 인덱스를 반환합니다. (0~5) */
	UFUNCTION(BlueprintPure, Category = "KC|Lobby")
	int32 GetSlotIndex() const { return SlotIndex; }

	/** @brief 서버 권한으로 슬롯 인덱스를 변경합니다. */
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby")
	void SetSlotIndex(int32 InSlotIndex);

	/** @brief 현재 설정된 TeamId를 반환합니다. (0: Red, 1: Blue) */
	UFUNCTION(BlueprintPure, Category = "KC|Lobby")
	int32 GetTeamId() const { return TeamId; }

	/** @brief 서버 권한으로 TeamId를 변경합니다. */
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby")
	void SetTeamId(int32 InTeamId);

	/** @brief 현재 플레이어의 준비(Ready) 완료 여부를 반환합니다. */
	UFUNCTION(BlueprintPure, Category = "KC|Lobby")
	bool IsReady() const { return bReady; }

	/** @brief 인게임 UI에 표시할 최종 플레이어 이름을 반환합니다. */
	UFUNCTION(BlueprintPure, Category = "KC|Player")
	FString GetGamePlayerName() const;

	/** @brief 서버 권한으로 인게임 UI 표시 이름을 변경합니다. */
	UFUNCTION(BlueprintCallable, Category = "KC|Player")
	void SetGamePlayerName(const FString& InPlayerName);

	/** @brief 서버 권한으로 준비 완료 여부를 설정합니다. */
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby")
	void SetIsReady(bool bNewReady);

	/** @brief 서버 권한으로 준비 완료 상태를 토글(Ready <-> Cancel)합니다. */
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby")
	void ToggleReady();

	/** 검증된 외형 페이로드를 서버 권위 상태로 게시합니다. */
	bool PublishCustomizationPayload(const TArray<uint8>& InPayload);

	/** 요청한 Revision/Hash와 일치하는 서버 전용 외형 페이로드를 반환합니다. */
	bool GetCustomizationPayload(
		uint32 ExpectedRevision,
		uint32 ExpectedHash,
		TArray<uint8>& OutPayload) const;

	const FKCCustomizationDescriptor& GetCustomizationDescriptor() const
	{
		return CustomizationDescriptor;
	}

public:
	/** @brief 준비 상태가 변경되었을 때 브로드캐스트되는 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category = "KC|Lobby|Events")
	FOnKCReadyStatusChangedDelegate OnReadyStatusChanged;

	/** @brief 팀 ID가 변경되었을 때 브로드캐스트되는 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category = "KC|Lobby|Events")
	FOnKCTeamIdChangedDelegate OnTeamIdChanged;

	/** @brief 슬롯 인덱스가 변경되었을 때 브로드캐스트되는 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category = "KC|Lobby|Events")
	FOnKCSlotIndexChangedDelegate OnSlotIndexChanged;

	/** @brief 인게임 UI 표시 이름이 변경되었을 때 브로드캐스트되는 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category = "KC|Player|Events")
	FOnKCGamePlayerNameChangedDelegate OnGamePlayerNameChanged;

	/** C++ 외형 컴포넌트가 Revision 변경을 구독합니다. */
	FOnKCCustomizationDescriptorChangedDelegate OnCustomizationDescriptorChanged;

protected:
	/** @brief 플레이어 준비 완료 상태 (네트워크 복제) */
	UPROPERTY(ReplicatedUsing = OnRep_Ready, VisibleAnywhere, BlueprintReadOnly, Category = "KC|Lobby")
	bool bReady = false;

	/** @brief 플레이어 소속 팀 ID (네트워크 복제, 0: Red, 1: Blue, 기본값: -1) */
	UPROPERTY(ReplicatedUsing = OnRep_TeamId, VisibleAnywhere, BlueprintReadOnly, Category = "KC|Lobby")
	int32 TeamId = INDEX_NONE;

	/** @brief 현재 플레이어가 점유 중인 슬롯 인덱스 (네트워크 복제, 0~5) */
	UPROPERTY(ReplicatedUsing = OnRep_SlotIndex, VisibleAnywhere, BlueprintReadOnly, Category = "KC|Lobby")
	int32 SlotIndex = INDEX_NONE;

	/** @brief 인게임 UI에 표시할 최종 플레이어 이름 */
	UPROPERTY(ReplicatedUsing = OnRep_GamePlayerName, VisibleAnywhere, BlueprintReadOnly, Category = "KC|Player")
	FString GamePlayerName;

	/** 실제 픽셀 데이터 대신 모든 클라이언트에 복제되는 외형 식별자입니다. */
	UPROPERTY(ReplicatedUsing = OnRep_CustomizationDescriptor)
	FKCCustomizationDescriptor CustomizationDescriptor;

	/** @brief bReady 프로퍼티 복제 수신 시 호출 */
	UFUNCTION()
	virtual void OnRep_Ready();

	/** @brief TeamId 프로퍼티 복제 수신 시 호출 */
	UFUNCTION()
	virtual void OnRep_TeamId();

	/** @brief SlotIndex 프로퍼티 복제 수신 시 호출 */
	UFUNCTION()
	virtual void OnRep_SlotIndex();

	/** @brief GamePlayerName 프로퍼티 복제 수신 시 호출 */
	UFUNCTION()
	virtual void OnRep_GamePlayerName();

	UFUNCTION()
	void OnRep_CustomizationDescriptor();

	virtual void BeginPlay() override;
	virtual void OnRep_PlayerName() override;

	/** @brief KCSessionSubsystem에 저장된 이전 팀/슬롯 정보가 있다면 자동 복원 */
	void TryRestoreSavedLobbyData();

	/** 서버에만 보관하며 요청받은 클라이언트에만 청크 RPC로 전송합니다. */
	TArray<uint8> CustomizationPayload;
};
