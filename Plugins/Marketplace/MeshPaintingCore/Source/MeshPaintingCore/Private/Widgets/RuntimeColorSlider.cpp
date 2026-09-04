// Copyright Shared Orbit 2026. All Rights Reserved.
#include "Widgets/RuntimeColorSlider.h"

#include "Algo/Reverse.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Rendering/DrawElements.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

SRuntimeMeshPaintColorSlider::SRuntimeMeshPaintColorSlider()
	: Orientation(*this, EOrientation::Orient_Horizontal)
	, GradientColors(*this)
	, bHasAlphaBackground(*this, false)
	, bUseSRGB(*this, true)
	, bSupportDynamicSliderMaxValue(*this, true)
	, bShowLabelAndSpinBox(true)
	, FractionalDigits(3)
	, HorizontalLabelWidth(8.0f)
	, HorizontalSliderLength(123.0f)
	, HorizontalSliderHeight(20.0f)
	, HorizontalTrackThickness(0.0f)
	, HorizontalSpinBoxWidth(60.0f)
	, VerticalSliderWidth(28.0f)
	, VerticalSliderHeight(200.0f)
	, BorderBrush(nullptr)
	, BorderActiveBrush(nullptr)
	, BorderHoveredBrush(nullptr)
	, AlphaBackgroundBrush(nullptr)
{
}

void SRuntimeMeshPaintColorSlider::Construct(const FArguments& InArgs)
{
	Orientation.Assign(*this, InArgs._Orientation);
	GradientColors.Assign(*this, InArgs._GradientColors);
	bHasAlphaBackground.Assign(*this, InArgs._HasAlphaBackground);
	bUseSRGB.Assign(*this, InArgs._UseSRGB);
	bSupportDynamicSliderMaxValue.Assign(*this, InArgs._SupportDynamicSliderMaxValue);

	bShowLabelAndSpinBox = InArgs._ShowLabelAndSpinBox;
	FractionalDigits = FMath::Clamp(InArgs._FractionalDigits, 0, 6);
	HorizontalLabelWidth = FMath::Max(1.0f, InArgs._HorizontalLabelWidth);
	HorizontalSliderLength = FMath::Max(1.0f, InArgs._HorizontalSliderLength);
	HorizontalSliderHeight = FMath::Max(1.0f, InArgs._HorizontalSliderHeight);
	HorizontalTrackThickness = FMath::Max(0.0f, InArgs._HorizontalTrackThickness);
	HorizontalSpinBoxWidth = FMath::Max(1.0f, InArgs._HorizontalSpinBoxWidth);
	VerticalSliderWidth = FMath::Max(1.0f, InArgs._VerticalSliderWidth);
	VerticalSliderHeight = FMath::Max(1.0f, InArgs._VerticalSliderHeight);

	OnEndSliderMovement = InArgs._OnEndSliderMovement;
	OnEndSpinBoxMovement = InArgs._OnEndSpinBoxMovement;
	OnValueCommitted = InArgs._OnValueCommitted;

	BorderBrush = FAppStyle::Get().GetBrush("ColorPicker.RoundedInputBorder");
	BorderActiveBrush = FAppStyle::Get().GetBrush("ColorPicker.RoundedInputBorderActive");
	BorderHoveredBrush = FAppStyle::Get().GetBrush("ColorPicker.RoundedInputBorderHovered");
	AlphaBackgroundBrush = FAppStyle::Get().GetBrush("ColorPicker.RoundedAlphaBackground");

	const FSlateFontInfo SmallFont = FAppStyle::Get().GetFontStyle("ColorPicker.SmallFont");
	const EOrientation SliderOrientation = Orientation.Get();

	SAssignNew(Slider, SSlider)
		.IndentHandle(false)
		.Orientation(SliderOrientation)
		.SliderBarColor(FLinearColor::Transparent)
		.SliderHandleColor(FLinearColor::Transparent)
		.Style(&FAppStyle::Get().GetWidgetStyle<FSliderStyle>("ColorPicker.Slider"))
		.MinValue(InArgs._MinSliderValue.Get())
		.MaxValue(InArgs._MaxSliderValue.Get())
		.StepSize(InArgs._Delta)
		.Value(InArgs._Value)
		.OnMouseCaptureBegin(InArgs._OnBeginSliderMovement)
		.OnMouseCaptureEnd(this, &SRuntimeMeshPaintColorSlider::OnSliderCaptureEnd)
		.OnValueChanged(InArgs._OnValueChanged);

	SAssignNew(ColorWidget, SHorizontalBox);

	if (SliderOrientation == Orient_Horizontal && bShowLabelAndSpinBox)
	{
		ColorWidget->AddSlot()
			.MinWidth(HorizontalLabelWidth)
			.MaxWidth(HorizontalLabelWidth)
			.Padding(0.0f, 0.0f, Padding, 0.0f)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(InArgs._Label)
					.TextStyle(&FAppStyle::Get().GetWidgetStyle<FTextBlockStyle>("SmallText"))
					.Justification(ETextJustify::Left)
				]
			];

		ColorWidget->AddSlot()
			.MinWidth(HorizontalSliderLength)
			.MaxWidth(HorizontalSliderLength)
			[
				Slider.ToSharedRef()
			];

		ColorWidget->AddSlot()
			.MinWidth(HorizontalSpinBoxWidth)
			.MaxWidth(HorizontalSpinBoxWidth)
			.Padding(Padding, 0.0f, 0.0f, 0.0f)
			[
				SAssignNew(SpinBox, SSpinBox<float>)
				.Style(FAppStyle::Get(), "ColorSlider.SpinBox")
				.MinValue(InArgs._MinSpinBoxValue.Get())
				.MaxValue(InArgs._MaxSpinBoxValue.Get())
				.MinSliderValue(InArgs._MinSliderValue.Get())
				.MaxSliderValue(InArgs._MaxSliderValue.Get())
				.MinFractionalDigits(FractionalDigits)
				.MaxFractionalDigits(FractionalDigits)
				.Delta(InArgs._Delta)
				.Value(InArgs._Value)
				.SupportDynamicSliderMaxValue(InArgs._SupportDynamicSliderMaxValue)
				.OnBeginSliderMovement(InArgs._OnBeginSpinBoxMovement)
				.OnEndSliderMovement(InArgs._OnEndSpinBoxMovement)
				.OnValueChanged(InArgs._OnValueChanged)
				.OnValueCommitted(this, &SRuntimeMeshPaintColorSlider::OnSpinBoxValueCommitted)
				.Font(SmallFont)
			];
	}
	else
	{
		ColorWidget->AddSlot()
			[
				Slider.ToSharedRef()
			];
	}

	ChildSlot
	[
		ColorWidget.ToSharedRef()
	];
}

void SRuntimeMeshPaintColorSlider::SetSliderRange(float NewMinValue, float NewMaxValue)
{
	if (Slider.IsValid()) Slider->SetMinAndMaxValues(NewMinValue, NewMaxValue);

	if (SpinBox.IsValid())
	{
		SpinBox->SetMinValue(NewMinValue);
		SpinBox->SetMaxValue(NewMaxValue);
		SpinBox->SetMinSliderValue(NewMinValue);
		SpinBox->SetMaxSliderValue(NewMaxValue);
	}
}

void SRuntimeMeshPaintColorSlider::SetFractionalDigits(int32 NewFractionalDigits)
{
	FractionalDigits = FMath::Clamp(NewFractionalDigits, 0, 6);

	if (SpinBox.IsValid())
	{
		SpinBox->SetMinFractionalDigits(FractionalDigits);
		SpinBox->SetMaxFractionalDigits(FractionalDigits);
	}
}

void SRuntimeMeshPaintColorSlider::SetHorizontalLayout(
	float NewLabelWidth, float NewSliderLength, float NewSliderHeight,
	float NewTrackThickness, float NewSpinBoxWidth)
{
	HorizontalLabelWidth = FMath::Max(1.0f, NewLabelWidth);
	HorizontalSliderLength = FMath::Max(1.0f, NewSliderLength);
	HorizontalSliderHeight = FMath::Max(1.0f, NewSliderHeight);
	HorizontalTrackThickness = FMath::Max(0.0f, NewTrackThickness);
	HorizontalSpinBoxWidth = FMath::Max(1.0f, NewSpinBoxWidth);

	if (ColorWidget.IsValid() && Orientation.Get() == Orient_Horizontal && bShowLabelAndSpinBox && ColorWidget->NumSlots() >= 3)
	{
		SHorizontalBox::FSlot& LabelSlot = ColorWidget->GetSlot(0);
		LabelSlot.SetMinWidth(HorizontalLabelWidth);
		LabelSlot.SetMaxWidth(HorizontalLabelWidth);

		SHorizontalBox::FSlot& SliderSlot = ColorWidget->GetSlot(1);
		SliderSlot.SetMinWidth(HorizontalSliderLength);
		SliderSlot.SetMaxWidth(HorizontalSliderLength);

		SHorizontalBox::FSlot& SpinBoxSlot = ColorWidget->GetSlot(2);
		SpinBoxSlot.SetMinWidth(HorizontalSpinBoxWidth);
		SpinBoxSlot.SetMaxWidth(HorizontalSpinBoxWidth);
	}

	Invalidate(EInvalidateWidgetReason::LayoutAndVolatility);
}

FVector2D SRuntimeMeshPaintColorSlider::ComputeDesiredSize(float) const
{
	if (Orientation.Get() == EOrientation::Orient_Horizontal)
	{
		const float TotalWidth = bShowLabelAndSpinBox
			? HorizontalLabelWidth + Padding + HorizontalSliderLength + Padding + HorizontalSpinBoxWidth
			: HorizontalSliderLength;
		return FVector2D(TotalWidth, HorizontalSliderHeight);
	}

	return FVector2D(VerticalSliderWidth, VerticalSliderHeight);
}

int32 SRuntimeMeshPaintColorSlider::OnPaint(
	const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId,
	const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const int32 MaxChildLayer = SCompoundWidget::OnPaint(
		Args, AllottedGeometry, MyCullingRect, OutDrawElements,
		LayerId, InWidgetStyle, bParentEnabled);
	LayerId = MaxChildLayer + 1;

	if (!Slider.IsValid()) return LayerId;

	const ESlateDrawEffect DrawEffects = ESlateDrawEffect::None;
	const FVector2f AllottedGeometrySize = AllottedGeometry.GetLocalSize();
	FVector2f SliderOffset = FVector2f::ZeroVector;
	FVector2f SliderSize = AllottedGeometrySize;
	FVector2f TrackOffset = FVector2f::ZeroVector;
	FVector2f TrackSize = AllottedGeometrySize;

	if (Orientation.Get() == Orient_Horizontal && bShowLabelAndSpinBox)
	{
		SliderOffset.X = HorizontalLabelWidth + Padding;
		SliderSize = FVector2f(HorizontalSliderLength, AllottedGeometrySize.Y);
	}
	else if (Orientation.Get() == Orient_Vertical) SliderSize = FVector2f(AllottedGeometrySize.X, AllottedGeometrySize.Y);

	TrackOffset = SliderOffset;
	TrackSize = SliderSize;
	if (Orientation.Get() == Orient_Horizontal && HorizontalTrackThickness > 0.0f)
	{
		TrackSize.Y = FMath::Min(TrackSize.Y, HorizontalTrackThickness);
		TrackOffset.Y += (SliderSize.Y - TrackSize.Y) * 0.5f;
	}

	const int32 NumColors = GradientColors.Get().Num();
	const float SliderLength = GetSliderLength(TrackSize);

	if (NumColors > 1)
	{
		if (bHasAlphaBackground.Get() && AlphaBackgroundBrush)
		{
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId++,
				AllottedGeometry.ToPaintGeometry(TrackSize, FSlateLayoutTransform(TrackOffset)),
				AlphaBackgroundBrush,
				DrawEffects);
		}

		TArray<FLinearColor> Colors = GradientColors.Get();
		if (Orientation.Get() == Orient_Vertical) Algo::Reverse(Colors);

		TArray<FSlateGradientStop> GradientStops;
		GradientStops.Reserve(Colors.Num());
		for (int32 ColorIndex = 0; ColorIndex < Colors.Num(); ++ColorIndex)
		{
			const float StopAlpha = static_cast<float>(ColorIndex) / (Colors.Num() - 1);
			GradientStops.Add(FSlateGradientStop(SliderLength * StopAlpha, Colors[ColorIndex].ToFColor(bUseSRGB.Get())));
		}

		const FVector4f CornerRadius(4.0f);
		FSlateDrawElement::MakeGradient(
			OutDrawElements,
			LayerId++,
			AllottedGeometry.ToPaintGeometry(TrackSize, FSlateLayoutTransform(TrackOffset)),
			GradientStops,
			(Orientation.Get() == Orient_Horizontal) ? EOrientation::Orient_Vertical : EOrientation::Orient_Horizontal,
			DrawEffects,
			CornerRadius);
	}

	const FSlateBrush* SelectorBrush = nullptr;
	FVector2f SelectorSize;
	FVector2f SelectorOffset;
	constexpr float SelectorThickness = 3.0f;
	const bool bUseRoundHorizontalSelector = Orientation.Get() == Orient_Horizontal && HorizontalTrackThickness > 0.0f;

	const float SliderValue = GetSliderValue();
	if ((SliderValue > Slider->GetMaxValue()) && bSupportDynamicSliderMaxValue.Get())
		Slider->SetMinAndMaxValues(Slider->GetMinValue(), SliderValue);

	const float SliderRange = FMath::Max(KINDA_SMALL_NUMBER, Slider->GetMaxValue() - Slider->GetMinValue());
	float FractionFilled = FMath::Clamp((SliderValue - Slider->GetMinValue()) / SliderRange, 0.0f, 1.0f);

	if (Orientation.Get() == Orient_Horizontal)
	{
		SelectorBrush = FAppStyle::Get().GetBrush("ColorPicker.SpinBoxSelectorVertical");
		if (bUseRoundHorizontalSelector)
		{
			const float SelectorDiameter = FMath::Max(10.0f, TrackSize.Y + 4.0f);
			SelectorSize = FVector2f(SelectorDiameter, SelectorDiameter);
		}
		else
		{
			SelectorSize = FVector2f(SelectorThickness, FMath::Max(8.0f, TrackSize.Y + 6.0f));
		}

		const float SelectorRange = FMath::Max(0.0f, TrackSize.X - SelectorSize.X);
		SelectorOffset = FVector2f(SelectorRange * FractionFilled, (TrackSize.Y - SelectorSize.Y) * 0.5f);
	}
	else
	{
		SelectorBrush = FAppStyle::Get().GetBrush("ColorPicker.SpinBoxSelectorHorizontal");
		SelectorSize = FVector2f(FMath::Max(1.0f, SliderSize.X - 2.0f), SelectorThickness);

		FractionFilled = 1.0f - FractionFilled;
		const float SelectorRange = FMath::Max(0.0f, SliderSize.Y - SelectorSize.Y);
		SelectorOffset = FVector2f(1.0f, SelectorRange * FractionFilled);
	}

	if (SelectorBrush && !bUseRoundHorizontalSelector)
	{
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId++,
			AllottedGeometry.ToPaintGeometry(SelectorSize, FSlateLayoutTransform(TrackOffset + SelectorOffset)),
			SelectorBrush,
			DrawEffects,
			SelectorBrush->GetTint(InWidgetStyle) * InWidgetStyle.GetColorAndOpacityTint());
	}

	const FSlateBrush* BorderImage = Slider->HasMouseCapture() ? BorderActiveBrush : (Slider->IsHovered() ? BorderHoveredBrush : BorderBrush);
	if (BorderImage)
	{
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId++,
			AllottedGeometry.ToPaintGeometry(TrackSize, FSlateLayoutTransform(TrackOffset)),
			BorderImage,
			DrawEffects,
			BorderImage->GetTint(InWidgetStyle) * InWidgetStyle.GetColorAndOpacityTint());
	}

	if (SelectorBrush && bUseRoundHorizontalSelector)
	{
		const FSlateRoundedBoxBrush RoundSelectorBrush(FLinearColor::White, SelectorSize.X * 0.5f, SelectorSize);
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId++,
			AllottedGeometry.ToPaintGeometry(SelectorSize, FSlateLayoutTransform(TrackOffset + SelectorOffset)),
			&RoundSelectorBrush,
			DrawEffects,
			SelectorBrush->GetTint(InWidgetStyle) * InWidgetStyle.GetColorAndOpacityTint());
	}

	return LayerId;
}

void SRuntimeMeshPaintColorSlider::OnSliderCaptureEnd()
{
	const float Value = GetSliderValue();
	OnEndSliderMovement.ExecuteIfBound(Value);
	OnValueCommitted.ExecuteIfBound(Value);
}

void SRuntimeMeshPaintColorSlider::OnSpinBoxValueCommitted(float NewValue, ETextCommit::Type CommitType)
{
	if (Slider.IsValid() && (NewValue > Slider->GetMaxValue()) && bSupportDynamicSliderMaxValue.Get())
		Slider->SetMinAndMaxValues(Slider->GetMinValue(), NewValue);

	OnEndSpinBoxMovement.ExecuteIfBound(NewValue);
	OnValueCommitted.ExecuteIfBound(NewValue);
}

float SRuntimeMeshPaintColorSlider::GetSliderValue() const
{
	return Slider.IsValid() ? Slider->GetValue() : 0.0f;
}

float SRuntimeMeshPaintColorSlider::GetSliderLength(const FVector2f& SliderSize) const
{
	return Orientation.Get() == Orient_Horizontal ? SliderSize.X : SliderSize.Y;
}
