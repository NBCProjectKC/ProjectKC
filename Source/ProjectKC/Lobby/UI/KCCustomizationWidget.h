#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Customization/KCCustomizationSaveGame.h"
#include "KCCustomizationWidget.generated.h"

class AKCLobbyPlayerController;
class UButton;
class UCheckBox;
class UKCLobbyWidget;
class UPaintingModeControllerComponent;
class USlider;
class UTextBlock;

/** 로비 캐릭터 직접 페인팅 UI의 C++ 동작 베이스입니다. */
UCLASS()
class PROJECTKC_API UKCCustomizationWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeLobbyWidget(UKCLobbyWidget* InLobbyWidget);

	/** 팔레트 버튼이나 별도 Color Picker가 사용할 색상 진입점입니다. */
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby|Customization|UI")
	void SetBrushColor(FLinearColor NewColor);

	UFUNCTION(BlueprintCallable, Category = "KC|Lobby|Customization|UI")
	void SetEraserEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "KC|Lobby|Customization|UI")
	AKCLobbyPlayerController* GetLobbyPlayerController() const
	{
		return LobbyPlayerController;
	}

	UFUNCTION(BlueprintPure, Category = "KC|Lobby|Customization|UI")
	UPaintingModeControllerComponent* GetPaintingController() const
	{
		return PaintingController;
	}

	/** 로비 종료 등 외부 요청에서 저장하지 않고 안전하게 닫습니다. */
	void RequestCancelAndClose();

protected:
	//~UUserWidget interface
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	//~End of UUserWidget interface

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "KC|Customization|UI")
	TObjectPtr<UButton> Button_Save;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "KC|Customization|UI")
	TObjectPtr<UButton> Button_Cancel;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "KC|Customization|UI")
	TObjectPtr<UButton> Button_Reset;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "KC|Customization|UI")
	TObjectPtr<UButton> Button_ResetCamera;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "KC|Customization|UI")
	TObjectPtr<USlider> Slider_BrushSize;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "KC|Customization|UI")
	TObjectPtr<UCheckBox> CheckBox_Eraser;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "KC|Customization|UI")
	TObjectPtr<UButton> Button_ColorBlack;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "KC|Customization|UI")
	TObjectPtr<UButton> Button_ColorWhite;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "KC|Customization|UI")
	TObjectPtr<UButton> Button_ColorRed;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "KC|Customization|UI")
	TObjectPtr<UButton> Button_ColorBlue;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "KC|Customization|UI")
	TObjectPtr<UButton> Button_ColorGreen;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "KC|Customization|UI")
	TObjectPtr<UButton> Button_ColorYellow;

	/** 실패 이유를 자동 표시합니다. 없어도 기능은 동작합니다. */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "KC|Customization|UI")
	TObjectPtr<UTextBlock> Text_Result;

	/** 별도 연출이나 팝업이 필요할 때만 Blueprint에서 구현합니다. */
	UFUNCTION(BlueprintImplementableEvent, Category = "KC|Lobby|Customization|UI")
	void OnCustomizationOperationFailed(EKCCustomizationSaveResult Result);

private:
	UFUNCTION()
	void HandleSaveClicked();

	UFUNCTION()
	void HandleCancelClicked();

	UFUNCTION()
	void HandleResetClicked();

	UFUNCTION()
	void HandleResetCameraClicked();

	UFUNCTION()
	void HandleBrushSizeChanged(float NormalizedValue);

	UFUNCTION()
	void HandleEraserChanged(bool bIsChecked);

	UFUNCTION()
	void HandleBlackClicked();

	UFUNCTION()
	void HandleWhiteClicked();

	UFUNCTION()
	void HandleRedClicked();

	UFUNCTION()
	void HandleBlueClicked();

	UFUNCTION()
	void HandleGreenClicked();

	UFUNCTION()
	void HandleYellowClicked();

	void BindControls();
	void UnbindControls();
	void SetOperationControlsEnabled(bool bEnabled);
	void ReportOperationFailure(EKCCustomizationSaveResult Result);
	void CloseWidget();

	UPROPERTY(Transient)
	TObjectPtr<AKCLobbyPlayerController> LobbyPlayerController;

	UPROPERTY(Transient)
	TObjectPtr<UPaintingModeControllerComponent> PaintingController;

	UPROPERTY(Transient)
	TObjectPtr<UKCLobbyWidget> OwningLobbyWidget;

	bool bClosing = false;
	bool bOperationInProgress = false;
};
