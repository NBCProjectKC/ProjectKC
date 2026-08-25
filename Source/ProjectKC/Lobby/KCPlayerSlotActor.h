/**
 * @file KCPlayerSlotActor.h
 * @brief 로비 월드에 배치되어 특정 위치에 캐릭터를 스폰하고 플레이어 슬롯을 관리하는 액터 클래스
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectKC/Lobby/Struct/KCPlayerInfoStruct.h"
#include "KCPlayerSlotActor.generated.h"

class AKCLobbyCharacter;

/**
 * @class AKCPlayerSlotActor
 * @brief 기존 BP_PlayerSlot 블루프린트를 1:1 매핑한 로비 슬롯 액터
 */
UCLASS()
class PROJECTKC_API AKCPlayerSlotActor : public AActor
{
	GENERATED_BODY()

public:
	AKCPlayerSlotActor();

	/** @brief 슬롯에 플레이어를 배정하고 캐릭터를 스폰합니다. */
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby")
	void AssignPlayer(const FKCPlayerInfoStruct& InPlayerInfo);

	/** @brief 슬롯을 비우고 스폰되었던 캐릭터를 파괴합니다. */
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby")
	void ClearSlot();

	/** @brief 현재 슬롯이 점유 중인지 여부를 반환합니다. */
	UFUNCTION(BlueprintPure, Category = "KC|Lobby")
	bool IsOccupied() const { return bIsOccupied; }

	/** @brief 슬롯 인덱스 번호를 반환합니다. */
	UFUNCTION(BlueprintPure, Category = "KC|Lobby")
	int32 GetSlotIndex() const { return SlotIndex; }

protected:
	/** @brief 슬롯 순서 인덱스 (0: 중앙, 오름차순 정렬) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KC|Lobby")
	int32 SlotIndex = 0;

	/** @brief 슬롯 점유 여부 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|Lobby")
	bool bIsOccupied = false;

	/** @brief 현재 슬롯에 배정된 플레이어 정보 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|Lobby")
	FKCPlayerInfoStruct CurrentPlayerInfo;

	/** @brief 스폰할 로비 캐릭터 클래스 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KC|Lobby")
	TSubclassOf<AKCLobbyCharacter> CharacterClass;

	/** @brief 현재 슬롯 위치에 스폰된 로비 캐릭터 액터 인스턴스 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KC|Lobby")
	TObjectPtr<AKCLobbyCharacter> SpawnedCharacter;
};
