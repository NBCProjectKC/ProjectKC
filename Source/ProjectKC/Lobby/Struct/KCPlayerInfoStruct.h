/**
 * @file KCPlayerInfoStruct.h
 * @brief 로비 시스템에서 플레이어 정보를 전달하고 동기화하기 위한 데이터 구조체 정의
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "KCPlayerInfoStruct.generated.h"

/**
 * @struct FKCPlayerInfoStruct
 * @brief 기존 S_PlayerInfo 블루프린트 구조체를 1:1 매핑한 C++ 구조체
 */
USTRUCT(BlueprintType)
struct PROJECTKC_API FKCPlayerInfoStruct
{
	GENERATED_BODY()

public:
	/** @brief 기본 생성자 */
	FKCPlayerInfoStruct()
		: PlayerName(TEXT(""))
		, PlayerState(nullptr)
		, bReady(false)
		, UniqueNetId(TEXT(""))
	{
	}

	/** @brief 편의 생성자 */
	FKCPlayerInfoStruct(const FString& InName, bool bInReady, APlayerState* InPS = nullptr, const FString& InNetId = TEXT(""))
		: PlayerName(InName)
		, PlayerState(InPS)
		, bReady(bInReady)
		, UniqueNetId(InNetId)
	{
	}

	bool operator==(const FKCPlayerInfoStruct& Other) const
	{
		return PlayerName == Other.PlayerName
			&& PlayerState == Other.PlayerState
			&& bReady == Other.bReady
			&& UniqueNetId == Other.UniqueNetId;
	}

	bool operator!=(const FKCPlayerInfoStruct& Other) const
	{
		return !(*this == Other);
	}

public:
	/** @brief 플레이어 표시 닉네임 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KC|Lobby")
	FString PlayerName;

	/** @brief 플레이어의 PlayerState 참조 (약참조) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KC|Lobby")
	TWeakObjectPtr<APlayerState> PlayerState = nullptr;

	/** @brief 플레이어 준비 완료(Ready) 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KC|Lobby")
	bool bReady = false;

	/** @brief 플레이어의 고유 넷 ID 문자열 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KC|Lobby")
	FString UniqueNetId;
};
