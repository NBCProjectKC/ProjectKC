// Copyright Shared Orbit 2026. All Rights Reserved.
#include "Widgets/ColorPickerToolStripWidget.h"

#include "Engine/Texture2D.h"
#include "Styling/CoreStyle.h"
#include "Styling/AppStyle.h"
#include "UObject/ConstructorHelpers.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "MeshPaintingCoreColorPickerToolStrip"

namespace
{
	const FName ToolStripColorMode(TEXT("ColorMode"));
	const FName ToolStripEyedropper(TEXT("Eyedropper"));
	const FName ToolStripPalette(TEXT("Palette"));
	const FName ToolStripEraser(TEXT("Eraser"));
}

UColorPickerToolStripWidget::UColorPickerToolStripWidget()
	: bUseSpectrumMode(false)
	, bIsEyedropperActive(false)
	, bIsThemePanelVisible(false)
	, bIsEraserActive(false)
	, EraserIconTexture(nullptr)
{
	static ConstructorHelpers::FObjectFinder<UTexture2D> DefaultEraserIcon(
		TEXT("/MeshPaintingCore/Widgets/Icons/T_Eraser.T_Eraser"));
	if (DefaultEraserIcon.Succeeded()) EraserIconTexture = DefaultEraserIcon.Object;

	UpdateEraserIconBrush();
}

void UColorPickerToolStripWidget::SetUseSpectrumMode(bool bNewUseSpectrumMode)
{
	if (bUseSpectrumMode == bNewUseSpectrumMode) return;

	bUseSpectrumMode = bNewUseSpectrumMode;
	InvalidateToolStrip();
}

void UColorPickerToolStripWidget::SetEyedropperActive(bool bNewIsActive)
{
	if (bIsEyedropperActive == bNewIsActive) return;

	bIsEyedropperActive = bNewIsActive;
	InvalidateToolStrip();
}

void UColorPickerToolStripWidget::SetThemePanelVisible(bool bNewIsVisible)
{
	if (bIsThemePanelVisible == bNewIsVisible) return;

	bIsThemePanelVisible = bNewIsVisible;
	InvalidateToolStrip();
}

void UColorPickerToolStripWidget::SetEraserActive(bool bNewIsActive)
{
	if (bIsEraserActive == bNewIsActive) return;

	bIsEraserActive = bNewIsActive;
	InvalidateToolStrip();
}

TSharedRef<SWidget> UColorPickerToolStripWidget::RebuildWidget()
{
	MyToolStrip =
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
			.MinHeight(28.0f)
			.MaxHeight(28.0f)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
					.MinWidth(48.0f)
					.MaxWidth(48.0f)
					[
						SNew(SButton)
							.OnClicked_UObject(this, &UColorPickerToolStripWidget::HandleColorPickerModeClicked)
							.ContentPadding(FMargin(2.0f, 2.5f))
							.Content()
							[
								SNew(SOverlay)
									.ToolTipText(LOCTEXT("ColorPickerModeEToolTip", "Toggle between color wheel and color spectrum."))

								+ SOverlay::Slot()
									[
										SNew(SImage)
											.Image(FAppStyle::Get().GetBrush("ColorPicker.ModeWheel"))
											.Visibility_UObject(this, &UColorPickerToolStripWidget::GetModeWheelVisibility)
									]

								+ SOverlay::Slot()
									[
										SNew(SImage)
											.Image(FAppStyle::Get().GetBrush("ColorPicker.ModeSpectrum"))
											.Visibility_UObject(this, &UColorPickerToolStripWidget::GetModeSpectrumVisibility)
									]
							]
					]

				+ SHorizontalBox::Slot()
					.MinWidth(48.0f)
					.MaxWidth(48.0f)
					.Padding(10.0f, 0.0f, 0.0f, 0.0f)
					[
						SNew(SButton)
							.OnClicked_UObject(this, &UColorPickerToolStripWidget::HandleEyeDropperClicked)
							.ContentPadding(FMargin(2.0f, 2.5f))
							.Content()
							[
								SNew(SOverlay)

								+ SOverlay::Slot()
									[
										SNew(SImage)
											.Image(FCoreStyle::Get().GetBrush("ColorPicker.EyeDropperLarge"))
											.ToolTipText(LOCTEXT("EyeDropperButton_ToolTip", "Sample a color from the 3D viewport."))
											.ColorAndOpacity_UObject(this, &UColorPickerToolStripWidget::GetEyeDropperImageColor)
									]

								+ SOverlay::Slot()
									[
										SNew(SHorizontalBox)

										+ SHorizontalBox::Slot()
											.VAlign(VAlign_Center)
											[
												SNew(STextBlock)
													.Text(LOCTEXT("EscapeCue", "Esc"))
													.ToolTipText(LOCTEXT("EyeDropperEscapeCue_ToolTip", "Hit Escape key to stop the eye dropper"))
													.Visibility_UObject(this, &UColorPickerToolStripWidget::GetEyeDropperEscapeTextVisibility)
											]
									]
							]
					]
			]

		+ SVerticalBox::Slot()
			.MinHeight(28.0f)
			.MaxHeight(28.0f)
			.Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
					.MinWidth(48.0f)
					.MaxWidth(48.0f)
					[
						SNew(SButton)
							.OnClicked_UObject(this, &UColorPickerToolStripWidget::HandleThemePanelClicked)
							.ContentPadding(FMargin(2.0f, 2.5f))
							.ToolTipText(LOCTEXT("ShowHideThemesButtonTooltip", "Toggle visibility of color themes"))
							.Content()
							[
								SNew(SImage)
									.Image_UObject(this, &UColorPickerToolStripWidget::GetThemePanelButtonImageBrush)
							]
					]

				+ SHorizontalBox::Slot()
					.MinWidth(48.0f)
					.MaxWidth(48.0f)
					.Padding(10.0f, 0.0f, 0.0f, 0.0f)
					[
						SNew(SButton)
							.OnClicked_UObject(this, &UColorPickerToolStripWidget::HandleEraserClicked)
							.ContentPadding(FMargin(2.0f, 2.5f))
							.ToolTipText(LOCTEXT("EraserButtonTooltip", "Erase painted color from the mesh."))
							.Content()
							[
								SNew(SImage)
									.Image_UObject(this, &UColorPickerToolStripWidget::GetEraserImageBrush)
									.ColorAndOpacity_UObject(this, &UColorPickerToolStripWidget::GetEraserImageColor)
							]
					]
			];

	return MyToolStrip.ToSharedRef();
}

void UColorPickerToolStripWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyToolStrip.Reset();
}

void UColorPickerToolStripWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	UpdateEraserIconBrush();
	InvalidateToolStrip();
}

FReply UColorPickerToolStripWidget::HandleColorPickerModeClicked()
{
	SetUseSpectrumMode(!bUseSpectrumMode);
	OnButtonClicked.Broadcast(ToolStripColorMode);
	return FReply::Handled();
}

FReply UColorPickerToolStripWidget::HandleEyeDropperClicked()
{
	OnButtonClicked.Broadcast(ToolStripEyedropper);
	return FReply::Handled();
}

FReply UColorPickerToolStripWidget::HandleThemePanelClicked()
{
	OnButtonClicked.Broadcast(ToolStripPalette);
	return FReply::Handled();
}

FReply UColorPickerToolStripWidget::HandleEraserClicked()
{
	OnButtonClicked.Broadcast(ToolStripEraser);
	return FReply::Handled();
}

EVisibility UColorPickerToolStripWidget::GetModeWheelVisibility() const
{
	return bUseSpectrumMode ? EVisibility::Visible : EVisibility::Hidden;
}

EVisibility UColorPickerToolStripWidget::GetModeSpectrumVisibility() const
{
	return bUseSpectrumMode ? EVisibility::Hidden : EVisibility::Visible;
}

EVisibility UColorPickerToolStripWidget::GetEyeDropperEscapeTextVisibility() const
{
	return bIsEyedropperActive ? EVisibility::Visible : EVisibility::Hidden;
}

FSlateColor UColorPickerToolStripWidget::GetEyeDropperImageColor() const
{
	return bIsEyedropperActive ? FLinearColor(0.3f, 0.3f, 0.3f, 1.0f) : FSlateColor::UseForeground();
}

FSlateColor UColorPickerToolStripWidget::GetEraserImageColor() const
{
	return bIsEraserActive ? FLinearColor(0.25f, 0.55f, 0.95f, 1.0f) : FSlateColor::UseForeground();
}

const FSlateBrush* UColorPickerToolStripWidget::GetThemePanelButtonImageBrush() const
{
	return bIsThemePanelVisible
		? FAppStyle::Get().GetBrush("ColorPicker.ColorThemes")
		: FAppStyle::Get().GetBrush("ColorPicker.ColorThemesOff");
}

const FSlateBrush* UColorPickerToolStripWidget::GetEraserImageBrush() const
{
	return &EraserIconBrush;
}

void UColorPickerToolStripWidget::UpdateEraserIconBrush()
{
	EraserIconBrush = FSlateBrush();
	EraserIconBrush.DrawAs = EraserIconTexture ? ESlateBrushDrawType::Image : ESlateBrushDrawType::NoDrawType;
	EraserIconBrush.ImageSize = FVector2D(20.0f, 20.0f);
	EraserIconBrush.TintColor = FSlateColor::UseForeground();
	EraserIconBrush.SetResourceObject(EraserIconTexture);
}

void UColorPickerToolStripWidget::InvalidateToolStrip()
{
	if (MyToolStrip.IsValid()) MyToolStrip->Invalidate(EInvalidateWidgetReason::Paint);
}

#if WITH_EDITOR
const FText UColorPickerToolStripWidget::GetPaletteCategory()
{
	return NSLOCTEXT("MeshPaintingCore", "ColorPickerPaletteCategory", "Color Picker");
}
#endif

#undef LOCTEXT_NAMESPACE
