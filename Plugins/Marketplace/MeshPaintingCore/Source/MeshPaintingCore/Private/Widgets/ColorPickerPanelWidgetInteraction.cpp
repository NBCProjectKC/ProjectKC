// Copyright Shared Orbit 2026. All Rights Reserved.
#include "Widgets/ColorPickerPanelWidget.h"

#include "ColorPickerPanelWidgetPrivate.h"
#include "ColorPicker/ColorPickerState.h"
#include "Components/InputComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Painting/PaintingModeControllerComponent.h"
#include "Templates/UnrealTemplate.h"
#include "Widgets/ColorHistoryBarWidget.h"
#include "Widgets/ColorOptionRowWidget.h"
#include "Widgets/ColorPickerEyedropperUtils.h"
#include "Widgets/ColorPickerToolStripWidget.h"
#include "Widgets/ColorPreviewWidget.h"
#include "Widgets/ColorWheelWidget.h"
#include "Widgets/HexColorInputWidget.h"
#include "Widgets/VerticalColorBarWidget.h"

bool UColorPickerPanelWidget::BeginEyedropperMode()
{
	if (bIsEyedropperActive) return true;

	APlayerController* PlayerController = GetOwningPlayer();
	if (!PlayerController) return false;

	EyedropperPlayerController = PlayerController;
	PreviousMouseCursor = PlayerController->CurrentMouseCursor.GetValue();
	PlayerController->CurrentMouseCursor = EMouseCursor::EyeDropper;
	PlayerController->bShowMouseCursor = true;

	EyedropperInputComponent = NewObject<UInputComponent>(PlayerController, TEXT("RuntimeColorPickerEyedropperInput"));
	if (!EyedropperInputComponent)
	{
		PlayerController->CurrentMouseCursor = PreviousMouseCursor;
		EyedropperPlayerController.Reset();
		return false;
	}

	EyedropperInputComponent->Priority = 1000;
	EyedropperInputComponent->bBlockInput = true;
	EyedropperInputComponent->RegisterComponentWithWorld(PlayerController->GetWorld());

	EyedropperInputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &UColorPickerPanelWidget::HandleEyedropperConfirm);
	EyedropperInputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &UColorPickerPanelWidget::HandleEyedropperCancelInput);
	EyedropperInputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &UColorPickerPanelWidget::HandleEyedropperCancelInput);
	PlayerController->PushInputComponent(EyedropperInputComponent);

	bIsEyedropperActive = true;
	SetEyedropperActiveVisual(true);
	OnEyedropperRequested.Broadcast();
	OnEyedropperActiveChanged.Broadcast(true);
	return true;
}

void UColorPickerPanelWidget::CancelEyedropperMode()
{
	if (!bIsEyedropperActive) return;

	if (APlayerController* PlayerController = EyedropperPlayerController.Get())
	{
		if (EyedropperInputComponent) PlayerController->PopInputComponent(EyedropperInputComponent);
		PlayerController->CurrentMouseCursor = PreviousMouseCursor;
	}

	if (EyedropperInputComponent)
	{
		EyedropperInputComponent->DestroyComponent();
		EyedropperInputComponent = nullptr;
	}

	bIsEyedropperActive = false;
	EyedropperPlayerController.Reset();
	SetEyedropperActiveVisual(false);
	OnEyedropperActiveChanged.Broadcast(false);
}

bool UColorPickerPanelWidget::ToggleEyedropperMode()
{
	const bool bWasActive = bIsEyedropperActive;
	if (bIsEyedropperActive) CancelEyedropperMode();
	else BeginEyedropperMode();

	if (bWasActive != bIsEyedropperActive) OnToolToggled.Broadcast(ToolEyedropper, bIsEyedropperActive);
	OnToolClicked.Broadcast(ToolEyedropper);
	return bIsEyedropperActive;
}

void UColorPickerPanelWidget::SynchronizeAllWidgets()
{
	SynchronizeColorWidgets();
	SetBrushSize(BrushSize, false);
	SetMetallic(Metallic, false);
	SetRoughness(Roughness, false);

	if (SrgbPreviewOption && ColorState) SrgbPreviewOption->SetIsChecked(ColorState->bSRGBPreview, false);
	ConfigureHistoryBar();
}

void UColorPickerPanelWidget::SynchronizeColorWidgets()
{
	TGuardValue<bool> Guard(bIsSynchronizing, true);

	float Hue = 0.0f;
	float Saturation = 0.0f;
	float Value = 0.0f;
	GetHSV(CurrentColor, Hue, Saturation, Value);
	const FLinearColor DisplayColor = ColorState && ColorState->bSRGBPreview ? MakeSRGBPreviewColor(CurrentColor) : CurrentColor;

	if (ColorWheel)
	{
		ColorWheel->SetHueSaturation(Hue, Saturation, false);
		ColorWheel->SetValueLevel(Value);
	}
	if (ValueBar) ValueBar->SetValue(Value, false);
	if (SaturationBar) SaturationBar->SetValue(Saturation, false);
	if (CurrentPreview) CurrentPreview->SetColor(DisplayColor);
	if (PreviousPreview) PreviousPreview->SetColor(PreviousColor);
	if (HistoryBar)
	{
		HistoryBar->SetUseSRGB(ColorState ? ColorState->bSRGBPreview : true);
		HistoryBar->SetActiveColor(CurrentColor);
	}

	if (RedRow) RedRow->SetValue(CurrentColor.R, false);
	if (GreenRow) GreenRow->SetValue(CurrentColor.G, false);
	if (BlueRow) BlueRow->SetValue(CurrentColor.B, false);
	if (HueRow) HueRow->SetValue(Hue, false);
	if (SaturationRow) SaturationRow->SetValue(Saturation, false);
	if (ValueRow)
	{
		if (bUsingNativeFallbackLayout) ValueRow->SetLabel(FText::FromString(TEXT("V")));
		ValueRow->SetValue(Value, false);
	}
	if (HexInput) HexInput->SetHexFromColor(CurrentColor, false);

	UpdateChannelGradients();
}

void UColorPickerPanelWidget::UpdateChannelGradients()
{
	float Hue = 0.0f;
	float Saturation = 0.0f;
	float Value = 0.0f;
	GetHSV(CurrentColor, Hue, Saturation, Value);

	if (SaturationBar) SaturationBar->SetGradientColors(ColorFromHSV(Hue, 0.0f, Value), ColorFromHSV(Hue, 1.0f, Value));
	if (ValueBar) ValueBar->SetGradientColors(FLinearColor::Black, ColorFromHSV(Hue, Saturation, 1.0f));
	if (RedRow)
		RedRow->SetGradient(
			ColorFromRGB(0.0f, CurrentColor.G, CurrentColor.B),
			ColorFromRGB(1.0f, CurrentColor.G, CurrentColor.B),
			EColorBarGradientMode::TwoColor);
	if (GreenRow)
		GreenRow->SetGradient(
			ColorFromRGB(CurrentColor.R, 0.0f, CurrentColor.B),
			ColorFromRGB(CurrentColor.R, 1.0f, CurrentColor.B),
			EColorBarGradientMode::TwoColor);
	if (BlueRow)
		BlueRow->SetGradient(
			ColorFromRGB(CurrentColor.R, CurrentColor.G, 0.0f),
			ColorFromRGB(CurrentColor.R, CurrentColor.G, 1.0f),
			EColorBarGradientMode::TwoColor);
	if (HueRow) HueRow->SetGradient(FLinearColor::Red, FLinearColor::Red, EColorBarGradientMode::Hue);
	if (SaturationRow)
		SaturationRow->SetGradient(ColorFromHSV(Hue, 0.0f, Value), ColorFromHSV(Hue, 1.0f, Value), EColorBarGradientMode::TwoColor);
	if (ValueRow) ValueRow->SetGradient(FLinearColor::Black, ColorFromHSV(Hue, Saturation, 1.0f), EColorBarGradientMode::TwoColor);
	const FLinearColor ScalarTrackColor(0.015f, 0.016f, 0.018f, 1.0f);
	if (BrushSizeRow) BrushSizeRow->SetGradient(ScalarTrackColor, ScalarTrackColor, EColorBarGradientMode::TwoColor);
	if (MetallicRow) MetallicRow->SetGradient(ScalarTrackColor, ScalarTrackColor, EColorBarGradientMode::TwoColor);
	if (RoughnessRow) RoughnessRow->SetGradient(ScalarTrackColor, ScalarTrackColor, EColorBarGradientMode::TwoColor);
}

void UColorPickerPanelWidget::BeginContinuousInteraction()
{
	if (!bIsContinuousInteraction)
	{
		PreviousColor = CurrentColor;
		if (ColorState) ColorState->PreviousLinearColor = PreviousColor;
	}

	bIsContinuousInteraction = true;
}

void UColorPickerPanelWidget::EndContinuousInteraction(bool bCommitColor)
{
	if (bCommitColor) CommitCurrentColor();

	bIsContinuousInteraction = false;
}

void UColorPickerPanelWidget::SetEyedropperActiveVisual(bool bActive)
{
	if (ToolStrip) ToolStrip->SetEyedropperActive(bActive);
}

void UColorPickerPanelWidget::SetEraserActiveVisual(bool bActive)
{
	if (ToolStrip) ToolStrip->SetEraserActive(bActive);
}

bool UColorPickerPanelWidget::IsScalarChannel(FName ChannelId) const
{
	return ChannelId == ChannelBrushSize || ChannelId == ChannelMetallic || ChannelId == ChannelRoughness;
}

bool UColorPickerPanelWidget::GetScalarChannelValue(FName ChannelId, float& OutValue) const
{
	if (ChannelId == ChannelBrushSize)
	{
		OutValue = BrushSize;
		return true;
	}
	if (ChannelId == ChannelMetallic)
	{
		OutValue = Metallic;
		return true;
	}
	if (ChannelId == ChannelRoughness)
	{
		OutValue = Roughness;
		return true;
	}
	return false;
}

bool UColorPickerPanelWidget::SetScalarChannelValue(FName ChannelId, float Value, bool bBroadcast)
{
	if (ChannelId == ChannelBrushSize)
	{
		SetBrushSize(Value, bBroadcast);
		return true;
	}
	if (ChannelId == ChannelMetallic)
	{
		SetMetallic(Value, bBroadcast);
		return true;
	}
	if (ChannelId == ChannelRoughness)
	{
		SetRoughness(Value, bBroadcast);
		return true;
	}
	return false;
}

bool UColorPickerPanelWidget::SetRGBChannelValue(FName ChannelId, float Value)
{
	if (ChannelId != ChannelRed && ChannelId != ChannelGreen && ChannelId != ChannelBlue) return false;

	FLinearColor NewColor = CurrentColor;
	if (ChannelId == ChannelRed) NewColor.R = Value;
	else if (ChannelId == ChannelGreen) NewColor.G = Value;
	else NewColor.B = Value;
	SetCurrentColor(NewColor, true);
	return true;
}

bool UColorPickerPanelWidget::SetHSVChannelValue(FName ChannelId, float Value)
{
	if (ChannelId != ChannelHue && ChannelId != ChannelSaturation && ChannelId != ChannelValue) return false;

	float Hue = 0.0f;
	float Saturation = 0.0f;
	float CurrentValue = 0.0f;
	GetHSV(CurrentColor, Hue, Saturation, CurrentValue);

	if (ChannelId == ChannelHue) Hue = Value;
	else if (ChannelId == ChannelSaturation) Saturation = Value;
	else CurrentValue = Value;

	SetCurrentColor(ColorFromHSV(Hue, Saturation, CurrentValue), true);
	return true;
}

void UColorPickerPanelWidget::HandleWheelHSVChanged(float Hue, float Saturation, float Value)
{
	if (bIsSynchronizing) return;

	SetCurrentColor(ColorFromHSV(Hue, Saturation, Value), true);
}

void UColorPickerPanelWidget::HandleColorInteractionStarted()
{
	BeginContinuousInteraction();
}

void UColorPickerPanelWidget::HandleWheelHSVCommitted(float Hue, float Saturation, float Value)
{
	EndContinuousInteraction(true);
}

void UColorPickerPanelWidget::HandleVerticalSaturationChanged(float Value)
{
	if (bIsSynchronizing) return;

	float Hue = 0.0f;
	float Saturation = 0.0f;
	float CurrentValue = 0.0f;
	GetHSV(CurrentColor, Hue, Saturation, CurrentValue);
	SetCurrentColor(ColorFromHSV(Hue, Value, CurrentValue), true);
}

void UColorPickerPanelWidget::HandleVerticalValueChanged(float Value)
{
	if (bIsSynchronizing) return;

	float Hue = 0.0f;
	float Saturation = 0.0f;
	float CurrentValue = 0.0f;
	GetHSV(CurrentColor, Hue, Saturation, CurrentValue);
	SetCurrentColor(ColorFromHSV(Hue, Saturation, Value), true);
}

void UColorPickerPanelWidget::HandleColorValueCommitted(float Value)
{
	EndContinuousInteraction(true);
}

void UColorPickerPanelWidget::HandleChannelInteractionStarted(FName ChannelId, float InitialValue)
{
	if (IsScalarChannel(ChannelId)) bIsContinuousInteraction = true;
	else BeginContinuousInteraction();
}

void UColorPickerPanelWidget::HandleChannelChanged(FName ChannelId, float Value)
{
	if (bIsSynchronizing) return;

	if (SetScalarChannelValue(ChannelId, Value, true)) return;
	if (SetRGBChannelValue(ChannelId, Value)) return;
	SetHSVChannelValue(ChannelId, Value);
}

void UColorPickerPanelWidget::HandleChannelCommitted(FName ChannelId, float Value)
{
	HandleChannelChanged(ChannelId, Value);

	if (IsScalarChannel(ChannelId))
	{
		CommitScalar(ChannelId);
		bIsContinuousInteraction = false;
	}
	else
	{
		EndContinuousInteraction(true);
	}
}

void UColorPickerPanelWidget::HandleHexCommitted(const FString& HexText, FLinearColor Color, bool bIsValid, bool bHasAlpha)
{
	if (bIsSynchronizing || !bIsValid)
	{
		SynchronizeColorWidgets();
		return;
	}

	FLinearColor NewColor = Color;
	NewColor.A = 1.0f;
	SetCurrentColor(NewColor, true);
	CommitCurrentColor();
}

void UColorPickerPanelWidget::HandleOptionChanged(FName OptionId, bool bIsChecked)
{
	EnsureRuntimeObjects();
	if (OptionId == OptionSRGBPreview && ColorState)
	{
		ColorState->SetSRGBPreview(bIsChecked);
		SynchronizeColorWidgets();
	}
	OnOptionChanged.Broadcast(OptionId, bIsChecked);
}

void UColorPickerPanelWidget::HandlePaletteColorSelected(FLinearColor Color, int32 ColorIndex)
{
	SetCurrentColor(Color, true);
	CommitCurrentColor();
	OnPaletteColorSelected.Broadcast(Color, ColorIndex);
}

void UColorPickerPanelWidget::HandlePaletteChanged(const TArray<FLinearColor>& Colors)
{
	if (HistoryBar) HistoryBar->SetHistoryColors(Colors);
}

void UColorPickerPanelWidget::HandleToolStripButtonClicked(FName ToolId)
{
	if (ToolId == ToolEyedropper)
	{
		ToggleEyedropperMode();
		return;
	}
	else if (ToolId == ToolPalette)
	{
		SetRecentColorsExpanded(!bRecentColorsExpanded);
		OnToolToggled.Broadcast(ToolId, bRecentColorsExpanded);
	}
	else if (ToolId == ToolColorMode)
	{
		ApplyColorModeToWidgets();
		OnToolToggled.Broadcast(ToolId, ToolStrip ? ToolStrip->GetUseSpectrumMode() : false);
	}
	else if (ToolId == ToolEraser)
	{
		if (bIsEyedropperActive) CancelEyedropperMode();
		SetEraserActive(!IsEraserActive(), true);
	}

	OnToolClicked.Broadcast(ToolId);
}

void UColorPickerPanelWidget::HandlePreviousPreviewClicked(FLinearColor Color)
{
	SetCurrentColor(PreviousColor, true);
	CommitCurrentColor();
}

void UColorPickerPanelWidget::HandleEyedropperConfirm()
{
	SampleColorUnderCursor();
}

bool UColorPickerPanelWidget::SampleColorUnderCursor()
{
	FRuntimeMeshPaintSampleResult SampleResult;
	APlayerController* PlayerController = EyedropperPlayerController.Get();
	if (!PlayerController) PlayerController = GetOwningPlayer();
	if (!PlayerController) return false;

	const ERuntimeMeshPaintColorSampleMode SampleMode = BoundPaintingController
		? BoundPaintingController->ColorPickerSampleMode
		: EyedropperSampleMode;
	const bool bSampled = ColorPickerEyedropper::SampleUnderCursor(
		PlayerController, EyedropperTraceChannel, bEyedropperTraceComplex,
		PaintTarget.GetObject(), SampleResult, SampleMode);
	if (!bSampled || !SampleResult.bSuccess) return false;

	SetCurrentColor(SampleResult.Color, true);
	CommitCurrentColor();
	OnColorSampled.Broadcast(SampleResult);

	ApplyBrushSettingsToPaintTarget();

	if (bIsEyedropperActive) CancelEyedropperMode();
	return true;
}

void UColorPickerPanelWidget::HandleEyedropperCancelInput()
{
	CancelEyedropperMode();
}
