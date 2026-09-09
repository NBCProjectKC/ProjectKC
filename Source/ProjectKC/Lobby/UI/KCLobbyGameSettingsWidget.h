#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameSystem/Enum/KCLevelType.h"
#include "KCLobbyGameSettingsWidget.generated.h"

class UKCLobbyWidget;
class UButton;

/**
 * @class UKCLobbyGameSettingsWidget
 * @brief 방장 전용 게임 세팅(인원수, 맵, 게임 시간) 팝업 모달 UI의 C++ 베이스 클래스
 */
UCLASS()
class PROJECTKC_API UKCLobbyGameSettingsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 로비 메인 위젯으로부터 현재 설정값을 받아 초기화 */
	void InitializeGameSettings(UKCLobbyWidget* InLobbyWidget, int32 InCount, EKCLevelType InMap, float InDuration);

	/** 인원수 선택 (2, 4, 6) */
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby|Settings")
	void SetSelectedPlayerCount(int32 InCount);

	/** 맵 선택 (GasRange, FryingPan, PortableGasStove 등) */
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby|Settings")
	void SetSelectedMapType(EKCLevelType InMapType);

	/** 게임 시간 선택 (초 단위, 예: 180, 300, 420) */
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby|Settings")
	void SetSelectedMatchDuration(float InSeconds);

	/** 설정 변경사항을 서버에 확정 적용하고 팝업 닫기 */
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby|Settings")
	void ConfirmAndApplySettings();

	/** 변경사항 적용 없이 팝업 닫기 */
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby|Settings")
	void CloseSettings();

	UFUNCTION(BlueprintPure, Category = "KC|Lobby|Settings")
	int32 GetSelectedPlayerCount() const { return SelectedPlayerCount; }

	UFUNCTION(BlueprintPure, Category = "KC|Lobby|Settings")
	EKCLevelType GetSelectedMapType() const { return SelectedMapType; }

	UFUNCTION(BlueprintPure, Category = "KC|Lobby|Settings")
	float GetSelectedMatchDuration() const { return SelectedMatchDuration; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** 블루프린트에서 UI 컴포넌트(버튼 하이라이트, 콤보박스 선택 등) 초기화용 이벤트 */
	UFUNCTION(BlueprintImplementableEvent, Category = "KC|Lobby|Settings|Events")
	void OnSettingsInitialized(int32 InCount, EKCLevelType InMap, float InDuration);

	/** 블루프린트에서 설정 값 변경 시 호출되는 이벤트 */
	UFUNCTION(BlueprintImplementableEvent, Category = "KC|Lobby|Settings|Events")
	void OnSettingValuesChanged(int32 InCount, EKCLevelType InMap, float InDuration);

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "KC|Lobby|Settings|UI")
	TObjectPtr<UButton> Button_Apply;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "KC|Lobby|Settings|UI")
	TObjectPtr<UButton> Button_Close;

	UPROPERTY(BlueprintReadOnly, Category = "KC|Lobby|Settings")
	int32 SelectedPlayerCount = 6;

	UPROPERTY(BlueprintReadOnly, Category = "KC|Lobby|Settings")
	EKCLevelType SelectedMapType = EKCLevelType::PortableGasStove;

	UPROPERTY(BlueprintReadOnly, Category = "KC|Lobby|Settings")
	float SelectedMatchDuration = 300.0f;

	UPROPERTY()
	TWeakObjectPtr<UKCLobbyWidget> OwnerLobbyWidget;

	UFUNCTION()
	virtual void OnApplyButtonClicked();

	UFUNCTION()
	virtual void OnCloseButtonClicked();
};
