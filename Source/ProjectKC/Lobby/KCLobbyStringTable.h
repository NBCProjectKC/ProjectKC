/**
 * @file KCLobbyStringTable.h
 * @brief 로비 및 세션 상황별 안내 메시지 관리용 스트링 테이블
 */

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ProjectKC/Lobby/Enum/KCLobbyMessageType.h"
#include "KCLobbyStringTable.generated.h"

/**
 * @class UKCLobbyStringTable
 * @brief 로비 및 세션 안내 메시지 스트링 테이블 관리 및 조회 라이브러리 (C++ / 블루프린트 공용)
 */
UCLASS()
class PROJECTKC_API UKCLobbyStringTable : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static const FName TableId;
	static const FName StringTablePath;

	/** 상황 Enum에 해당하는 로비 안내 메시지 FText 반환 (C++ 및 블루프린트 공용) */
	UFUNCTION(BlueprintPure, Category = "KC|Lobby|Message")
	static FText GetMessage(EKCLobbyMessageType MessageType);

	/** 스트링 테이블 키(FName)에 해당하는 FText 반환 (C++ 및 블루프린트 공용) */
	UFUNCTION(BlueprintPure, Category = "KC|Lobby|Message")
	static FText GetMessageByKey(FName Key);

	/** Enum에 해당하는 스트링 테이블 키 이름 반환 */
	static FName GetKeyName(EKCLobbyMessageType MessageType);

private:
	/** 스트링 테이블 에셋이 메모리에 로드되어 있는지 확인하고 필요 시 로드 */
	static void EnsureStringTableLoaded();
};
