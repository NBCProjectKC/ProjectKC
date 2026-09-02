// Copyright Shared Orbit 2026. All Rights Reserved.
#include "Widgets/ColorWheelWidget.h"

#include "Styling/CoreStyle.h"
#include "Widgets/Colors/SColorSpectrum.h"
#include "Widgets/Colors/SColorWheel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/SOverlay.h"

namespace
{
	float NormalizeHue(float InHue)
	{
		float Result = FMath::Fmod(InHue, 360.0f);
		if (Result < 0.0f) Result += 360.0f;
		return Result;
	}
}

UColorWheelWidget::UColorWheelWidget()
	: Hue(0.0f)
	, Saturation(1.0f)
	, Value(1.0f)
	, DesiredWheelSize(160.0f, 160.0f)
	, bUseSpectrumMode(false)
{
	SetVisibility(ESlateVisibility::Visible);
}

void UColorWheelWidget::SetHueSaturation(float NewHue, float NewSaturation, bool bBroadcast)
{
	Hue = NormalizeHue(NewHue);
	Saturation = FMath::Clamp(NewSaturation, 0.0f, 1.0f);

	InvalidateSlateWidgets();

	if (bBroadcast)
	{
		OnHueSaturationChanged.Broadcast(Hue, Saturation);
		OnHSVChanged.Broadcast(Hue, Saturation, Value);
	}
}

void UColorWheelWidget::SetValueLevel(float NewValue)
{
	Value = FMath::Clamp(NewValue, 0.0f, 1.0f);
	InvalidateSlateWidgets();
}

void UColorWheelWidget::SetUseSpectrumMode(bool bNewUseSpectrumMode)
{
	if (bUseSpectrumMode == bNewUseSpectrumMode) return;

	bUseSpectrumMode = bNewUseSpectrumMode;
	if (MyColorWheel.IsValid()) MyColorWheel->Invalidate(EInvalidateWidgetReason::Visibility);
	if (MyColorSpectrum.IsValid()) MyColorSpectrum->Invalidate(EInvalidateWidgetReason::Visibility);
}

FLinearColor UColorWheelWidget::GetSelectedColorHSV() const
{
	return FLinearColor(Hue, Saturation, Value, 1.0f);
}

void UColorWheelWidget::HandleSlateValueChanged(FLinearColor NewHSVColor)
{
	if (!bUseSpectrumMode && FMath::IsNearlyZero(NewHSVColor.B)) NewHSVColor.B = 1.0f;

	Hue = NormalizeHue(NewHSVColor.R);
	Saturation = FMath::Clamp(NewHSVColor.G, 0.0f, 1.0f);
	Value = FMath::Clamp(NewHSVColor.B, 0.0f, 1.0f);
	InvalidateSlateWidgets();
	OnHueSaturationChanged.Broadcast(Hue, Saturation);
	OnHSVChanged.Broadcast(Hue, Saturation, Value);
}

void UColorWheelWidget::HandleSlateInteractionStarted()
{
	OnInteractionStarted.Broadcast();
}

void UColorWheelWidget::HandleSlateInteractionFinished()
{
	OnHueSaturationCommitted.Broadcast(Hue, Saturation);
	OnHSVCommitted.Broadcast(Hue, Saturation, Value);
}

TSharedRef<SWidget> UColorWheelWidget::RebuildWidget()
{
	const float WheelSide = FMath::Max(1.0f, FMath::Min(DesiredWheelSize.X, DesiredWheelSize.Y));

	MyColorWheel = SNew(SColorWheel)
		.SelectedColor_Lambda([this]() { return GetSelectedColorHSV(); })
		.ColorWheelBrush(FCoreStyle::Get().GetBrush("ColorWheel.HueValueCircle"))
		.Visibility_UObject(this, &UColorWheelWidget::GetWheelVisibility)
		.OnMouseCaptureBegin(BIND_UOBJECT_DELEGATE(FSimpleDelegate, HandleSlateInteractionStarted))
		.OnMouseCaptureEnd(BIND_UOBJECT_DELEGATE(FSimpleDelegate, HandleSlateInteractionFinished))
		.OnValueChanged(BIND_UOBJECT_DELEGATE(FOnLinearColorValueChanged, HandleSlateValueChanged));

	MyColorSpectrum = SNew(SColorSpectrum)
		.SelectedColor_Lambda([this]() { return GetSelectedColorHSV(); })
		.Visibility_UObject(this, &UColorWheelWidget::GetSpectrumVisibility)
		.OnMouseCaptureBegin(BIND_UOBJECT_DELEGATE(FSimpleDelegate, HandleSlateInteractionStarted))
		.OnMouseCaptureEnd(BIND_UOBJECT_DELEGATE(FSimpleDelegate, HandleSlateInteractionFinished))
		.OnValueChanged(BIND_UOBJECT_DELEGATE(FOnLinearColorValueChanged, HandleSlateValueChanged));

	return SNew(SBox)
		.WidthOverride(WheelSide)
		.HeightOverride(WheelSide)
		[
			SNew(SScaleBox)
			.Stretch(EStretch::ScaleToFit)
			.StretchDirection(EStretchDirection::Both)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SBox)
				.WidthOverride(WheelSide)
				.HeightOverride(WheelSide)
				[
					SNew(SOverlay)

					+ SOverlay::Slot()
					[
						MyColorWheel.ToSharedRef()
					]

					+ SOverlay::Slot()
					[
						MyColorSpectrum.ToSharedRef()
					]
				]
			]
		];
}

void UColorWheelWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyColorWheel.Reset();
	MyColorSpectrum.Reset();
}

void UColorWheelWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	SetHueSaturation(Hue, Saturation, false);
	SetValueLevel(Value);
	SetUseSpectrumMode(bUseSpectrumMode);
}

void UColorWheelWidget::InvalidateSlateWidgets()
{
	if (MyColorWheel.IsValid()) MyColorWheel->Invalidate(EInvalidateWidgetReason::Paint);
	if (MyColorSpectrum.IsValid()) MyColorSpectrum->Invalidate(EInvalidateWidgetReason::Paint);
}

EVisibility UColorWheelWidget::GetWheelVisibility() const
{
	return bUseSpectrumMode ? EVisibility::Hidden : EVisibility::Visible;
}

EVisibility UColorWheelWidget::GetSpectrumVisibility() const
{
	return bUseSpectrumMode ? EVisibility::Visible : EVisibility::Hidden;
}

#if WITH_EDITOR
const FText UColorWheelWidget::GetPaletteCategory()
{
	return NSLOCTEXT("MeshPaintingCore", "ColorPickerPaletteCategory", "Color Picker");
}
#endif
