/**
 * @file KCPlayerInfoWidget.h
 * @brief 캐릭터 머리 위에 3D 위젯으로 표시되는 플레이어 닉네임 및 준비 상태 표시 UI
 */

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ProjectKC/Lobby/Struct/KCPlayerInfoStruct.h"
#include "KCPlayerInfoWidget.generated.h"

class UTextBlock;

/**
 * @class UKCPlayerInfoWidget
 * @brief 기존 WBP_PlayerInfo 블루프린트를 1:1 매핑한 C++ 위젯 클래스
 */
UCLASS()
class PROJECTKC_API UKCPlayerInfoWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** @brief 플레이어 정보(닉네임, 준비 상태)를 받아 UI 텍스트 및 투명도를 갱신합니다. */
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby|UI")
	void UpdatePlayerInfo(const FKCPlayerInfoStruct& InInfo);

protected:
	/** @brief 플레이어 닉네임 텍스트 블록 바인딩 */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "KC|Lobby|UI")
	TObjectPtr<UTextBlock> Text_PlayerName;

	/** @brief 준비 상태(READY / NOT READY) 텍스트 블록 바인딩 */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "KC|Lobby|UI")
	TObjectPtr<UTextBlock> Text_ReadyStatus;

	/** @brief 캐싱된 플레이어 정보 구조체 */
	UPROPERTY(BlueprintReadOnly, Category = "KC|Lobby|UI")
	FKCPlayerInfoStruct PlayerInfo;
};
