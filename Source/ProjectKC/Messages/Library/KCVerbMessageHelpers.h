/**
 * @file KCVerbMessageHelpers.h
 * @brief VerbMessage 변환 및 플레이어 상태 추출용 블루프린트 함수 라이브러리 정의
 */

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ProjectKC/Messages/Struct/KCVerbMessageStruct.h"
#include "KCVerbMessageHelpers.generated.h"

class APlayerController;
class APlayerState;
class UObject;

/**
 * @class UKCVerbMessageHelpers
 * @brief VerbMessage와 관련된 유틸리티 및 변환 함수 모음 라이브러리
 */
UCLASS()
class PROJECTKC_API UKCVerbMessageHelpers : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * @brief 오브젝트(Pawn, PlayerController, PlayerState, Component 등)로부터 APlayerState를 추출합니다.
	 * @param Object 대상 오브젝트
	 * @return APlayerState* 플레이어 상태 포인터
	 */
	UFUNCTION(BlueprintCallable, Category = "ProjectKC|Messages")
	static APlayerState* GetPlayerStateFromObject(UObject* Object);

	/**
	 * @brief 오브젝트(Pawn, PlayerState, Controller, Component 등)로부터 APlayerController를 추출합니다.
	 * @param Object 대상 오브젝트
	 * @return APlayerController* 플레이어 컨트롤러 포인터
	 */
	UFUNCTION(BlueprintCallable, Category = "ProjectKC|Messages")
	static APlayerController* GetPlayerControllerFromObject(UObject* Object);
};
