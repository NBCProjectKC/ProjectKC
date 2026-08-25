/**
 * @file KCLobbyGameMode.h
 * @brief 로비 레벨 전용 GameMode 클래스 정의 (플레이어 접속 관리, 슬롯 동기화, 게임 시작 제어)
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "ProjectKC/Lobby/Struct/KCPlayerInfoStruct.h"
#include "KCLobbyGameMode.generated.h"

class AKCPlayerSlotActor;

/**
 * @class AKCLobbyGameMode
 * @brief 기존 BP_LobbyGameMode 블루프린트를 1:1 매핑하고 C++로 성능 및 안정성을 보강한 로비 GameMode
 */
UCLASS()
class PROJECTKC_API AKCLobbyGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AKCLobbyGameMode();

	//~AGameModeBase interface
	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	//~End of AGameModeBase interface

	/** @brief 세션 생성 시점에 호출하여 최대 필요 인원수를 설정합니다. */
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby")
	void SetRequiredPlayerCount(int32 InCount);

	/** @brief 현재 접속 중인 모든 플레이어가 준비 완료(Ready) 상태인지 확인합니다. */
	UFUNCTION(BlueprintPure, Category = "KC|Lobby")
	bool CheckAllPlayersReady() const;

	/** @brief 모든 클라이언트에 시작 연출 RPC를 보내고 1초 뒤 전투 레벨로 ServerTravel을 실행합니다. */
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby")
	void StartGame();

	/** @brief GameState의 PlayerArray를 기반으로 슬롯 액터들에 플레이어 정보를 동기화하고 캐릭터를 갱신합니다. */
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby")
	void UpdatePlayerSlots();

public:
	/** @brief 현재 접속 중인 플레이어들의 정보 배열 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|Lobby")
	TArray<FKCPlayerInfoStruct> ConnectedPlayers;

protected:
	/** @brief 게임 시작 시 전환될 전투 레벨의 이름 (기본값: Lvl_Main) */
	UPROPERTY(EditDefaultsOnly, Category = "KC|Lobby")
	FString BattleLevelName = TEXT("Lvl_Main");

	/** @brief 로비 레벨에 배치된 슬롯 액터 목록 (최대 6개 지원) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KC|Lobby")
	TArray<TObjectPtr<AKCPlayerSlotActor>> LobbySlots;

private:
	/** @brief 게임 시작을 위한 최소 필요 인원수 */
	int32 RequiredPlayerCount = 6;
};