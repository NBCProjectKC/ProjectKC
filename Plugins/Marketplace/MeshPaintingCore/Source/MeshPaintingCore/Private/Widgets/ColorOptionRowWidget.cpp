// Copyright Shared Orbit 2026. All Rights Reserved.
#include "Widgets/ColorOptionRowWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CheckBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Templates/UnrealTemplate.h"
#include "Widgets/ColorPickerSlateUtils.h"

UColorOptionRowWidget::UColorOptionRowWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, OptionId(TEXT("sRGBPreview"))
	, Label(FText::FromString(TEXT("sRGB Preview")))
	, bIsChecked(false)
	, bIsSynchronizing(false)
	, bNativeTreeBuilt(false)
{
}

void UColorOptionRowWidget::SetIsChecked(bool bNewChecked, bool bBroadcast)
{
	bIsChecked = bNewChecked;
	SynchronizeChildWidgets();

	if (bBroadcast) OnCheckStateChanged.Broadcast(OptionId, bIsChecked);
}

void UColorOptionRowWidget::SetLabel(FText NewLabel)
{
	Label = NewLabel;
	if (LabelText) LabelText->SetText(Label);
}

TSharedRef<SWidget> UColorOptionRowWidget::RebuildWidget()
{
	BuildWidgetTree();
	return Super::RebuildWidget();
}

void UColorOptionRowWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	SynchronizeChildWidgets();
}

void UColorOptionRowWidget::BuildWidgetTree()
{
	if (bNativeTreeBuilt && WidgetTree && WidgetTree->RootWidget) return;

	RootHorizontalBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	WidgetTree->RootWidget = RootHorizontalBox;

	USizeBox* CheckBoxSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	CheckBoxSizeBox->SetWidthOverride(18.0f);
	CheckBoxSizeBox->SetHeightOverride(18.0f);
	CheckBox = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass());
	CheckBox->OnCheckStateChanged.AddDynamic(this, &UColorOptionRowWidget::HandleCheckStateChanged);
	CheckBoxSizeBox->SetContent(CheckBox);

	UHorizontalBoxSlot* CheckSlot = RootHorizontalBox->AddChildToHorizontalBox(CheckBoxSizeBox);
	CheckSlot->SetPadding(FMargin(0.0f, 0.0f, 6.0f, 0.0f));
	CheckSlot->SetVerticalAlignment(VAlign_Center);

	LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	LabelText->SetColorAndOpacity(FSlateColor(UE::MeshPaintingCore::ColorPicker::Text()));
	FSlateFontInfo LabelFont = LabelText->GetFont();
	LabelFont.Size = 11;
	LabelText->SetFont(LabelFont);
	UHorizontalBoxSlot* LabelSlot = RootHorizontalBox->AddChildToHorizontalBox(LabelText);
	LabelSlot->SetVerticalAlignment(VAlign_Center);

	bNativeTreeBuilt = true;
	SynchronizeChildWidgets();
}

void UColorOptionRowWidget::SynchronizeChildWidgets()
{
	TGuardValue<bool> Guard(bIsSynchronizing, true);

	if (CheckBox) CheckBox->SetIsChecked(bIsChecked);

	if (LabelText) LabelText->SetText(Label);
}

void UColorOptionRowWidget::HandleCheckStateChanged(bool bNewChecked)
{
	if (bIsSynchronizing) return;

	bIsChecked = bNewChecked;
	OnCheckStateChanged.Broadcast(OptionId, bIsChecked);
}
