// Copyright Shared Orbit 2026. All Rights Reserved.
#include "Widgets/ColorSwatchWidget.h"

UColorSwatchWidget::UColorSwatchWidget()
	: SwatchIndex(INDEX_NONE)
{
	bIsClickable = true;
	DesiredPreviewSize = FVector2D(22.0f, 22.0f);
	CornerRadius = 3.0f;
	BorderThickness = 1.0f;
}

void UColorSwatchWidget::SetSwatchIndex(int32 NewIndex)
{
	SwatchIndex = NewIndex;
}

void UColorSwatchWidget::SetSwatchSize(float NewSize)
{
	const float SafeSize = FMath::Max(8.0f, NewSize);
	DesiredPreviewSize = FVector2D(SafeSize, SafeSize);
	InvalidateLayoutAndVolatility();
}

void UColorSwatchWidget::HandleSlateClicked()
{
	Super::HandleSlateClicked();
	OnSwatchSelected.Broadcast(Color, SwatchIndex);
}
