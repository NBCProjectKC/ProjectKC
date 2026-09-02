// Copyright Shared Orbit 2026. All Rights Reserved.
#include "Widgets/ColorToolButtonWidget.h"

#include "Input/Reply.h"
#include "Rendering/DrawElements.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/ColorPickerSlateUtils.h"

class SColorToolButtonSlate : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SColorToolButtonSlate) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UColorToolButtonWidget* InOwner)
	{
		Owner = InOwner;
	}

	virtual FVector2D ComputeDesiredSize(float) const override
	{
		if (const UColorToolButtonWidget* OwnerWidget = Owner.Get()) return OwnerWidget->ButtonSize;
		return FVector2D(28.0f, 28.0f);
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
		if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton || !Owner.IsValid() || !IsEnabled()) return FReply::Unhandled();

		bPressed = true;
		Invalidate(EInvalidateWidgetReason::Paint);
		return FReply::Handled().CaptureMouse(SharedThis(this));
	}

	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		UColorToolButtonWidget* OwnerWidget = Owner.Get();
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
		const UColorToolButtonWidget* OwnerWidget = Owner.Get();
		if (!OwnerWidget) return LayerId;

		const bool bEnabled = ShouldBeEnabled(bParentEnabled);
		const ESlateDrawEffect DrawEffects = bEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;
		const FVector2f Size = AllottedGeometry.GetLocalSize();
		const FLinearColor Background = GetBackgroundColor(*OwnerWidget, bEnabled);

		UE::MeshPaintingCore::ColorPicker::DrawRoundedBorderedFill(
			OutDrawElements,
			LayerId,
			AllottedGeometry,
			FVector2f::ZeroVector,
			Size,
			Background,
			OwnerWidget->BorderColor,
			1.0f,
			OwnerWidget->CornerRadius,
			DrawEffects);

		if (OwnerWidget->IconBrush.GetResourceObject() || OwnerWidget->IconBrush.DrawAs != ESlateBrushDrawType::NoDrawType)
		{
			const float IconSide = FMath::Max(8.0f, FMath::Min(Size.X, Size.Y) - 10.0f);
			const FVector2f IconSize(IconSide, IconSide);
			const FVector2f IconPosition((Size.X - IconSide) * 0.5f, (Size.Y - IconSide) * 0.5f);
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId + 2,
				AllottedGeometry.ToPaintGeometry(IconSize, FSlateLayoutTransform(IconPosition)),
				&OwnerWidget->IconBrush,
				DrawEffects,
				OwnerWidget->IconTint);
		}

		return LayerId + 2;
	}

private:
	FLinearColor GetBackgroundColor(const UColorToolButtonWidget& OwnerWidget, bool bEnabled) const
	{
		if (!bEnabled) return OwnerWidget.DisabledColor;

		if (OwnerWidget.bIsChecked) return OwnerWidget.CheckedColor;

		if (bPressed) return OwnerWidget.PressedColor;

		if (bHovered) return OwnerWidget.HoveredColor;

		return OwnerWidget.NormalColor;
	}

	TWeakObjectPtr<UColorToolButtonWidget> Owner;
	bool bHovered = false;
	bool bPressed = false;
};

UColorToolButtonWidget::UColorToolButtonWidget()
	: ToolId(NAME_None)
	, bIsToggle(false)
	, bIsChecked(false)
	, ButtonSize(28.0f, 28.0f)
	, CornerRadius(4.0f)
	, NormalColor(UE::MeshPaintingCore::ColorPicker::DarkInput())
	, HoveredColor(FLinearColor(0.12f, 0.13f, 0.15f, 1.0f))
	, PressedColor(FLinearColor(0.17f, 0.19f, 0.22f, 1.0f))
	, CheckedColor(FLinearColor(0.14f, 0.25f, 0.42f, 1.0f))
	, DisabledColor(FLinearColor(0.04f, 0.045f, 0.05f, 0.6f))
	, BorderColor(UE::MeshPaintingCore::ColorPicker::Border())
	, IconTint(UE::MeshPaintingCore::ColorPicker::Text())
{
	SetVisibility(ESlateVisibility::Visible);
}

void UColorToolButtonWidget::SetIconBrush(const FSlateBrush& NewIconBrush)
{
	IconBrush = NewIconBrush;
	if (MyButton.IsValid()) MyButton->Invalidate(EInvalidateWidgetReason::Paint);
}

void UColorToolButtonWidget::SetToggleMode(bool bNewIsToggle)
{
	bIsToggle = bNewIsToggle;
}

void UColorToolButtonWidget::SetIsChecked(bool bNewIsChecked, bool bBroadcast)
{
	if (bIsChecked == bNewIsChecked) return;

	bIsChecked = bNewIsChecked;
	if (MyButton.IsValid()) MyButton->Invalidate(EInvalidateWidgetReason::Paint);

	if (bBroadcast) OnToggled.Broadcast(ToolId, bIsChecked);
}

void UColorToolButtonWidget::HandleSlateClicked()
{
	if (bIsToggle) SetIsChecked(!bIsChecked, true);

	OnClicked.Broadcast(ToolId);
}

TSharedRef<SWidget> UColorToolButtonWidget::RebuildWidget()
{
	MyButton = SNew(SColorToolButtonSlate, this);
	if (!GetToolTipText().IsEmpty()) MyButton->SetToolTipText(GetToolTipText());
	return MyButton.ToSharedRef();
}

void UColorToolButtonWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyButton.Reset();
}

void UColorToolButtonWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	if (MyButton.IsValid())
	{
		MyButton->SetToolTipText(GetToolTipText());
		MyButton->Invalidate(EInvalidateWidgetReason::Paint);
	}
}

#if WITH_EDITOR
const FText UColorToolButtonWidget::GetPaletteCategory()
{
	return NSLOCTEXT("MeshPaintingCore", "ColorPickerPaletteCategory", "Color Picker");
}
#endif
