// Copyright Shared Orbit 2026. All Rights Reserved.
#include "Widgets/VerticalColorBarWidget.h"

#include "Widgets/RuntimeColorSlider.h"

UVerticalColorBarWidget::UVerticalColorBarWidget()
	: Value(1.0f)
	, TopColor(FLinearColor::White)
	, BottomColor(FLinearColor::Black)
	, bShowCheckerboard(false)
	, DesiredBarSize(24.0f, 160.0f)
{
	SetVisibility(ESlateVisibility::Visible);
}

void UVerticalColorBarWidget::SetValue(float NewValue, bool bBroadcast)
{
	Value = FMath::Clamp(NewValue, 0.0f, 1.0f);

	if (MyColorBar.IsValid()) MyColorBar->Invalidate(EInvalidateWidgetReason::Paint);

	if (bBroadcast) OnValueChanged.Broadcast(Value);
}

void UVerticalColorBarWidget::SetGradientColors(FLinearColor NewBottomColor, FLinearColor NewTopColor)
{
	BottomColor = NewBottomColor;
	TopColor = NewTopColor;

	if (MyColorBar.IsValid()) MyColorBar->Invalidate(EInvalidateWidgetReason::Paint);
}

TArray<FLinearColor> UVerticalColorBarWidget::GetGradientColors() const
{
	return { BottomColor, TopColor };
}

bool UVerticalColorBarWidget::HasAlphaBackground() const
{
	return bShowCheckerboard;
}

void UVerticalColorBarWidget::HandleSlateValueChanged(float NewValue)
{
	SetValue(NewValue, false);
	OnValueChanged.Broadcast(Value);
}

void UVerticalColorBarWidget::HandleSlateInteractionStarted()
{
	OnInteractionStarted.Broadcast();
}

void UVerticalColorBarWidget::HandleSlateInteractionFinished(float NewValue)
{
	SetValue(NewValue, false);
	OnValueCommitted.Broadcast(Value);
}

TSharedRef<SWidget> UVerticalColorBarWidget::RebuildWidget()
{
	MyColorBar = SNew(SRuntimeMeshPaintColorSlider)
		.Value_Lambda([this]() { return GetValue(); })
		.MinSliderValue(0.0f)
		.MaxSliderValue(1.0f)
		.MinSpinBoxValue(0.0f)
		.MaxSpinBoxValue(1.0f)
		.Delta(0.001f)
		.SupportDynamicSliderMaxValue(false)
		.Orientation(Orient_Vertical)
		.GradientColors_Lambda([this]() { return GetGradientColors(); })
		.HasAlphaBackground_Lambda([this]() { return HasAlphaBackground(); })
		.UseSRGB(true)
		.ShowLabelAndSpinBox(false)
		.VerticalSliderWidth(DesiredBarSize.X)
		.VerticalSliderHeight(DesiredBarSize.Y)
		.OnBeginSliderMovement(BIND_UOBJECT_DELEGATE(FSimpleDelegate, HandleSlateInteractionStarted))
		.OnValueChanged(SRuntimeMeshPaintColorSlider::FOnValueChanged::CreateUObject(this, &UVerticalColorBarWidget::HandleSlateValueChanged))
		.OnEndSliderMovement(SRuntimeMeshPaintColorSlider::FOnValueCommitted::CreateUObject(this, &UVerticalColorBarWidget::HandleSlateInteractionFinished));

	return MyColorBar.ToSharedRef();
}

void UVerticalColorBarWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyColorBar.Reset();
}

void UVerticalColorBarWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	SetValue(Value, false);
}

#if WITH_EDITOR
const FText UVerticalColorBarWidget::GetPaletteCategory()
{
	return NSLOCTEXT("MeshPaintingCore", "ColorPickerPaletteCategory", "Color Picker");
}
#endif
