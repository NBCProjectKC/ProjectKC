/**
 * @file KCFriendWidget.h
 * @brief 스팀 친구 목록의 개별 친구 항목 UI (아바타, 이름, 온라인 여부, 초대 버튼)
 */

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BlueprintDataDefinitions.h"
#include "KCFriendWidget.generated.h"

class UButton;
class UTextBlock;
class UImage;

/**
 * @class UKCFriendWidget
 * @brief 기존 WBP_Friend 위젯을 1:1 매핑한 C++ 위젯 클래스
 */
UCLASS()
class PROJECTKC_API UKCFriendWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** @brief 스팀 친구 정보(FBPFriendInfo)를 받아 아바타, 닉네임, 온라인 투명도를 갱신합니다. */
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby|UI")
	void Update(const FBPFriendInfo& FriendData);

	/** @brief 친구 이름 및 고유 Net ID를 직접 설정합니다. */
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby|UI")
	void SetupFriend(const FString& InFriendName, const FString& InFriendUniqueNetId);

protected:
	//~UUserWidget interface
	virtual void NativeConstruct() override;
	//~End of UUserWidget interface

	/** @brief 스팀 프로필 아바타 이미지 위젯 */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "KC|Lobby|UI")
	TObjectPtr<UImage> Image_Avatar;

	/** @brief 친구 닉네임 텍스트 블록 */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "KC|Lobby|UI")
	TObjectPtr<UTextBlock> Text_PlayerName;

	/** @brief 세션 초대 버튼 */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "KC|Lobby|UI")
	TObjectPtr<UButton> Button_Invite;

	/** @brief 캐싱된 스팀 친구 정보 구조체 */
	UPROPERTY(BlueprintReadOnly, Category = "KC|Lobby|UI")
	FBPFriendInfo CachedFriendData;

	/** @brief 문자열 형태의 친구 고유 Net ID */
	UPROPERTY(BlueprintReadOnly, Category = "KC|Lobby|UI")
	FString FriendUniqueNetId;

	/** @brief 초대 버튼 클릭 시 세션 초대를 전송하는 핸들러 */
	UFUNCTION()
	virtual void OnInviteClicked();
};
