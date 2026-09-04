// Copyright Shared Orbit 2026. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GenericPlatform/ICursor.h"
#include "Layout/Margin.h"
#include "MeshPaintingCoreTypes.h"
#include "Painting/RuntimeMeshPaintTargetInterface.h"
#include "Widgets/ColorPickerTypes.h"
#include "ColorPickerPanelWidget.generated.h"

class APlayerController;
class UBorder;
class UColorPickerPaletteDataAsset;
class UColorPickerPaletteStorage;
class UColorPickerState;
class UColorChannelRowWidget;
class UColorHistoryBarWidget;
class UColorOptionRowWidget;
class UColorPaletteGridWidget;
class UColorPreviewWidget;
class UColorPickerToolStripWidget;
class UColorWheelWidget;
class UHexColorInputWidget;
class UHorizontalBox;
class UInputComponent;
class UPaintingModeControllerComponent;
class URuntimeMeshPaintTargetComponent;
class UWidget;
class UVerticalBox;
class UVerticalColorBarWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnColorPickerColorChanged, FLinearColor, Color);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnColorPickerColorCommitted, FLinearColor, Color);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnColorPickerBoolOptionChanged, FName, OptionId, bool, bIsEnabled);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnColorPickerScalarChanged, FName, ScalarId, float, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnColorPickerScalarCommitted, FName, ScalarId, float, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnColorPickerBrushSettingsChanged, const FMeshPaintBrushMaterialSettings&, Settings);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnColorPickerToolClicked, FName, ToolId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnColorPickerToolToggled, FName, ToolId, bool, bIsChecked);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnColorPickerPaletteColorSelected, FLinearColor, Color, int32, ColorIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnColorPickerEyedropperRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnColorPickerEyedropperActiveChanged, bool, bIsActive);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnColorPickerColorSampled, const FRuntimeMeshPaintSampleResult&, SampleResult);

UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "Color Picker Panel Widget"))
class MESHPAINTINGCORE_API UColorPickerPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UColorPickerPanelWidget(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color Picker")
	FLinearColor CurrentColor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color Picker")
	FLinearColor PreviousColor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color Picker", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float BrushSize;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color Picker")
	float Metallic;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color Picker")
	float Roughness;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Picker")
	bool bPushSettingsToPaintTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Picker|Palette")
	TObjectPtr<UColorPickerPaletteDataAsset> PaletteDataAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Picker|Palette")
	FString PaletteSaveSlotName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Picker|Eyedropper")
	TEnumAsByte<ECollisionChannel> EyedropperTraceChannel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Picker|Eyedropper")
	bool bEyedropperTraceComplex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Picker|Eyedropper")
	ERuntimeMeshPaintColorSampleMode EyedropperSampleMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Picker|Integration")
	TScriptInterface<class IRuntimeMeshPaintTargetInterface> PaintTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Picker|Layout")
	bool bBuildNativeLayoutIfMissing;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Picker|Layout")
	bool bRecentColorsExpanded;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Picker|Layout")
	FMargin PanelPadding;

	UPROPERTY(BlueprintAssignable, Category = "Color Picker|Events")
	FOnColorPickerColorChanged OnColorChanged;

	UPROPERTY(BlueprintAssignable, Category = "Color Picker|Events")
	FOnColorPickerColorCommitted OnColorCommitted;

	UPROPERTY(BlueprintAssignable, Category = "Color Picker|Events")
	FOnColorPickerBoolOptionChanged OnOptionChanged;

	UPROPERTY(BlueprintAssignable, Category = "Color Picker|Events")
	FOnColorPickerScalarChanged OnScalarChanged;

	UPROPERTY(BlueprintAssignable, Category = "Color Picker|Events")
	FOnColorPickerScalarCommitted OnScalarCommitted;

	UPROPERTY(BlueprintAssignable, Category = "Color Picker|Events")
	FOnColorPickerBrushSettingsChanged OnBrushSettingsChanged;

	UPROPERTY(BlueprintAssignable, Category = "Color Picker|Events")
	FOnColorPickerToolClicked OnToolClicked;

	UPROPERTY(BlueprintAssignable, Category = "Color Picker|Events")
	FOnColorPickerToolToggled OnToolToggled;

	UPROPERTY(BlueprintAssignable, Category = "Color Picker|Events")
	FOnColorPickerPaletteColorSelected OnPaletteColorSelected;

	UPROPERTY(BlueprintAssignable, Category = "Color Picker|Events")
	FOnColorPickerEyedropperRequested OnEyedropperRequested;

	UPROPERTY(BlueprintAssignable, Category = "Color Picker|Events")
	FOnColorPickerEyedropperActiveChanged OnEyedropperActiveChanged;

	UPROPERTY(BlueprintAssignable, Category = "Color Picker|Events")
	FOnColorPickerColorSampled OnColorSampled;

	UFUNCTION(BlueprintCallable, Category = "Color Picker")
	void SetCurrentColor(FLinearColor NewColor, bool bBroadcast = false);

	UFUNCTION(BlueprintCallable, Category = "Color Picker")
	void CommitCurrentColor();

	UFUNCTION(BlueprintCallable, Category = "Color Picker")
	void SetPreviousColor(FLinearColor NewColor);

	UFUNCTION(BlueprintCallable, Category = "Color Picker")
	void SetBrushSize(float NewBrushSize, bool bBroadcast = false);

	UFUNCTION(BlueprintCallable, Category = "Color Picker")
	void SetMetallic(float NewMetallic, bool bBroadcast = false);

	UFUNCTION(BlueprintCallable, Category = "Color Picker")
	void SetRoughness(float NewRoughness, bool bBroadcast = false);

	UFUNCTION(BlueprintCallable, Category = "Color Picker")
	void CommitScalar(FName ScalarId);

	UFUNCTION(BlueprintCallable, Category = "Color Picker")
	void SetPaintTarget(UObject* NewPaintTarget);

	UFUNCTION(BlueprintCallable, Category = "Color Picker")
	void ApplyBrushSettingsToPaintTarget();

	UFUNCTION(BlueprintCallable, Category = "Color Picker|Palette")
	void AddCurrentColorToCustomPalette();

	UFUNCTION(BlueprintCallable, Category = "Color Picker|Palette")
	void RemoveCustomPaletteColorAt(int32 CustomColorIndex);

	UFUNCTION(BlueprintCallable, Category = "Color Picker|Palette")
	void RestoreDefaultPalette();

	UFUNCTION(BlueprintCallable, Category = "Color Picker|Palette")
	void RecordCurrentColorAsRecent();

	UFUNCTION(BlueprintCallable, Category = "Color Picker|Palette")
	void NotifyPaintApplied();

	UFUNCTION(BlueprintCallable, Category = "Color Picker|Palette")
	void SetRecentColorsExpanded(bool bExpanded);

	UFUNCTION(BlueprintCallable, Category = "Color Picker|Tools")
	void SetEraserActive(bool bActive, bool bBroadcast = true);

	UFUNCTION(BlueprintCallable, Category = "Color Picker|Eyedropper")
	bool BeginEyedropperMode();

	UFUNCTION(BlueprintCallable, Category = "Color Picker|Eyedropper")
	void CancelEyedropperMode();

	UFUNCTION(BlueprintCallable, Category = "Color Picker|Eyedropper")
	bool ToggleEyedropperMode();

	/** Samples the color beneath the cursor immediately using the same path as the eyedropper confirm action. */
	UFUNCTION(BlueprintCallable, Category = "Color Picker|Eyedropper")
	bool SampleColorUnderCursor();

	UColorWheelWidget* GetColorWheel() const { return ColorWheel; }
	UColorPaletteGridWidget* GetPaletteGrid() const { return PaletteGrid; }
	bool IsEyedropperActive() const { return bIsEyedropperActive; }
	bool IsEraserActive() const;
	bool IsCursorOverPanel() const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeOnInitialized() override;
	virtual void NativePreConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent) override;
	virtual FCursorReply NativeOnCursorQuery(const FGeometry& InGeometry, const FPointerEvent& InCursorEvent) override;

private:
	void BuildWidgetTree();
	void BuildTopSection(UVerticalBox* RootVerticalBox);
	void BuildMiddleSection(UVerticalBox* RootVerticalBox);
	void BuildBottomSection(UVerticalBox* RootVerticalBox);
	void ConfigureBoundWidgets();
	void BindChildWidgetEvents();
	void ConfigureChannelRows(bool bApplyDesignDefaults);
	void ConfigureChannelRow(
		UColorChannelRowWidget* Row, FName ChannelId, const TCHAR* Label,
		float InMinValue, float InMaxValue, int32 Decimals,
		EColorBarGradientMode InGradientMode, bool bApplyDesignDefaults);
	void EnsureToolStripWidget();
	void ConfigureToolStrip();
	void EnsureHistoryBarWidget();
	void ConfigureHistoryBar();
	void ApplyPreviewLayoutOrdering();
	void ApplyScalarLayoutOrdering();
	void ApplyColorModeToWidgets();
	void EnsureRuntimeObjects();
	void SynchronizeAllWidgets();
	void SynchronizeColorWidgets();
	void UpdateChannelGradients();
	void BroadcastBrushSettings();
	void BeginContinuousInteraction();
	void EndContinuousInteraction(bool bCommitColor);
	void SetEyedropperActiveVisual(bool bActive);
	void SetEraserActiveVisual(bool bActive);
	bool IsScalarChannel(FName ChannelId) const;
	bool GetScalarChannelValue(FName ChannelId, float& OutValue) const;
	bool SetScalarChannelValue(FName ChannelId, float Value, bool bBroadcast);
	bool SetRGBChannelValue(FName ChannelId, float Value);
	bool SetHSVChannelValue(FName ChannelId, float Value);
	void SetScalarValue(
		FName ScalarId, float NewValue, float MinValue, float MaxValue,
		float& StoredValue, UColorChannelRowWidget* Row, bool bBroadcast);
	float GetMaxBrushSize() const;
	float GetBrushRadiusFromNormalizedSize(float NormalizedBrushSize) const;
	float GetNormalizedBrushSizeFromRadius(float BrushRadius) const;
	TArray<UColorChannelRowWidget*> GetAllChannelRows() const;
	UColorChannelRowWidget* FindOrCreateChannelRow(FName WidgetName);
	void AddScalarRowToStack(UVerticalBox* ScalarRowsStack, UColorChannelRowWidget* Row, const FMargin& RowPadding);
	void NotifyPaintingUIPointerState(bool bNewIsPointerOverPanel);

	UColorChannelRowWidget* MakeChannelRow(
		UVerticalBox* Parent, FName ChannelId, const FString& Label,
		float MinValue, float MaxValue, int32 Decimals);

	UFUNCTION()
	void HandleWheelHSVChanged(float Hue, float Saturation, float Value);

	UFUNCTION()
	void HandleColorInteractionStarted();

	UFUNCTION()
	void HandleWheelHSVCommitted(float Hue, float Saturation, float Value);

	UFUNCTION()
	void HandleVerticalSaturationChanged(float Value);

	UFUNCTION()
	void HandleVerticalValueChanged(float Value);

	UFUNCTION()
	void HandleColorValueCommitted(float Value);

	UFUNCTION()
	void HandleChannelInteractionStarted(FName ChannelId, float InitialValue);

	UFUNCTION()
	void HandleChannelChanged(FName ChannelId, float Value);

	UFUNCTION()
	void HandleChannelCommitted(FName ChannelId, float Value);

	UFUNCTION()
	void HandleHexCommitted(const FString& HexText, FLinearColor Color, bool bIsValid, bool bHasAlpha);

	UFUNCTION()
	void HandleOptionChanged(FName OptionId, bool bIsChecked);

	UFUNCTION()
	void HandlePaletteColorSelected(FLinearColor Color, int32 ColorIndex);

	UFUNCTION()
	void HandlePaletteChanged(const TArray<FLinearColor>& Colors);

	UFUNCTION()
	void HandleToolStripButtonClicked(FName ToolId);

	UFUNCTION()
	void HandlePreviousPreviewClicked(FLinearColor Color);

	UFUNCTION()
	void HandleRuntimePaintApplied(FRuntimeMeshPaintSampleResult PaintResult);

	UFUNCTION()
	void HandlePaintingControllerBrushSizeChanged(float NewSize);

	void HandleEyedropperConfirm();
	void HandleEyedropperCancelInput();
	void BindRuntimePaintTarget(URuntimeMeshPaintTargetComponent* RuntimePaintTarget);
	void UnbindRuntimePaintTarget();
	void BindPaintingController(UPaintingModeControllerComponent* PaintingController);
	void UnbindPaintingController();

	UPROPERTY(Transient)
	TObjectPtr<UColorPickerState> ColorState;

	UPROPERTY(Transient)
	TObjectPtr<UColorPickerPaletteStorage> PaletteStorage;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Color Picker|Bound Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UBorder> RootBorder;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Color Picker|Bound Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UColorWheelWidget> ColorWheel;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Color Picker|Bound Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UVerticalColorBarWidget> SaturationBar;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Color Picker|Bound Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UVerticalColorBarWidget> ValueBar;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Color Picker|Bound Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UColorPreviewWidget> CurrentPreview;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Color Picker|Bound Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UColorPreviewWidget> PreviousPreview;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Color Picker|Bound Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UVerticalBox> PreviewAndToolsBox;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Color Picker|Bound Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UColorPickerToolStripWidget> ToolStrip;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Color Picker|Bound Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UWidget> ToolRow;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Color Picker|Bound Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UColorOptionRowWidget> SrgbPreviewOption;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Color Picker|Bound Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UHexColorInputWidget> HexInput;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Color Picker|Bound Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UVerticalBox> BottomBox;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Color Picker|Bound Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UColorHistoryBarWidget> HistoryBar;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Color Picker|Bound Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UWidget> RecentColorsArea;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Color Picker|Bound Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UColorPaletteGridWidget> PaletteGrid;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Color Picker|Bound Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UWidget> EyedropperButton;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Color Picker|Bound Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UColorChannelRowWidget> RedRow;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Color Picker|Bound Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UColorChannelRowWidget> GreenRow;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Color Picker|Bound Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UColorChannelRowWidget> BlueRow;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Color Picker|Bound Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UColorChannelRowWidget> HueRow;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Color Picker|Bound Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UColorChannelRowWidget> SaturationRow;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Color Picker|Bound Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UColorChannelRowWidget> ValueRow;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Color Picker|Bound Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UColorChannelRowWidget> BrushSizeRow;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Color Picker|Bound Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UColorChannelRowWidget> MetallicRow;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Color Picker|Bound Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UColorChannelRowWidget> RoughnessRow;

	UPROPERTY(Transient)
	TObjectPtr<UInputComponent> EyedropperInputComponent;

	UPROPERTY(Transient)
	TObjectPtr<URuntimeMeshPaintTargetComponent> BoundRuntimePaintTarget;

	UPROPERTY(Transient)
	TObjectPtr<UPaintingModeControllerComponent> BoundPaintingController;

	TWeakObjectPtr<APlayerController> EyedropperPlayerController;

	EMouseCursor::Type PreviousMouseCursor;

	bool bIsSynchronizing;
	bool bNativeTreeBuilt;
	bool bUsingNativeFallbackLayout;
	bool bIsEyedropperActive;
	bool bIsContinuousInteraction;
};
