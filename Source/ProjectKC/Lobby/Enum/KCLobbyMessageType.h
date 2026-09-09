#pragma once

#include "CoreMinimal.h"
#include "KCLobbyMessageType.generated.h"

/**
 * @enum EKCLobbyMessageType
 * @brief 로비 시스템에서 발생하는 주요 상황 타입
 */
UENUM(BlueprintType)
enum class EKCLobbyMessageType : uint8
{
	LobbyFull       UMETA(DisplayName = "로비 정원 초과"),
	HostClosed      UMETA(DisplayName = "방장 로비 종료"),
	HostLost        UMETA(DisplayName = "방장 연결 끊김"),
	SessionNotFound UMETA(DisplayName = "세션 찾을 수 없음"),
	NotAllReady     UMETA(DisplayName = "전원 준비 미완료")
};
