#pragma once

#include "CoreMinimal.h"
#include "KCLevelType.generated.h"

/*
 * GameFlow state 관리를 위한 레벨 Enum 파일입니다.
 */

UENUM(BlueprintType)
enum class EKCLevelType : uint8
{
	None			UMETA(DisplayName = "없음"),
	SplashScreen	UMETA(DisplayName = "스플래시"),
	MainMenu		UMETA(DisplayName = "메인메뉴"),
	LoadInLevel		UMETA(DisplayName = "로드인(세션생성)"),
	LobbyLevel		UMETA(DisplayName = "로비"),
	Loading			UMETA(DisplayName = "로딩"),
	GasRange		UMETA(DisplayName = "가스레인지(전투)")
};