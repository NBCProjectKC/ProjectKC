// Copyright Shared Orbit 2026. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateTypes.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SCompoundWidget.h"

class SSlider;

template<typename NumericType>
class SSpinBox;

class SRuntimeMeshPaintColorSlider : public SCompoundWidget
{
public:
	DECLARE_DELEGATE_OneParam(FOnValueChanged, float);
	DECLARE_DELEGATE_OneParam(FOnValueCommitted, float);

	SLATE_BEGIN_ARGS(SRuntimeMeshPaintColorSlider)
		: _Value(0.0f)
		, _MinSpinBoxValue(0.0f)
		, _MaxSpinBoxValue(1.0f)
		, _MinSliderValue(0.0f)
		, _MaxSliderValue(1.0f)
		, _Delta(0.01f)
		, _SupportDynamicSliderMaxValue(true)
		, _Orientation(Orient_Horizontal)
		, _Label()
		, _GradientColors()
		, _HasAlphaBackground(false)
		, _UseSRGB(true)
		, _ShowLabelAndSpinBox(true)
		, _FractionalDigits(3)
		, _HorizontalLabelWidth(8.0f)
		, _HorizontalSliderLength(123.0f)
		, _HorizontalSliderHeight(20.0f)
		, _HorizontalTrackThickness(0.0f)
		, _HorizontalSpinBoxWidth(60.0f)
		, _VerticalSliderWidth(28.0f)
		, _VerticalSliderHeight(200.0f)
	{ }

		SLATE_ATTRIBUTE(float, Value)
		SLATE_ATTRIBUTE(float, MinSpinBoxValue)
		SLATE_ATTRIBUTE(float, MaxSpinBoxValue)
		SLATE_ATTRIBUTE(float, MinSliderValue)
		SLATE_ATTRIBUTE(float, MaxSliderValue)
		SLATE_ATTRIBUTE(float, Delta)
		SLATE_ATTRIBUTE(bool, SupportDynamicSliderMaxValue)
		SLATE_ATTRIBUTE(EOrientation, Orientation)
		SLATE_ATTRIBUTE(FText, Label)
		SLATE_ATTRIBUTE(TArray<FLinearColor>, GradientColors)
		SLATE_ATTRIBUTE(bool, HasAlphaBackground)
		SLATE_ATTRIBUTE(bool, UseSRGB)

		SLATE_ARGUMENT(bool, ShowLabelAndSpinBox)
		SLATE_ARGUMENT(int32, FractionalDigits)
		SLATE_ARGUMENT(float, HorizontalLabelWidth)
		SLATE_ARGUMENT(float, HorizontalSliderLength)
		SLATE_ARGUMENT(float, HorizontalSliderHeight)
		SLATE_ARGUMENT(float, HorizontalTrackThickness)
		SLATE_ARGUMENT(float, HorizontalSpinBoxWidth)
		SLATE_ARGUMENT(float, VerticalSliderWidth)
		SLATE_ARGUMENT(float, VerticalSliderHeight)

		SLATE_EVENT(FOnValueChanged, OnValueChanged)
		SLATE_EVENT(FSimpleDelegate, OnBeginSliderMovement)
		SLATE_EVENT(FOnValueCommitted, OnEndSliderMovement)
		SLATE_EVENT(FSimpleDelegate, OnBeginSpinBoxMovement)
		SLATE_EVENT(FOnValueCommitted, OnEndSpinBoxMovement)
		SLATE_EVENT(FOnValueCommitted, OnValueCommitted)

	SLATE_END_ARGS()

	SRuntimeMeshPaintColorSlider();

	void Construct(const FArguments& InArgs);
	void SetSliderRange(float NewMinValue, float NewMaxValue);
	void SetFractionalDigits(int32 NewFractionalDigits);
	void SetHorizontalLayout(
		float NewLabelWidth, float NewSliderLength, float NewSliderHeight,
		float NewTrackThickness, float NewSpinBoxWidth);

	virtual int32 OnPaint(
		const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId,
		const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float) const override;

private:
	void OnSliderCaptureEnd();
	void OnSpinBoxValueCommitted(float NewValue, ETextCommit::Type CommitType);
	float GetSliderValue() const;
	float GetSliderLength(const FVector2f& SliderSize) const;

	TSlateAttribute<EOrientation, EInvalidateWidgetReason::Paint> Orientation;
	TSlateAttribute<TArray<FLinearColor>, EInvalidateWidgetReason::Paint> GradientColors;
	TSlateAttribute<bool, EInvalidateWidgetReason::Paint> bHasAlphaBackground;
	TSlateAttribute<bool, EInvalidateWidgetReason::Paint> bUseSRGB;
	TSlateAttribute<bool, EInvalidateWidgetReason::Paint> bSupportDynamicSliderMaxValue;

	TSharedPtr<SSlider> Slider;
	TSharedPtr<SSpinBox<float>> SpinBox;
	TSharedPtr<SHorizontalBox> ColorWidget;

	bool bShowLabelAndSpinBox;
	int32 FractionalDigits;
	float HorizontalLabelWidth;
	float HorizontalSliderLength;
	float HorizontalSliderHeight;
	float HorizontalTrackThickness;
	float HorizontalSpinBoxWidth;
	float VerticalSliderWidth;
	float VerticalSliderHeight;

	FOnValueCommitted OnEndSliderMovement;
	FOnValueCommitted OnEndSpinBoxMovement;
	FOnValueCommitted OnValueCommitted;

	const FSlateBrush* BorderBrush;
	const FSlateBrush* BorderActiveBrush;
	const FSlateBrush* BorderHoveredBrush;
	const FSlateBrush* AlphaBackgroundBrush;

	static constexpr float Padding = 8.0f;
};
