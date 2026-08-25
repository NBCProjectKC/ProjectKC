/**
 * @file KCFriendListWidget.h
 * @brief 스팀 친구 목록을 주기적으로 갱신하고 위젯 풀링으로 표시하는 소셜 목록 UI 클래스
 */

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BlueprintDataDefinitions.h"
#include "KCFriendListWidget.generated.h"

class UPanelWidget;
class UKCFriendWidget;

/**
 * @class UKCFriendListWidget
 * @brief 기존 WBP_FriendList 위젯을 1:1 매핑한 C++ 위젯 클래스
 */
UCLASS()
class PROJECTKC_API UKCFriendListWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UKCFriendListWidget(const FObjectInitializer& ObjectInitializer);

	/** @brief 친구 목록을 갱신합니다. (NativeConstruct 시 5초 주기의 타이머로 자동 호출됨) */
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby|UI")
	virtual void RefreshFriendList();

protected:
	//~UUserWidget interface
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	//~End of UUserWidget interface

	/** @brief 기존 블루프린트의 FriendList (VerticalBox 등 패널 위젯) 자동 바인딩 */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "KC|Lobby|UI")
	TObjectPtr<UPanelWidget> FriendList;

	/** @brief 개별 친구 항목으로 생성할 WBP_Friend 위젯 클래스 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Lobby|UI")
	TSubclassOf<UKCFriendWidget> FriendEntryWidgetClass;

	/** @brief 생성된 위젯 재사용을 위한 풀링 배열 */
	UPROPERTY(BlueprintReadOnly, Category = "KC|Lobby|UI")
	TArray<TObjectPtr<UKCFriendWidget>> AddedFriendWidgets;

private:
	/** @brief 친구 목록 자동 갱신을 위한 타이머 핸들 */
	FTimerHandle FriendListRefreshTimerHandle;

	/** @brief OSS 친구 목록 읽기 완료 콜백 핸들러 */
	void HandleReadFriendsListComplete(int32 LocalUserNum, bool bWasSuccessful, const FString& ListName, const FString& ErrorStr);
};
