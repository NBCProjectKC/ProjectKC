/**
 * @file KCLobbyGameMode.h
 * @brief 로비 레벨 전용 GameMode 클래스 정의 (슬롯 고정형 배정, 개별 슬롯 자리 이동, 유동 인원수 & 2팀 슬롯 동기화)
 */

#pragma once

#include "CoreMinimal.h"
#include "KCLevelTypeLibrary.h"
#include "GameFramework/GameMode.h"
#include "ProjectKC/Lobby/Struct/KCPlayerInfoStruct.h"
#include "KCLobbyGameMode.generated.h"

class AKCPlayerSlotActor;

/**
 * @class AKCLobbyGameMode
 * @brief 로비 레벨의 슬롯 고정형(Slot-Based) 플레이어 배정, 개별 자리 이동(Move to Slot) 및 게임 시작을 총괄하는 GameMode
 */
UCLASS()
class PROJECTKC_API AKCLobbyGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AKCLobbyGameMode();

	//~AGameModeBase Interface
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	//~End of AGameModeBase Interface

	/** @brief 세션 생성 시점 또는 런타임에 게임 필요 총 인원수(2, 4, 6인)를 설정합니다. */
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby")
	void SetRequiredPlayerCount(int32 InCount);

	/** @brief 현재 설정된 필요 인원수를 반환합니다. */
	UFUNCTION(BlueprintPure, Category = "KC|Lobby")
	int32 GetRequiredPlayerCount() const { return RequiredPlayerCount; }

	/**
	 * @brief 특정 플레이어가 원하는 대상 슬롯(TargetSlotIndex)으로 1:1 자리 이동을 요청합니다.
	 * @param Controller 이동을 요청한 플레이어의 Controller
	 * @param TargetSlotIndex 목표 슬롯 번호 (0~5)
	 * @return 이동 성공 여부 (비어있고 열려있는 슬롯일 때 true)
	 */
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby")
	bool MovePlayerToSlot(AController* Controller, int32 TargetSlotIndex);

	/** @brief 플레이어가 레디 상태를 토글했을 때 해당 슬롯 정보만 안전하게 갱신합니다. */
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby")
	void HandlePlayerReadyToggled(AController* Controller);

	/** @brief 필요 인원수가 모두 접속하고, 전원 준비 완료(Ready) 상태인지 확인합니다. */
	UFUNCTION(BlueprintPure, Category = "KC|Lobby")
	bool CheckAllPlayersReady() const;

	/** @brief 모든 클라이언트에 시작 연출 RPC(Client_OnMatchBegin)를 보내고 1초 뒤 전투 레벨로 ServerTravel 실행 */
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby")
	void StartGame();

	/** @brief 전체 슬롯 및 방장 시작 버튼 상태를 갱신합니다. */
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby")
	void UpdateLobbyReadyState();

	/** @brief 디버깅용 콘솔 명령어: ~ 콘솔창에서 "Debug_SetLobbyPlayers 2" 또는 "Debug_SetLobbyPlayers 4" */
	UFUNCTION(Exec)
	void Debug_SetLobbyPlayers(int32 InCount);

public:
	/** @brief 현재 접속 중인 플레이어들의 정보 구조체 캐싱 배열 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|Lobby")
	TArray<FKCPlayerInfoStruct> ConnectedPlayers;

protected:
	/** @brief 게임 시작 시 ServerTravel 대상이 될 전투 레벨 맵 이름 (기본값: Lvl_Main 추후 L_GasRange로 변경 예정) */
	// LevelTypeLibrary 사용
	UPROPERTY(EditDefaultsOnly, Category = "KC|Lobby")
	FString TravelURL = UKCLevelTypeLibrary::GetLevelName(EKCLevelType::GasRange).ToString() + TEXT("?listen");

	/** @brief 로비 레벨에 배치된 슬롯 액터 목록 (0~2: Team 0, 3~5: Team 1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KC|Lobby")
	TArray<TObjectPtr<AKCPlayerSlotActor>> LobbySlots;

	/** @brief 게임 시작을 위한 최소 필요 인원수 (기본값 6인: 3 vs 3, 에디터에서 2, 4, 6으로 수정 가능) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KC|Lobby", meta = (ClampMin = "2", ClampMax = "6"))
	int32 RequiredPlayerCount = 6;

private:
	/** @brief 슬롯 액터들이 수집되었는지 확인하고 없으면 즉시 수집/정렬 */
	void EnsureSlotsCollected();

	/** @brief 슬롯 인덱스(0~5)로 슬롯 액터를 검색하여 반환 */
	AKCPlayerSlotActor* FindSlotByIndex(int32 SlotIndex) const;

	/** @brief 플레이어 스테이트를 특정 슬롯 액터에 배정하고 슬롯/팀 동기화 */
	void AssignPlayerToSlot(AKCPlayerSlotActor* TargetSlot, class AKCLobbyPlayerState* PS);

	/** @brief 신규 접속자를 비어있는 첫 번째 유효 슬롯에 배정 */
	void AssignPlayerToAvailableSlot(APlayerController* NewPlayer);

	/** @brief 퇴장한 플레이어의 슬롯을 찾아 비움 */
	void RemovePlayerFromSlot(AController* Exiting);

	/** @brief 인원수 변경 시 유효/닫힘 슬롯 설정 적용 */
	void ApplySlotOpenCloseRules();
};