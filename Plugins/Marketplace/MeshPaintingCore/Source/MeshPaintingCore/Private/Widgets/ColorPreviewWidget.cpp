// Copyright Shared Orbit 2026. All Rights Reserved.
#include "Widgets/ColorPreviewWidget.h"

#include "Input/Reply.h"
#include "Rendering/DrawElements.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/ColorPickerSlateUtils.h"

class SColorPreviewSlate : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SColorPreviewSlate) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UColorPreviewWidget* InOwner)
	{
		Owner = InOwner;
	}

	virtual FVector2D ComputeDesiredSize(float) const override
	{
		if (const UColorPreviewWidget* OwnerWidget = Owner.Get()) return OwnerWidget->DesiredPreviewSize;
		return FVector2D(56.0f, 42.0f);
	}

	virtual void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		bHovered = true;
		Invalidate(EInvalidateWidgetReason::Paint);
	}

	virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override
	{
		bHovered = false;
		bPressed = false;
		Invalidate(EInvalidateWidgetReason::Paint);
	}

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		UColorPreviewWidget* OwnerWidget = Owner.Get();
		if (!OwnerWidget || !OwnerWidget->bIsClickable || MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Unhandled();

		bPressed = true;
		Invalidate(EInvalidateWidgetReason::Paint);
		return FReply::Handled().CaptureMouse(SharedThis(this));
	}

	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		UColorPreviewWidget* OwnerWidget = Owner.Get();
		if (!OwnerWidget || MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton || !HasMouseCapture()) return FReply::Unhandled();

		bPressed = false;
		const FVector2f LocalMouse = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
		const FVector2f LocalSize = MyGeometry.GetLocalSize();
		const bool bInside = LocalMouse.X >= 0.0f && LocalMouse.Y >= 0.0f && LocalMouse.X <= LocalSize.X && LocalMouse.Y <= LocalSize.Y;
		if (bInside) OwnerWidget->HandleSlateClicked();

		Invalidate(EInvalidateWidgetReason::Paint);
		return FReply::Handled().ReleaseMouseCapture();
	}

	virtual int32 OnPaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override
	{
		const UColorPreviewWidget* OwnerWidget = Owner.Get();
		if (!OwnerWidget) return LayerId;

		const bool bEnabled = ShouldBeEnabled(bParentEnabled);
		const ESlateDrawEffect DrawEffects = bEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;
		const FVector2f Size = AllottedGeometry.GetLocalSize();
		const FVector2f Position(0.0f, 0.0f);
		const FLinearColor BorderColor = GetBorderColor(*OwnerWidget);

		UE::MeshPaintingCore::ColorPicker::DrawRoundedFill(
			OutDrawElements,
			LayerId,
			AllottedGeometry,
			Position,
			Size,
			BorderColor,
			OwnerWidget->CornerRadius,
			DrawEffects);

		const float Inset = FMath::Max(0.0f, OwnerWidget->BorderThickness);
		const FVector2f InnerPosition(Inset, Inset);
		const FVector2f InnerSize(FMath::Max(1.0f, Size.X - Inset * 2.0f), FMath::Max(1.0f, Size.Y - Inset * 2.0f));

		if (OwnerWidget->bShowCheckerboardForAlpha && OwnerWidget->Color.A < 1.0f)
		{
			UE::MeshPaintingCore::ColorPicker::DrawCheckerboard(
				OutDrawElements,
				LayerId + 1,
				AllottedGeometry,
				InnerPosition,
				InnerSize,
				OwnerWidget->CheckerCellSize,
				DrawEffects);
		}
		else
		{
			UE::MeshPaintingCore::ColorPicker::DrawRoundedFill(
				OutDrawElements,
				LayerId + 1,
				AllottedGeometry,
				InnerPosition,
				InnerSize,
				FLinearColor::Black,
				FMath::Max(0.0f, OwnerWidget->CornerRadius - Inset),
				DrawEffects);
		}

		UE::MeshPaintingCore::ColorPicker::DrawRoundedFill(
			OutDrawElements,
			LayerId + 2,
			AllottedGeometry,
			InnerPosition,
			InnerSize,
			OwnerWidget->Color,
			FMath::Max(0.0f, OwnerWidget->CornerRadius - Inset),
			DrawEffects);

		return LayerId + 2;
	}

private:
	FLinearColor GetBorderColor(const UColorPreviewWidget& OwnerWidget) const
	{
		if (OwnerWidget.bIsSelected) return OwnerWidget.SelectedBorderColor;

		if (bPressed) return OwnerWidget.PressedBorderColor;

		if (bHovered) return OwnerWidget.HoveredBorderColor;

		return OwnerWidget.NormalBorderColor;
	}

	TWeakObjectPtr<UColorPreviewWidget> Owner;
	bool bHovered = false;
	bool bPressed = false;
};

UColorPreviewWidget::UColorPreviewWidget()
	: Color(FLinearColor::White)
	, bShowCheckerboardForAlpha(true)
	, bIsClickable(false)
	, bIsSelected(false)
	, DesiredPreviewSize(56.0f, 42.0f)
	, CornerRadius(4.0f)
	, BorderThickness(1.0f)
	, NormalBorderColor(UE::MeshPaintingCore::ColorPicker::Border())
	, HoveredBorderColor(UE::MeshPaintingCore::ColorPicker::HoverBorder())
	, PressedBorderColor(FLinearColor(0.72f, 0.78f, 0.86f, 1.0f))
	, SelectedBorderColor(UE::MeshPaintingCore::ColorPicker::Accent())
	, CheckerCellSize(6.0f)
{
	SetVisibility(ESlateVisibility::Visible);
}

void UColorPreviewWidget::SetColor(FLinearColor NewColor)
{
	Color = NewColor;
	if (MyPreview.IsValid()) MyPreview->Invalidate(EInvalidateWidgetReason::Paint);
}

void UColorPreviewWidget::SetSelected(bool bNewSelected)
{
	bIsSelected = bNewSelected;
	if (MyPreview.IsValid()) MyPreview->Invalidate(EInvalidateWidgetReason::Paint);
}

void UColorPreviewWidget::HandleSlateClicked()
{
	OnClicked.Broadcast(Color);
}

TSharedRef<SWidget> UColorPreviewWidget::RebuildWidget()
{
	MyPreview = SNew(SColorPreviewSlate, this);
	return MyPreview.ToSharedRef();
}

void UColorPreviewWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyPreview.Reset();
}

void UColorPreviewWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	SetColor(Color);
	SetSelected(bIsSelected);
}

#if WITH_EDITOR
const FText UColorPreviewWidget::GetPaletteCategory()
{
	return NSLOCTEXT("MeshPaintingCore", "ColorPickerPaletteCategory", "Color Picker");
}
#endif
