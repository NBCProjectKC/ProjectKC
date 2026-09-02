// Copyright Shared Orbit 2026. All Rights Reserved.
#include "Widgets/HorizontalColorBarWidget.h"

#include "Widgets/RuntimeColorSlider.h"

namespace
{
	float SafeRange(float MinValue, float MaxValue)
	{
		return FMath::Max(KINDA_SMALL_NUMBER, MaxValue - MinValue);
	}

	TArray<FLinearColor> MakeHorizontalHueColors()
	{
		return
		{
			FLinearColor::Red,
			FLinearColor::Yellow,
			FLinearColor::Green,
			FLinearColor(0.0f, 1.0f, 1.0f, 1.0f),
			FLinearColor::Blue,
			FLinearColor(1.0f, 0.0f, 1.0f, 1.0f),
			FLinearColor::Red
		};
	}
}

UHorizontalColorBarWidget::UHorizontalColorBarWidget()
	: Value(0.0f)
	, MinValue(0.0f)
	, MaxValue(1.0f)
	, StartColor(FLinearColor::Black)
	, EndColor(FLinearColor::White)
	, GradientMode(EColorBarGradientMode::TwoColor)
	, DesiredBarSize(156.0f, 18.0f)
{
	SetVisibility(ESlateVisibility::Visible);
}

void UHorizontalColorBarWidget::SetValue(float NewValue, bool bBroadcast)
{
	const float Lower = FMath::Min(MinValue, MaxValue);
	const float Upper = FMath::Max(MinValue, MaxValue);
	Value = FMath::Clamp(NewValue, Lower, Upper);

	if (MyColorBar.IsValid()) MyColorBar->Invalidate(EInvalidateWidgetReason::Paint);

	if (bBroadcast) OnValueChanged.Broadcast(Value);
}

void UHorizontalColorBarWidget::SetRange(float NewMinValue, float NewMaxValue)
{
	MinValue = NewMinValue;
	MaxValue = FMath::IsNearlyEqual(NewMinValue, NewMaxValue) ? NewMinValue + 1.0f : NewMaxValue;
	SetValue(Value, false);

	if (MyColorBar.IsValid()) MyColorBar->SetSliderRange(MinValue, MaxValue);
}

void UHorizontalColorBarWidget::SetGradientColors(FLinearColor NewStartColor, FLinearColor NewEndColor)
{
	StartColor = NewStartColor;
	EndColor = NewEndColor;

	if (MyColorBar.IsValid()) MyColorBar->Invalidate(EInvalidateWidgetReason::Paint);
}

void UHorizontalColorBarWidget::SetGradientMode(EColorBarGradientMode NewGradientMode)
{
	GradientMode = NewGradientMode;

	if (MyColorBar.IsValid()) MyColorBar->Invalidate(EInvalidateWidgetReason::Paint);
}

float UHorizontalColorBarWidget::GetNormalizedValue() const
{
	return FMath::Clamp((Value - MinValue) / SafeRange(MinValue, MaxValue), 0.0f, 1.0f);
}

TArray<FLinearColor> UHorizontalColorBarWidget::GetGradientColors() const
{
	if (GradientMode == EColorBarGradientMode::Hue) return MakeHorizontalHueColors();

	return { StartColor, EndColor };
}

bool UHorizontalColorBarWidget::HasAlphaBackground() const
{
	return GradientMode == EColorBarGradientMode::Alpha;
}

void UHorizontalColorBarWidget::HandleSlateValueChanged(float NewValue)
{
	SetValue(NewValue, false);
	OnValueChanged.Broadcast(Value);
}

void UHorizontalColorBarWidget::HandleSlateInteractionStarted()
{
	OnInteractionStarted.Broadcast();
}

void UHorizontalColorBarWidget::HandleSlateInteractionFinished(float NewValue)
{
	SetValue(NewValue, false);
	OnValueCommitted.Broadcast(Value);
}

TSharedRef<SWidget> UHorizontalColorBarWidget::RebuildWidget()
{
	MyColorBar = SNew(SRuntimeMeshPaintColorSlider)
		.Value_Lambda([this]() { return GetValue(); })
		.MinSliderValue(MinValue)
		.MaxSliderValue(MaxValue)
		.MinSpinBoxValue(MinValue)
		.MaxSpinBoxValue(MaxValue)
		.Delta(0.001f)
		.SupportDynamicSliderMaxValue(false)
		.Orientation(Orient_Horizontal)
		.GradientColors_Lambda([this]() { return GetGradientColors(); })
		.HasAlphaBackground_Lambda([this]() { return HasAlphaBackground(); })
		.UseSRGB(true)
		.ShowLabelAndSpinBox(false)
		.HorizontalSliderLength(DesiredBarSize.X)
		.HorizontalSliderHeight(DesiredBarSize.Y)
		.OnBeginSliderMovement(BIND_UOBJECT_DELEGATE(FSimpleDelegate, HandleSlateInteractionStarted))
		.OnValueChanged(SRuntimeMeshPaintColorSlider::FOnValueChanged::CreateUObject(this, &UHorizontalColorBarWidget::HandleSlateValueChanged))
		.OnEndSliderMovement(SRuntimeMeshPaintColorSlider::FOnValueCommitted::CreateUObject(this, &UHorizontalColorBarWidget::HandleSlateInteractionFinished));

	return MyColorBar.ToSharedRef();
}

void UHorizontalColorBarWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyColorBar.Reset();
}

void UHorizontalColorBarWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	SetValue(Value, false);
	SetRange(MinValue, MaxValue);
}

#if WITH_EDITOR
const FText UHorizontalColorBarWidget::GetPaletteCategory()
{
	return NSLOCTEXT("MeshPaintingCore", "ColorPickerPaletteCategory", "Color Picker");
}
#endif
