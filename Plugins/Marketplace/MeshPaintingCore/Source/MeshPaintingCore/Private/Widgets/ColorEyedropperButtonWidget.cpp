// Copyright Shared Orbit 2026. All Rights Reserved.
#include "Widgets/ColorEyedropperButtonWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Widgets/ColorPreviewWidget.h"
#include "Widgets/ColorPickerSlateUtils.h"

UColorEyedropperButtonWidget::UColorEyedropperButtonWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, PreviewColor(FLinearColor::White)
	, ButtonText(FText::FromString(TEXT("3D Eyedropper")))
	, bNativeTreeBuilt(false)
{
}

void UColorEyedropperButtonWidget::SetPreviewColor(FLinearColor NewPreviewColor)
{
	PreviewColor = NewPreviewColor;
	SynchronizeChildWidgets();
}

void UColorEyedropperButtonWidget::SetIconBrush(const FSlateBrush& NewIconBrush)
{
	IconBrush = NewIconBrush;
	SynchronizeChildWidgets();
}

TSharedRef<SWidget> UColorEyedropperButtonWidget::RebuildWidget()
{
	BuildWidgetTree();
	return Super::RebuildWidget();
}

void UColorEyedropperButtonWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	SynchronizeChildWidgets();
}

void UColorEyedropperButtonWidget::BuildWidgetTree()
{
	if (bNativeTreeBuilt && WidgetTree && WidgetTree->RootWidget) return;

	Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	Button->OnClicked.AddDynamic(this, &UColorEyedropperButtonWidget::HandleButtonClicked);

	FButtonStyle ButtonStyle;
	ButtonStyle.SetNormal(FSlateRoundedBoxBrush(
		UE::MeshPaintingCore::ColorPicker::DarkInput(), 5.0f,
		UE::MeshPaintingCore::ColorPicker::Border(), 1.0f));
	ButtonStyle.SetHovered(FSlateRoundedBoxBrush(
		FLinearColor(0.12f, 0.13f, 0.15f, 1.0f), 5.0f,
		UE::MeshPaintingCore::ColorPicker::HoverBorder(), 1.0f));
	ButtonStyle.SetPressed(FSlateRoundedBoxBrush(
		FLinearColor(0.16f, 0.18f, 0.21f, 1.0f), 5.0f,
		UE::MeshPaintingCore::ColorPicker::Accent(), 1.0f));
	ButtonStyle.SetDisabled(FSlateRoundedBoxBrush(
		FLinearColor(0.04f, 0.04f, 0.045f, 0.7f), 5.0f,
		UE::MeshPaintingCore::ColorPicker::Border(), 1.0f));
	Button->SetStyle(ButtonStyle);

	ContentBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

	USizeBox* PreviewSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	PreviewSizeBox->SetWidthOverride(28.0f);
	PreviewSizeBox->SetHeightOverride(22.0f);
	Preview = WidgetTree->ConstructWidget<UColorPreviewWidget>(UColorPreviewWidget::StaticClass());
	Preview->DesiredPreviewSize = FVector2D(28.0f, 22.0f);
	PreviewSizeBox->SetContent(Preview);
	UHorizontalBoxSlot* PreviewSlot = ContentBox->AddChildToHorizontalBox(PreviewSizeBox);
	PreviewSlot->SetPadding(FMargin(10.0f, 4.0f, 8.0f, 4.0f));
	PreviewSlot->SetVerticalAlignment(VAlign_Center);

	USizeBox* IconSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	IconSizeBox->SetWidthOverride(20.0f);
	IconSizeBox->SetHeightOverride(20.0f);
	IconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	IconImage->SetColorAndOpacity(UE::MeshPaintingCore::ColorPicker::Text());
	IconSizeBox->SetContent(IconImage);
	UHorizontalBoxSlot* IconSlot = ContentBox->AddChildToHorizontalBox(IconSizeBox);
	IconSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
	IconSlot->SetVerticalAlignment(VAlign_Center);

	LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	LabelText->SetColorAndOpacity(FSlateColor(UE::MeshPaintingCore::ColorPicker::Text()));
	FSlateFontInfo LabelFont = LabelText->GetFont();
	LabelFont.Size = 12;
	LabelText->SetFont(LabelFont);
	UHorizontalBoxSlot* LabelSlot = ContentBox->AddChildToHorizontalBox(LabelText);
	LabelSlot->SetVerticalAlignment(VAlign_Center);

	Button->SetContent(ContentBox);
	WidgetTree->RootWidget = Button;

	bNativeTreeBuilt = true;
	SynchronizeChildWidgets();
}

void UColorEyedropperButtonWidget::SynchronizeChildWidgets()
{
	if (Preview) Preview->SetColor(PreviewColor);

	if (IconImage) IconImage->SetBrush(IconBrush);

	if (LabelText) LabelText->SetText(ButtonText);
}

void UColorEyedropperButtonWidget::HandleButtonClicked()
{
	OnClicked.Broadcast();
}
