/**
 * @file KCLobbySavedPlayerDataStruct.h
 * @brief 로비에서 인게임으로 전환 시 플레이어의 팀 및 슬롯 정보를 보존하기 위한 데이터 구조체 정의
 */

#pragma once

#include "CoreMinimal.h"
#include "KCLobbySavedPlayerDataStruct.generated.h"

/**
 * @struct FKCLobbySavedPlayerDataStruct
 * @brief 세션 서브시스템에 저장되는 플레이어 로비 세션 데이터 구조체
 */
USTRUCT(BlueprintType)
struct PROJECTKC_API FKCLobbySavedPlayerDataStruct
{
	GENERATED_BODY()

public:
	FKCLobbySavedPlayerDataStruct()
		: PlayerName(TEXT(""))
		, TeamId(0)
		, SlotIndex(INDEX_NONE)
	{
	}

	FKCLobbySavedPlayerDataStruct(const FString& InPlayerName, int32 InTeamId, int32 InSlotIndex)
		: PlayerName(InPlayerName)
		, TeamId(InTeamId)
		, SlotIndex(InSlotIndex)
	{
	}

public:
	/** @brief 플레이어 표시 닉네임 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KC|Lobby")
	FString PlayerName;

	/** @brief 플레이어 소속 팀 ID (0: Red, 1: Blue) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KC|Lobby")
	int32 TeamId = 0;

	/** @brief 플레이어가 점유 중인 슬롯 인덱스 (0~5) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KC|Lobby")
	int32 SlotIndex = INDEX_NONE;
};
