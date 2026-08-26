/**
 * @file KCPlayerSlotActor.h
 * @brief 로비 월드에 배치되어 특정 위치에 캐릭터를 스폰하고 플레이어 슬롯을 관리하는 액터 클래스
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectKC/Lobby/Struct/KCPlayerInfoStruct.h"
#include "ProjectKC/Lobby/Enum/KCLobbySlotStateType.h"
#include "KCPlayerSlotActor.generated.h"

class AKCLobbyCharacter;

/**
 * @class AKCPlayerSlotActor
 * @brief 로비 레벨의 3D 월드 상 특정 스폰 지점에 배치되어 플레이어 캐릭터 스폰 및 라이프사이클을 관리하는 슬롯 액터
 */
UCLASS()
class PROJECTKC_API AKCPlayerSlotActor : public AActor
{
	GENERATED_BODY()

public:
	AKCPlayerSlotActor();

	//~AActor Interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~End of AActor Interface

	/**
	 * @brief 슬롯에 플레이어를 배정하고 3D 캐릭터(AKCLobbyCharacter)를 스폰 또는 갱신합니다. (서버 전용)
	 * @param InPlayerInfo 배정할 플레이어의 정보
	 */
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby")
	void AssignPlayer(const FKCPlayerInfoStruct& InPlayerInfo);

	/**
	 * @brief 슬롯을 비우고 스폰되어 있던 3D 캐릭터 액터를 파괴합니다. (서버 전용)
	 */
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby")
	void ClearSlot();

	/**
	 * @brief 슬롯을 닫힘(Closed) 또는 열림(Open/Empty) 상태로 설정합니다. (인원수 제한 제어)
	 * @param bClosed true일 경우 비활성화(Closed), false일 경우 활성화(Empty)
	 */
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby")
	void SetSlotClosed(bool bClosed);

	/** @brief 현재 슬롯이 플레이어에 의해 점유 중인지 여부를 반환합니다. */
	UFUNCTION(BlueprintPure, Category = "KC|Lobby")
	bool IsOccupied() const { return bIsOccupied; }

	/** @brief 슬롯이 닫혀있는지(Closed) 여부를 반환합니다. */
	UFUNCTION(BlueprintPure, Category = "KC|Lobby")
	bool IsClosed() const { return SlotState == EKCLobbySlotStateType::Closed; }

	/** @brief 슬롯 정렬 인덱스 번호를 반환합니다. (0~5) */
	UFUNCTION(BlueprintPure, Category = "KC|Lobby")
	int32 GetSlotIndex() const { return SlotIndex; }

	/** @brief 슬롯이 소속된 팀 ID를 반환합니다. (0: Team 0, 1: Team 1) */
	UFUNCTION(BlueprintPure, Category = "KC|Lobby")
	int32 GetSlotTeamId() const { return TeamId; }

	/** @brief 현재 슬롯을 차지하고 있는 PlayerState를 반환합니다. */
	UFUNCTION(BlueprintPure, Category = "KC|Lobby")
	APlayerState* GetOccupyingPlayerState() const { return CurrentPlayerInfo.PlayerState.Get(); }

	/** @brief 현재 슬롯의 상태 열거형을 반환합니다. */
	UFUNCTION(BlueprintPure, Category = "KC|Lobby")
	EKCLobbySlotStateType GetSlotState() const { return SlotState; }

public:
	/**
	 * @brief 슬롯의 상태(Empty, Occupied, Ready, Closed)가 변경되었을 때 블루프린트에서 색상/비주얼을 변경할 수 있는 이벤트
	 * @param NewState 변경된 새로운 슬롯 상태
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "KC|Lobby|Events")
	void OnSlotStateChanged(EKCLobbySlotStateType NewState);

protected:
	/** @brief 슬롯 순서 인덱스 (0~2: Team 0, 3~5: Team 1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KC|Lobby")
	int32 SlotIndex = 0;

	/** @brief 이 슬롯이 소속된 팀 ID (기본적으로 0~2번: 0, 3~5번: 1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KC|Lobby")
	int32 TeamId = 0;

	/** @brief 슬롯 현재 상태 (네트워크 복제) */
	UPROPERTY(ReplicatedUsing = OnRep_SlotState, VisibleAnywhere, BlueprintReadOnly, Category = "KC|Lobby")
	EKCLobbySlotStateType SlotState = EKCLobbySlotStateType::Empty;

	/** @brief 슬롯 점유 여부 (네트워크 복제) */
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "KC|Lobby")
	bool bIsOccupied = false;

	/** @brief 현재 슬롯에 배정된 플레이어 정보 구조체 (네트워크 복제) */
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "KC|Lobby")
	FKCPlayerInfoStruct CurrentPlayerInfo;

	/** @brief 슬롯에 스폰할 로비 캐릭터 블루프린트 클래스 (BP_Lobby_PlayerCharacter) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KC|Lobby")
	TSubclassOf<AKCLobbyCharacter> CharacterClass;

	/** @brief 현재 슬롯 위치에 스폰되어 있는 로비 캐릭터 액터 인스턴스 참조 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KC|Lobby")
	TObjectPtr<AKCLobbyCharacter> SpawnedCharacter;

	/** @brief SlotState 복제 수신 시 호출 */
	UFUNCTION()
	virtual void OnRep_SlotState();
};
