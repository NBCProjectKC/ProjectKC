/**
 * @file KCLobbyToastWidget.h
 * @brief 로비 및 세션 접속 실패 등 알림을 화면에 띄우고 자동으로 사라지게 하는 전용 토스트 위젯
 */

#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/UI/Common/Widget/KCUserWidget.h"
#include "KCLobbyToastWidget.generated.h"

class UTextBlock;
class UWidgetAnimation;

UCLASS(Abstract, Blueprintable)
class PROJECTKC_API UKCLobbyToastWidget : public UKCUserWidget
{
	GENERATED_BODY()

public:
	UKCLobbyToastWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** 토스트 메시지 설정 (C++ 텍스트 블록 갱신 및 블루프린트 이벤트 호출) */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "KC|Lobby|UI")
	void SetToastMessage(const FText& Message);
	virtual void SetToastMessage_Implementation(const FText& Message);

	/** 토스트 메시지를 표시하고 지정한 시간 후 자동 소멸 타이머 시작 */
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby|UI")
	void ShowToast(const FText& Message, float DisplayDuration = 3.0f);

	/** 토스트 즉시 닫기 */
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby|UI")
	void DismissToast();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** 메시지 텍스트 블록 바인딩 (WBP_LobbyToast의 Text_Message 또는 MessageText) */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "KC|Lobby|UI")
	TObjectPtr<UTextBlock> Text_Message;

	/** 등장 및 페이드 연출 애니메이션 */
	UPROPERTY(Transient, meta = (BindWidgetAnimOptional), BlueprintReadOnly, Category = "KC|Lobby|UI")
	TObjectPtr<UWidgetAnimation> Anim;

	/** 기본 표시 지속 시간 (초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Lobby|UI")
	float DefaultToastDuration = 3.0f;

private:
	FTimerHandle DismissTimerHandle;
	FText CachedMessage;

	UFUNCTION()
	void OnDismissTimerFired();
};
