// Copyright Shared Orbit 2026. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "ColorPicker/ColorPickerConversionLibrary.h"
#include "Painting/RuntimeMeshPaintTargetInterface.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/PanelSlot.h"
#include "Components/PanelWidget.h"
#include "Components/SlateWrapperTypes.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Widget.h"
#include "Widgets/ColorChannelRowWidget.h"

static const FName ChannelRed(TEXT("R"));
static const FName ChannelGreen(TEXT("G"));
static const FName ChannelBlue(TEXT("B"));
static const FName ChannelHue(TEXT("H"));
static const FName ChannelSaturation(TEXT("S"));
static const FName ChannelValue(TEXT("V"));
static const FName ChannelBrushSize(TEXT("BrushSize"));
static const FName ChannelMetallic(TEXT("Metallic"));
static const FName ChannelRoughness(TEXT("Roughness"));
static const FName ScalarRowsStackWidgetName(TEXT("PaintScalarRowsStack"));
static const FName LegacyScalarRowsStackWidgetName(TEXT("ScalarRowsStack"));
static const FName OptionSRGBPreview(TEXT("sRGBPreview"));
static const FName ToolColorMode(TEXT("ColorMode"));
static const FName ToolEyedropper(TEXT("Eyedropper"));
static const FName ToolPalette(TEXT("Palette"));
static const FName ToolEraser(TEXT("Eraser"));

constexpr float ScalarLabelWidth = 108.0f;
constexpr float ScalarSliderLength = 190.0f;
constexpr float ScalarSliderHeight = 18.0f;
constexpr float ScalarTrackThickness = 6.0f;
constexpr float ScalarSpinBoxWidth = 72.0f;
constexpr float DefaultNormalizedBrushSize = 0.05f;
constexpr float MinNormalizedBrushSize = 0.01f;
constexpr float MaxNormalizedBrushSize = 1.0f;

struct FColorChannelRowConfig
{
	UColorChannelRowWidget* Row;
	FName ChannelId;
	const TCHAR* Label;
	float MinValue;
	float MaxValue;
	int32 Decimals;
	EColorBarGradientMode GradientMode;
};

static FLinearColor ClampColor01(FLinearColor Color)
{
	Color.R = FMath::Clamp(Color.R, 0.0f, 1.0f);
	Color.G = FMath::Clamp(Color.G, 0.0f, 1.0f);
	Color.B = FMath::Clamp(Color.B, 0.0f, 1.0f);
	Color.A = 1.0f;
	return Color;
}

static FLinearColor MakeSRGBPreviewColor(FLinearColor LinearColor)
{
	const FColor SRGBColor = UColorPickerConversionLibrary::LinearRGBToSRGB8(LinearColor);
	return FLinearColor(
		SRGBColor.R / 255.0f,
		SRGBColor.G / 255.0f,
		SRGBColor.B / 255.0f,
		1.0f);
}

static void GetHSV(FLinearColor Color, float& Hue, float& Saturation, float& Value)
{
	float Alpha = 1.0f;
	UColorPickerConversionLibrary::LinearRGBToHSV(Color, Hue, Saturation, Value, Alpha);
}

static FLinearColor ColorFromHSV(float Hue, float Saturation, float Value)
{
	return UColorPickerConversionLibrary::HSVToLinearRGB(Hue, Saturation, Value, 1.0f);
}

static FLinearColor ColorFromRGB(float R, float G, float B)
{
	return ClampColor01(FLinearColor(R, G, B, 1.0f));
}

static bool ImplementsPaintTarget(UObject* Object)
{
	return Object && Object->GetClass()->ImplementsInterface(URuntimeMeshPaintTargetInterface::StaticClass());
}

static void DetachWidgetFromParent(UWidget* Widget)
{
	if (Widget && Widget->Slot && Widget->Slot->Parent) Widget->Slot->Parent->RemoveChild(Widget);
}

static void SetHorizontalSlotPadding(UWidget* Widget, const FMargin& Padding)
{
	if (UHorizontalBoxSlot* HorizontalSlot = Widget ? Cast<UHorizontalBoxSlot>(Widget->Slot) : nullptr) HorizontalSlot->SetPadding(Padding);
}

static void MoveWidgetBefore(UWidget* Widget, UWidget* BeforeWidget, const FMargin& Padding)
{
	if (!Widget || !BeforeWidget || !Widget->Slot || !BeforeWidget->Slot) return;

	UPanelWidget* Parent = Widget->Slot->Parent;
	if (!Parent || Parent != BeforeWidget->Slot->Parent) return;

	const int32 WidgetIndex = Parent->GetChildIndex(Widget);
	const int32 BeforeIndex = Parent->GetChildIndex(BeforeWidget);
	if (WidgetIndex == INDEX_NONE || BeforeIndex == INDEX_NONE || WidgetIndex + 1 == BeforeIndex)
	{
		SetHorizontalSlotPadding(Widget, Padding);
		return;
	}

	Parent->RemoveChild(Widget);
	const int32 InsertIndex = Parent->GetChildIndex(BeforeWidget);
	UPanelSlot* InsertedSlot = Parent->InsertChildAt(InsertIndex == INDEX_NONE ? 0 : InsertIndex, Widget);
	if (UHorizontalBoxSlot* HorizontalSlot = Cast<UHorizontalBoxSlot>(InsertedSlot)) HorizontalSlot->SetPadding(Padding);
}

static void SetVerticalAutoSlot(UWidget* Widget, const FMargin& Padding)
{
	if (UVerticalBoxSlot* VerticalSlot = Widget ? Cast<UVerticalBoxSlot>(Widget->Slot) : nullptr)
	{
		FSlateChildSize AutoSize;
		AutoSize.SizeRule = ESlateSizeRule::Automatic;
		VerticalSlot->SetSize(AutoSize);
		VerticalSlot->SetHorizontalAlignment(HAlign_Fill);
		VerticalSlot->SetVerticalAlignment(VAlign_Center);
		VerticalSlot->SetPadding(Padding);
	}
}
