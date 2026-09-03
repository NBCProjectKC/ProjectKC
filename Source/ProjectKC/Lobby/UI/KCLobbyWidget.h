/**
 * @file KCLobbyWidget.h
 * @brief 로비 레벨의 메인 UI 위젯 클래스 정의 (레디 토글, 소셜 메뉴 열기, 게임 시작)
 */

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "KCLobbyWidget.generated.h"

class UButton;
class UTextBlock;
class UKCCustomizationWidget;
class UKCFriendListWidget;
class UWidgetAnimation;

/**
 * @class UKCLobbyWidget
 * @brief 기존 WBP_LobbyUI 위젯을 1:1 매핑한 C++ 위젯 클래스
 */
UCLASS()
class PROJECTKC_API UKCLobbyWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** @brief 매치 시작 애니메이션을 정방향 재생하고 입력을 비활성화합니다. */
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby|UI")
	void PlayMatchStartAnim();

	/** @brief StartGame 버튼의 활성화(클릭 가능) 여부를 설정합니다. */
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby|UI")
	void SetStartGameButtonEnabled(bool bEnabled);

	/** @brief PlayerState에 안전하게 바인딩을 시도합니다. 바인딩 성공 시 true 반환 */
	bool TryBindPlayerState();

	/** 커스터마이징 위젯이 닫힐 때 로비 UI를 복원합니다. */
	void NotifyCustomizationWidgetClosed(UKCCustomizationWidget* ClosedWidget);

protected:
	//~UUserWidget interface
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	//~End of UUserWidget interface

	/** @brief 친구 목록 열기/닫기 토글 버튼 바인딩 */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "KC|Lobby|UI")
	TObjectPtr<UButton> Button_Socials;

	/** @brief 준비 상태 토글(READY / CANCEL READY) 버튼 바인딩 */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "KC|Lobby|UI")
	TObjectPtr<UButton> Button_Ready;

	/** @brief 방장 전용 게임 시작 버튼 바인딩 */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "KC|Lobby|UI")
	TObjectPtr<UButton> Button_StartGame;

	/** 이름만 맞춰 배치하면 C++에서 자동으로 커스터마이징 UI를 엽니다. */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "KC|Lobby|UI")
	TObjectPtr<UButton> Button_Customization;

	/** @brief 준비 버튼 내부의 텍스트 블록 바인딩 */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "KC|Lobby|UI")
	TObjectPtr<UTextBlock> Text_Ready;

	/** @brief 현재 소속 팀명 표시 텍스트 블록 바인딩 (Optional) */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "KC|Lobby|UI")
	TObjectPtr<UTextBlock> Text_TeamName;

	/** @brief 내부 임베드된 친구 목록 서브 위젯 */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "KC|Lobby|UI")
	TObjectPtr<UKCFriendListWidget> WBP_FriendList;

	/** @brief 게임 시작 시 재생할 위젯 애니메이션 */
	UPROPERTY(Transient, meta = (BindWidgetAnimOptional), BlueprintReadOnly, Category = "KC|Lobby|UI")
	TObjectPtr<UWidgetAnimation> MatchStartAnim;

	/** 비워두면 기본 경로의 WBP_Customization을 자동 탐색합니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Lobby|Customization")
	TSubclassOf<UKCCustomizationWidget> CustomizationWidgetClass;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "KC|Lobby|Customization")
	TObjectPtr<UKCCustomizationWidget> CustomizationWidgetInstance;

	/** @brief 소셜 버튼 클릭 핸들러 */
	UFUNCTION()
	virtual void OnSocialsClicked();

	/** @brief 준비 버튼 클릭 핸들러 */
	UFUNCTION()
	virtual void OnReadyClicked();

	/** @brief 게임 시작 버튼 클릭 핸들러 */
	UFUNCTION()
	virtual void OnStartGameClicked();

	UFUNCTION()
	virtual void OnCustomizationClicked();

	/** @brief 플레이어 레디 상태 변경 시 UI 텍스트를 갱신하는 콜백 핸들러 */
	UFUNCTION()
	virtual void OnReadyStatusUpdated(bool bIsReady);

	/** @brief 플레이어 팀 ID 변경 시 호출되는 콜백 핸들러 */
	UFUNCTION()
	virtual void OnTeamIdUpdated(int32 NewTeamId);

private:
	/** @brief PlayerState와의 바인딩 및 초기화 완료 여부 */
	bool bPlayerStateBound = false;

	/** @brief PlayerState 바인딩 재시도 타이머 핸들 (NativeTick 대신 0.05초 간격 경량 타이머 사용) */
	FTimerHandle PlayerStateBindRetryTimerHandle;

	/** @brief 재시도 횟수 제한 */
	int32 BindRetryCount = 0;
};
