// Copyright Shared Orbit 2026. All Rights Reserved.
#include "Widgets/ColorChannelRowWidget.h"

#include "Widgets/RuntimeColorSlider.h"

namespace
{
	TArray<FLinearColor> MakeHueColors()
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

UColorChannelRowWidget::UColorChannelRowWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, ChannelId(NAME_None)
	, Label(FText::FromString(TEXT("R")))
	, Value(0.0f)
	, MinValue(0.0f)
	, MaxValue(1.0f)
	, DecimalPlaces(3)
	, GradientStartColor(FLinearColor::Black)
	, GradientEndColor(FLinearColor::White)
	, GradientMode(EColorBarGradientMode::TwoColor)
	, HorizontalLabelWidth(8.0f)
	, HorizontalSliderLength(123.0f)
	, HorizontalSliderHeight(20.0f)
	, HorizontalTrackThickness(0.0f)
	, HorizontalSpinBoxWidth(60.0f)
	, bIsSynchronizing(false)
{
}

void UColorChannelRowWidget::SetValue(float NewValue, bool bBroadcast)
{
	const float ClampedValue = ClampToRange(NewValue);
	if (FMath::IsNearlyEqual(Value, ClampedValue) && !bBroadcast)
	{
		SynchronizeSlateWidget();
		return;
	}

	Value = ClampedValue;
	SynchronizeSlateWidget();

	if (bBroadcast) OnValueChanged.Broadcast(ChannelId, Value);
}

void UColorChannelRowWidget::SetRange(float NewMinValue, float NewMaxValue)
{
	MinValue = NewMinValue;
	MaxValue = FMath::IsNearlyEqual(NewMinValue, NewMaxValue) ? NewMinValue + 1.0f : NewMaxValue;
	SetValue(Value, false);
	SynchronizeSlateWidget();
}

void UColorChannelRowWidget::SetLabel(FText NewLabel)
{
	Label = NewLabel;
	SynchronizeSlateWidget();
}

void UColorChannelRowWidget::SetDecimalPlaces(int32 NewDecimalPlaces)
{
	DecimalPlaces = FMath::Clamp(NewDecimalPlaces, 0, 6);
	SynchronizeSlateWidget();
}

void UColorChannelRowWidget::SetGradient(FLinearColor NewStartColor, FLinearColor NewEndColor, EColorBarGradientMode NewGradientMode)
{
	GradientStartColor = NewStartColor;
	GradientEndColor = NewEndColor;
	GradientMode = NewGradientMode;
	SynchronizeSlateWidget();
}

void UColorChannelRowWidget::SetHorizontalLayout(
	float NewLabelWidth, float NewSliderLength, float NewSliderHeight,
	float NewTrackThickness, float NewSpinBoxWidth)
{
	HorizontalLabelWidth = FMath::Max(1.0f, NewLabelWidth);
	HorizontalSliderLength = FMath::Max(1.0f, NewSliderLength);
	HorizontalSliderHeight = FMath::Max(1.0f, NewSliderHeight);
	HorizontalTrackThickness = FMath::Max(0.0f, NewTrackThickness);
	HorizontalSpinBoxWidth = FMath::Max(1.0f, NewSpinBoxWidth);
	SynchronizeSlateWidget();
}

TSharedRef<SWidget> UColorChannelRowWidget::RebuildWidget()
{
	MyColorSlider = SNew(SRuntimeMeshPaintColorSlider)
		.Value_Lambda([this]() { return GetValue(); })
		.MinSpinBoxValue_Lambda([this]() { return GetMinValue(); })
		.MaxSpinBoxValue_Lambda([this]() { return GetMaxValue(); })
		.MinSliderValue(MinValue)
		.MaxSliderValue(MaxValue)
		.Delta_Lambda([this]() { return GetDelta(); })
		.SupportDynamicSliderMaxValue_Lambda([this]() { return SupportsDynamicSliderMaxValue(); })
		.Orientation(Orient_Horizontal)
		.Label_Lambda([this]() { return GetSliderLabel(); })
		.GradientColors_Lambda([this]() { return GetGradientColors(); })
		.HasAlphaBackground_Lambda([this]() { return HasAlphaBackground(); })
		.UseSRGB(true)
		.ShowLabelAndSpinBox(true)
		.FractionalDigits(DecimalPlaces)
		.HorizontalLabelWidth(HorizontalLabelWidth)
		.HorizontalSliderLength(HorizontalSliderLength)
		.HorizontalSliderHeight(HorizontalSliderHeight)
		.HorizontalTrackThickness(HorizontalTrackThickness)
		.HorizontalSpinBoxWidth(HorizontalSpinBoxWidth)
		.OnBeginSliderMovement(BIND_UOBJECT_DELEGATE(FSimpleDelegate, HandleSliderInteractionStarted))
		.OnBeginSpinBoxMovement(BIND_UOBJECT_DELEGATE(FSimpleDelegate, HandleSliderInteractionStarted))
		.OnValueChanged(SRuntimeMeshPaintColorSlider::FOnValueChanged::CreateUObject(this, &UColorChannelRowWidget::HandleSliderValueChanged))
		.OnEndSliderMovement(SRuntimeMeshPaintColorSlider::FOnValueCommitted::CreateUObject(this, &UColorChannelRowWidget::HandleSliderValueCommitted))
		.OnEndSpinBoxMovement(SRuntimeMeshPaintColorSlider::FOnValueCommitted::CreateUObject(this, &UColorChannelRowWidget::HandleSliderValueCommitted));

	return MyColorSlider.ToSharedRef();
}

void UColorChannelRowWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyColorSlider.Reset();
	NativeSliderHost = nullptr;
}

void UColorChannelRowWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	SynchronizeSlateWidget();
}

void UColorChannelRowWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	SynchronizeSlateWidget();
}

void UColorChannelRowWidget::SynchronizeSlateWidget()
{
	TGuardValue<bool> Guard(bIsSynchronizing, true);

	if (MyColorSlider.IsValid())
	{
		MyColorSlider->SetSliderRange(MinValue, MaxValue);
		MyColorSlider->SetFractionalDigits(DecimalPlaces);
		MyColorSlider->SetHorizontalLayout(
			HorizontalLabelWidth, HorizontalSliderLength, HorizontalSliderHeight,
			HorizontalTrackThickness, HorizontalSpinBoxWidth);
		MyColorSlider->Invalidate(EInvalidateWidgetReason::Paint);
	}
}

float UColorChannelRowWidget::ClampToRange(float InValue) const
{
	const float Lower = FMath::Min(MinValue, MaxValue);
	const float Upper = FMath::Max(MinValue, MaxValue);
	return FMath::Clamp(InValue, Lower, Upper);
}

float UColorChannelRowWidget::GetDelta() const
{
	return GradientMode == EColorBarGradientMode::Hue ? 1.0f : 0.001f;
}

TArray<FLinearColor> UColorChannelRowWidget::GetGradientColors() const
{
	if (GradientMode == EColorBarGradientMode::Hue) return MakeHueColors();

	return { GradientStartColor, GradientEndColor };
}

bool UColorChannelRowWidget::HasAlphaBackground() const
{
	return GradientMode == EColorBarGradientMode::Alpha;
}

bool UColorChannelRowWidget::SupportsDynamicSliderMaxValue() const
{
	return GradientMode != EColorBarGradientMode::Hue;
}

void UColorChannelRowWidget::HandleSliderValueChanged(float NewValue)
{
	if (bIsSynchronizing) return;

	Value = ClampToRange(NewValue);
	SynchronizeSlateWidget();
	OnValueChanged.Broadcast(ChannelId, Value);
}

void UColorChannelRowWidget::HandleSliderInteractionStarted()
{
	if (bIsSynchronizing) return;

	OnInteractionStarted.Broadcast(ChannelId, Value);
}

void UColorChannelRowWidget::HandleSliderValueCommitted(float NewValue)
{
	if (bIsSynchronizing) return;

	Value = ClampToRange(NewValue);
	SynchronizeSlateWidget();
	OnValueCommitted.Broadcast(ChannelId, Value);
}
