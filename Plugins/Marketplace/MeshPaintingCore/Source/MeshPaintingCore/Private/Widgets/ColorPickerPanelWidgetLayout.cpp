// Copyright Shared Orbit 2026. All Rights Reserved.
#include "Widgets/ColorPickerPanelWidget.h"

#include "ColorPickerPanelWidgetPrivate.h"
#include "ColorPicker/ColorPickerPalette.h"
#include "ColorPicker/ColorPickerState.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Widgets/ColorHistoryBarWidget.h"
#include "Widgets/ColorOptionRowWidget.h"
#include "Widgets/ColorPickerToolStripWidget.h"
#include "Widgets/ColorPreviewWidget.h"
#include "Widgets/ColorWheelWidget.h"
#include "Widgets/HexColorInputWidget.h"
#include "Widgets/VerticalColorBarWidget.h"

void UColorPickerPanelWidget::BuildWidgetTree()
{
	if (bNativeTreeBuilt && WidgetTree && WidgetTree->RootWidget) return;

	if (WidgetTree && WidgetTree->RootWidget)
	{
		bNativeTreeBuilt = true;
		bUsingNativeFallbackLayout = false;
		return;
	}

	if (!bBuildNativeLayoutIfMissing) return;

	RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	RootBorder->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.5f));
	RootBorder->SetVerticalAlignment(VAlign_Top);
	RootBorder->SetHorizontalAlignment(HAlign_Left);
	RootBorder->SetPadding(PanelPadding);
	WidgetTree->RootWidget = RootBorder;

	UVerticalBox* RootVerticalBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	RootBorder->SetContent(RootVerticalBox);

	BuildTopSection(RootVerticalBox);
	BuildMiddleSection(RootVerticalBox);
	BuildBottomSection(RootVerticalBox);

	bNativeTreeBuilt = true;
	bUsingNativeFallbackLayout = true;
}

void UColorPickerPanelWidget::BuildTopSection(UVerticalBox* RootVerticalBox)
{
	UHorizontalBox* TopBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UVerticalBoxSlot* TopSlot = RootVerticalBox->AddChildToVerticalBox(TopBox);
	TopSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));

	USizeBox* WheelSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	WheelSizeBox->SetWidthOverride(160.0f);
	WheelSizeBox->SetHeightOverride(160.0f);
	ColorWheel = WidgetTree->ConstructWidget<UColorWheelWidget>(UColorWheelWidget::StaticClass());
	WheelSizeBox->SetContent(ColorWheel);
	UHorizontalBoxSlot* WheelSlot = TopBox->AddChildToHorizontalBox(WheelSizeBox);
	WheelSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));

	USizeBox* SaturationBarSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	SaturationBarSizeBox->SetWidthOverride(24.0f);
	SaturationBarSizeBox->SetHeightOverride(160.0f);
	SaturationBar = WidgetTree->ConstructWidget<UVerticalColorBarWidget>(UVerticalColorBarWidget::StaticClass());
	SaturationBarSizeBox->SetContent(SaturationBar);
	UHorizontalBoxSlot* SaturationBarSlot = TopBox->AddChildToHorizontalBox(SaturationBarSizeBox);
	SaturationBarSlot->SetPadding(FMargin(0.0f, 0.0f, 6.0f, 0.0f));

	USizeBox* ValueBarSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	ValueBarSizeBox->SetWidthOverride(24.0f);
	ValueBarSizeBox->SetHeightOverride(160.0f);
	ValueBar = WidgetTree->ConstructWidget<UVerticalColorBarWidget>(UVerticalColorBarWidget::StaticClass());
	ValueBarSizeBox->SetContent(ValueBar);
	UHorizontalBoxSlot* ValueBarSlot = TopBox->AddChildToHorizontalBox(ValueBarSizeBox);
	ValueBarSlot->SetPadding(FMargin(0.0f, 0.0f, 10.0f, 0.0f));

	PreviewAndToolsBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UHorizontalBoxSlot* RightTopSlot = TopBox->AddChildToHorizontalBox(PreviewAndToolsBox);
	RightTopSlot->SetVerticalAlignment(VAlign_Top);

	CurrentPreview = WidgetTree->ConstructWidget<UColorPreviewWidget>(UColorPreviewWidget::StaticClass());
	CurrentPreview->DesiredPreviewSize = FVector2D(136.0f, 32.0f);
	UVerticalBoxSlot* CurrentPreviewSlot = PreviewAndToolsBox->AddChildToVerticalBox(CurrentPreview);
	CurrentPreviewSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));

	PreviousPreview = WidgetTree->ConstructWidget<UColorPreviewWidget>(UColorPreviewWidget::StaticClass());
	PreviousPreview->DesiredPreviewSize = FVector2D(136.0f, 32.0f);
	PreviousPreview->bIsClickable = true;
	PreviousPreview->SetToolTipText(FText::FromString(TEXT("Restore previous color")));
	UVerticalBoxSlot* PreviousPreviewSlot = PreviewAndToolsBox->AddChildToVerticalBox(PreviousPreview);
	PreviousPreviewSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

	SrgbPreviewOption = WidgetTree->ConstructWidget<UColorOptionRowWidget>(UColorOptionRowWidget::StaticClass());
	SrgbPreviewOption->OptionId = OptionSRGBPreview;
	SrgbPreviewOption->Label = FText::FromString(TEXT("sRGB Preview"));
	PreviewAndToolsBox->AddChildToVerticalBox(SrgbPreviewOption);

	ToolStrip = WidgetTree->ConstructWidget<UColorPickerToolStripWidget>(UColorPickerToolStripWidget::StaticClass());
	UVerticalBoxSlot* ToolStripSlot = PreviewAndToolsBox->AddChildToVerticalBox(ToolStrip);
	ToolStripSlot->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 0.0f));
}

void UColorPickerPanelWidget::BuildMiddleSection(UVerticalBox* RootVerticalBox)
{
	UHorizontalBox* MiddleBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UVerticalBoxSlot* MiddleSlot = RootVerticalBox->AddChildToVerticalBox(MiddleBox);
	MiddleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));

	UVerticalBox* LeftColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UHorizontalBoxSlot* LeftSlot = MiddleBox->AddChildToHorizontalBox(LeftColumn);
	FSlateChildSize ColumnSize;
	ColumnSize.SizeRule = ESlateSizeRule::Fill;
	ColumnSize.Value = 1.0f;
	LeftSlot->SetSize(ColumnSize);
	LeftSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));

	RedRow = MakeChannelRow(LeftColumn, ChannelRed, TEXT("R"), 0.0f, 1.0f, 3);
	GreenRow = MakeChannelRow(LeftColumn, ChannelGreen, TEXT("G"), 0.0f, 1.0f, 3);
	BlueRow = MakeChannelRow(LeftColumn, ChannelBlue, TEXT("B"), 0.0f, 1.0f, 3);

	UVerticalBox* RightColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UHorizontalBoxSlot* RightSlot = MiddleBox->AddChildToHorizontalBox(RightColumn);
	RightSlot->SetSize(ColumnSize);

	HueRow = MakeChannelRow(RightColumn, ChannelHue, TEXT("H"), 0.0f, 360.0f, 1);
	HueRow->GradientMode = EColorBarGradientMode::Hue;
	SaturationRow = MakeChannelRow(RightColumn, ChannelSaturation, TEXT("S"), 0.0f, 1.0f, 3);
	ValueRow = MakeChannelRow(RightColumn, ChannelValue, TEXT("V"), 0.0f, 1.0f, 3);

	HexInput = WidgetTree->ConstructWidget<UHexColorInputWidget>(UHexColorInputWidget::StaticClass());
	UVerticalBoxSlot* HexSlot = RightColumn->AddChildToVerticalBox(HexInput);
	HexSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
}

void UColorPickerPanelWidget::BuildBottomSection(UVerticalBox* RootVerticalBox)
{
	BottomBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	RootVerticalBox->AddChildToVerticalBox(BottomBox);

	HistoryBar = WidgetTree->ConstructWidget<UColorHistoryBarWidget>(UColorHistoryBarWidget::StaticClass());
	HistoryBar->SetVisibility(bRecentColorsExpanded ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	UVerticalBoxSlot* HistorySlot = BottomBox->AddChildToVerticalBox(HistoryBar);
	HistorySlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));

	UVerticalBox* ScalarRowsStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), ScalarRowsStackWidgetName);
	BottomBox->AddChildToVerticalBox(ScalarRowsStack);

	BrushSizeRow = MakeChannelRow(ScalarRowsStack, ChannelBrushSize, TEXT("Brush Size"), MinNormalizedBrushSize, MaxNormalizedBrushSize, 3);
	MetallicRow = MakeChannelRow(ScalarRowsStack, ChannelMetallic, TEXT("Metallic"), 0.0f, 1.0f, 6);
	RoughnessRow = MakeChannelRow(ScalarRowsStack, ChannelRoughness, TEXT("Roughness"), 0.0f, 1.0f, 6);
}

void UColorPickerPanelWidget::ConfigureBoundWidgets()
{
	const bool bCanMutateDesignerLayout = bUsingNativeFallbackLayout;

	if (RootBorder) RootBorder->SetPadding(PanelPadding);

	UWidget* LegacyAlphaBarWidget = WidgetTree ? WidgetTree->FindWidget(TEXT("AlphaBar")) : nullptr;
	if (!SaturationBar) SaturationBar = Cast<UVerticalColorBarWidget>(LegacyAlphaBarWidget);
	if (SaturationBar && bCanMutateDesignerLayout)
	{
		SaturationBar->bShowCheckerboard = false;
		MoveWidgetBefore(SaturationBar, ValueBar, FMargin(0.0f, 0.0f, 6.0f, 0.0f));
	}
	else if (SaturationBar) SaturationBar->bShowCheckerboard = false;
	if (ValueBar && bCanMutateDesignerLayout) SetHorizontalSlotPadding(ValueBar, FMargin(0.0f, 0.0f, 10.0f, 0.0f));
	if (bCanMutateDesignerLayout && LegacyAlphaBarWidget && LegacyAlphaBarWidget != SaturationBar) LegacyAlphaBarWidget->RemoveFromParent();
	if (bCanMutateDesignerLayout)
	{
		if (UWidget* LegacyAlphaRowWidget = WidgetTree ? WidgetTree->FindWidget(TEXT("AlphaRow")) : nullptr)
			LegacyAlphaRowWidget->RemoveFromParent();
		if (UWidget* LegacyOverwriteSamplingOption = WidgetTree ? WidgetTree->FindWidget(TEXT("OverwriteSamplingOption")) : nullptr)
			LegacyOverwriteSamplingOption->RemoveFromParent();
	}

	if (PreviousPreview)
	{
		PreviousPreview->bIsClickable = true;
		PreviousPreview->SetToolTipText(FText::FromString(TEXT("Restore previous color")));
	}

	if (RecentColorsArea && bCanMutateDesignerLayout) RecentColorsArea->SetVisibility(ESlateVisibility::Collapsed);

	if (bCanMutateDesignerLayout)
	{
		EnsureHistoryBarWidget();
		ApplyPreviewLayoutOrdering();
		ApplyScalarLayoutOrdering();
	}
	ConfigureHistoryBar();

	ConfigureChannelRows(bCanMutateDesignerLayout);

	if (SrgbPreviewOption)
	{
		SrgbPreviewOption->OptionId = OptionSRGBPreview;
		if (bCanMutateDesignerLayout) SrgbPreviewOption->SetLabel(FText::FromString(TEXT("sRGB Preview")));
	}

	if (ToolRow && bCanMutateDesignerLayout) ToolRow->SetVisibility(ESlateVisibility::Collapsed);
	if (EyedropperButton && bCanMutateDesignerLayout)
	{
		EyedropperButton->RemoveFromParent();
		EyedropperButton = nullptr;
	}

	if (bCanMutateDesignerLayout) EnsureToolStripWidget();
	ConfigureToolStrip();
}

void UColorPickerPanelWidget::BindChildWidgetEvents()
{
	if (ColorWheel)
	{
		ColorWheel->OnHSVChanged.AddUniqueDynamic(this, &UColorPickerPanelWidget::HandleWheelHSVChanged);
		ColorWheel->OnInteractionStarted.AddUniqueDynamic(this, &UColorPickerPanelWidget::HandleColorInteractionStarted);
		ColorWheel->OnHSVCommitted.AddUniqueDynamic(this, &UColorPickerPanelWidget::HandleWheelHSVCommitted);
	}

	if (SaturationBar)
	{
		SaturationBar->OnValueChanged.AddUniqueDynamic(this, &UColorPickerPanelWidget::HandleVerticalSaturationChanged);
		SaturationBar->OnInteractionStarted.AddUniqueDynamic(this, &UColorPickerPanelWidget::HandleColorInteractionStarted);
		SaturationBar->OnValueCommitted.AddUniqueDynamic(this, &UColorPickerPanelWidget::HandleColorValueCommitted);
	}

	if (ValueBar)
	{
		ValueBar->OnValueChanged.AddUniqueDynamic(this, &UColorPickerPanelWidget::HandleVerticalValueChanged);
		ValueBar->OnInteractionStarted.AddUniqueDynamic(this, &UColorPickerPanelWidget::HandleColorInteractionStarted);
		ValueBar->OnValueCommitted.AddUniqueDynamic(this, &UColorPickerPanelWidget::HandleColorValueCommitted);
	}

	if (PreviousPreview) PreviousPreview->OnClicked.AddUniqueDynamic(this, &UColorPickerPanelWidget::HandlePreviousPreviewClicked);
	if (SrgbPreviewOption) SrgbPreviewOption->OnCheckStateChanged.AddUniqueDynamic(this, &UColorPickerPanelWidget::HandleOptionChanged);
	if (HexInput) HexInput->OnHexCommitted.AddUniqueDynamic(this, &UColorPickerPanelWidget::HandleHexCommitted);
	if (HistoryBar) HistoryBar->OnColorSelected.AddUniqueDynamic(this, &UColorPickerPanelWidget::HandlePaletteColorSelected);
	if (ToolStrip) ToolStrip->OnButtonClicked.AddUniqueDynamic(this, &UColorPickerPanelWidget::HandleToolStripButtonClicked);

	for (UColorChannelRowWidget* Row : GetAllChannelRows())
	{
		if (!Row) continue;

		Row->OnInteractionStarted.AddUniqueDynamic(this, &UColorPickerPanelWidget::HandleChannelInteractionStarted);
		Row->OnValueChanged.AddUniqueDynamic(this, &UColorPickerPanelWidget::HandleChannelChanged);
		Row->OnValueCommitted.AddUniqueDynamic(this, &UColorPickerPanelWidget::HandleChannelCommitted);
	}
}

void UColorPickerPanelWidget::ConfigureChannelRows(bool bApplyDesignDefaults)
{
	const FColorChannelRowConfig Rows[] =
	{
		{ RedRow, ChannelRed, TEXT("R"), 0.0f, 1.0f, 3, EColorBarGradientMode::TwoColor },
		{ GreenRow, ChannelGreen, TEXT("G"), 0.0f, 1.0f, 3, EColorBarGradientMode::TwoColor },
		{ BlueRow, ChannelBlue, TEXT("B"), 0.0f, 1.0f, 3, EColorBarGradientMode::TwoColor },
		{ HueRow, ChannelHue, TEXT("H"), 0.0f, 360.0f, 1, EColorBarGradientMode::Hue },
		{ SaturationRow, ChannelSaturation, TEXT("S"), 0.0f, 1.0f, 3, EColorBarGradientMode::TwoColor },
		{ ValueRow, ChannelValue, TEXT("V"), 0.0f, 1.0f, 3, EColorBarGradientMode::TwoColor },
		{ BrushSizeRow, ChannelBrushSize, TEXT("Brush Size"), MinNormalizedBrushSize, MaxNormalizedBrushSize, 3, EColorBarGradientMode::TwoColor },
		{ MetallicRow, ChannelMetallic, TEXT("Metallic"), 0.0f, 1.0f, 3, EColorBarGradientMode::TwoColor },
		{ RoughnessRow, ChannelRoughness, TEXT("Roughness"), 0.0f, 1.0f, 3, EColorBarGradientMode::TwoColor }
	};

	for (const FColorChannelRowConfig& Row : Rows)
	{
		ConfigureChannelRow(
			Row.Row, Row.ChannelId, Row.Label,
			Row.MinValue, Row.MaxValue, Row.Decimals,
			Row.GradientMode, bApplyDesignDefaults);
	}
}

void UColorPickerPanelWidget::ConfigureChannelRow(
	UColorChannelRowWidget* Row, FName ChannelId, const TCHAR* Label,
	float InMinValue, float InMaxValue, int32 Decimals,
	EColorBarGradientMode InGradientMode, bool bApplyDesignDefaults)
{
	if (!Row) return;

	Row->ChannelId = ChannelId;
	if (!bApplyDesignDefaults) return;

	const bool bIsMaterialScalarRow = ChannelId == ChannelMetallic || ChannelId == ChannelRoughness;
	const bool bIsThinScalarRow = ChannelId == ChannelBrushSize || bIsMaterialScalarRow;
	Row->SetVisibility(ESlateVisibility::Visible);
	Row->SetIsEnabled(true);
	Row->SetLabel(FText::FromString(Label));
	Row->SetRange(InMinValue, InMaxValue);
	Row->SetDecimalPlaces(bIsMaterialScalarRow ? 6 : Decimals);
	Row->GradientMode = InGradientMode;
	Row->SetHorizontalLayout(
		bIsThinScalarRow ? ScalarLabelWidth : 8.0f,
		bIsThinScalarRow ? ScalarSliderLength : 123.0f,
		bIsThinScalarRow ? ScalarSliderHeight : 20.0f,
		bIsThinScalarRow ? ScalarTrackThickness : 0.0f,
		bIsThinScalarRow ? ScalarSpinBoxWidth : 60.0f);
}

void UColorPickerPanelWidget::EnsureToolStripWidget()
{
	if (ToolStrip || !WidgetTree || !PreviewAndToolsBox) return;

	ToolStrip = WidgetTree->ConstructWidget<UColorPickerToolStripWidget>(UColorPickerToolStripWidget::StaticClass(), TEXT("ToolStripRuntime"));
	if (!ToolStrip) return;

	int32 InsertIndex = PreviewAndToolsBox->GetChildrenCount();
	if (SrgbPreviewOption)
	{
		const int32 SrgbOptionIndex = PreviewAndToolsBox->GetChildIndex(SrgbPreviewOption);
		if (SrgbOptionIndex != INDEX_NONE) InsertIndex = SrgbOptionIndex + 1;
	}

	UPanelSlot* InsertedSlot = PreviewAndToolsBox->InsertChildAt(InsertIndex, ToolStrip);
	if (UVerticalBoxSlot* ToolStripSlot = Cast<UVerticalBoxSlot>(InsertedSlot)) ToolStripSlot->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 0.0f));
}

void UColorPickerPanelWidget::ConfigureToolStrip()
{
	if (!ToolStrip) return;

	ToolStrip->SetEyedropperActive(bIsEyedropperActive);
	ToolStrip->SetThemePanelVisible(bRecentColorsExpanded);
	ToolStrip->SetEraserActive(IsEraserActive());
	if (bUsingNativeFallbackLayout) ApplyColorModeToWidgets();
}

void UColorPickerPanelWidget::ApplyColorModeToWidgets()
{
	const bool bUseSpectrumMode = ToolStrip && ToolStrip->GetUseSpectrumMode();
	if (ColorWheel) ColorWheel->SetUseSpectrumMode(bUseSpectrumMode);
	if (SaturationBar) SaturationBar->SetVisibility(bUseSpectrumMode ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	if (ValueBar) ValueBar->SetVisibility(bUseSpectrumMode ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
}

void UColorPickerPanelWidget::EnsureHistoryBarWidget()
{
	if (HistoryBar || !WidgetTree) return;

	HistoryBar = WidgetTree->ConstructWidget<UColorHistoryBarWidget>(UColorHistoryBarWidget::StaticClass(), TEXT("HistoryBarRuntime"));
	if (!HistoryBar) return;

	UPanelWidget* ParentPanel = BottomBox;
	int32 InsertIndex = 0;
	if (!ParentPanel && RecentColorsArea && RecentColorsArea->Slot)
	{
		ParentPanel = RecentColorsArea->Slot->Parent;
		InsertIndex = ParentPanel ? ParentPanel->GetChildIndex(RecentColorsArea) : 0;
	}

	if (!ParentPanel) return;

	UPanelSlot* InsertedSlot = ParentPanel->InsertChildAt(InsertIndex, HistoryBar);
	if (UVerticalBoxSlot* HistorySlot = Cast<UVerticalBoxSlot>(InsertedSlot)) HistorySlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
}

void UColorPickerPanelWidget::ConfigureHistoryBar()
{
	if (!HistoryBar) return;

	HistoryBar->MaxColors = 46;
	if (bUsingNativeFallbackLayout) HistoryBar->SetVisibility(bRecentColorsExpanded ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	HistoryBar->SetUseSRGB(ColorState ? ColorState->bSRGBPreview : true);
	HistoryBar->SetUseAlpha(false);
	HistoryBar->SetActiveColor(CurrentColor);

	if (PaletteStorage) HistoryBar->SetHistoryColors(PaletteStorage->GetCombinedColors());
}

void UColorPickerPanelWidget::ApplyPreviewLayoutOrdering()
{
	if (!PreviewAndToolsBox) return;

	if (CurrentPreview) CurrentPreview->DesiredPreviewSize = FVector2D(136.0f, 32.0f);
	if (PreviousPreview)
	{
		PreviousPreview->DesiredPreviewSize = FVector2D(136.0f, 32.0f);
		PreviousPreview->bIsClickable = true;
		PreviousPreview->SetToolTipText(FText::FromString(TEXT("Restore previous color")));
	}

	DetachWidgetFromParent(CurrentPreview);
	DetachWidgetFromParent(PreviousPreview);

	if (UWidget* PreviewRow = WidgetTree ? WidgetTree->FindWidget(FName(TEXT("PreviewRow"))) : nullptr)
	{
		if (PreviewRow->Slot && PreviewRow->Slot->Parent == PreviewAndToolsBox) PreviewAndToolsBox->RemoveChild(PreviewRow);
	}

	int32 InsertIndex = 0;
	if (CurrentPreview)
	{
		if (UVerticalBoxSlot* CurrentPreviewSlot = Cast<UVerticalBoxSlot>(PreviewAndToolsBox->InsertChildAt(InsertIndex++, CurrentPreview)))
			CurrentPreviewSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
	}
	if (PreviousPreview)
	{
		if (UVerticalBoxSlot* PreviousPreviewSlot = Cast<UVerticalBoxSlot>(PreviewAndToolsBox->InsertChildAt(InsertIndex++, PreviousPreview)))
			PreviousPreviewSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
	}
}

void UColorPickerPanelWidget::ApplyScalarLayoutOrdering()
{
	if (!BottomBox || !WidgetTree) return;

	UVerticalBox* ScalarRowsStack = Cast<UVerticalBox>(WidgetTree->FindWidget(ScalarRowsStackWidgetName));
	if (!ScalarRowsStack) ScalarRowsStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), ScalarRowsStackWidgetName);

	if (!BrushSizeRow) BrushSizeRow = FindOrCreateChannelRow(FName(TEXT("BrushSizeRow")));
	if (!MetallicRow) MetallicRow = FindOrCreateChannelRow(FName(TEXT("MetallicRow")));
	if (!RoughnessRow) RoughnessRow = FindOrCreateChannelRow(FName(TEXT("RoughnessRow")));

	DetachWidgetFromParent(BrushSizeRow);
	DetachWidgetFromParent(MetallicRow);
	DetachWidgetFromParent(RoughnessRow);
	if (UWidget* LegacyScalarRowsBox = WidgetTree->FindWidget(FName(TEXT("ScalarRowsBox")))) DetachWidgetFromParent(LegacyScalarRowsBox);
	if (UWidget* LegacyMetallicColumn = WidgetTree->FindWidget(FName(TEXT("MetallicColumn")))) DetachWidgetFromParent(LegacyMetallicColumn);
	if (UWidget* LegacyRoughnessColumn = WidgetTree->FindWidget(FName(TEXT("RoughnessColumn")))) DetachWidgetFromParent(LegacyRoughnessColumn);
	if (UWidget* LegacyScalarRowsStack = WidgetTree->FindWidget(LegacyScalarRowsStackWidgetName))
	{
		if (LegacyScalarRowsStack != ScalarRowsStack) DetachWidgetFromParent(LegacyScalarRowsStack);
	}
	DetachWidgetFromParent(ScalarRowsStack);

	int32 InsertIndex = 0;
	if (HistoryBar)
	{
		const int32 HistoryIndex = BottomBox->GetChildIndex(HistoryBar);
		if (HistoryIndex != INDEX_NONE) InsertIndex = HistoryIndex + 1;
	}

	BottomBox->InsertChildAt(InsertIndex, ScalarRowsStack);
	ScalarRowsStack->ClearChildren();

	AddScalarRowToStack(ScalarRowsStack, BrushSizeRow, FMargin(0.0f, 0.0f, 0.0f, 2.0f));
	AddScalarRowToStack(ScalarRowsStack, MetallicRow, FMargin(0.0f, 0.0f, 0.0f, 2.0f));
	AddScalarRowToStack(ScalarRowsStack, RoughnessRow, FMargin(0.0f));
}

TArray<UColorChannelRowWidget*> UColorPickerPanelWidget::GetAllChannelRows() const
{
	return
	{
		RedRow.Get(),
		GreenRow.Get(),
		BlueRow.Get(),
		HueRow.Get(),
		SaturationRow.Get(),
		ValueRow.Get(),
		BrushSizeRow.Get(),
		MetallicRow.Get(),
		RoughnessRow.Get()
	};
}

UColorChannelRowWidget* UColorPickerPanelWidget::FindOrCreateChannelRow(FName WidgetName)
{
	if (!WidgetTree) return nullptr;

	if (UColorChannelRowWidget* Row = Cast<UColorChannelRowWidget>(WidgetTree->FindWidget(WidgetName))) return Row;

	return WidgetTree->ConstructWidget<UColorChannelRowWidget>(UColorChannelRowWidget::StaticClass(), WidgetName);
}

void UColorPickerPanelWidget::AddScalarRowToStack(UVerticalBox* ScalarRowsStack, UColorChannelRowWidget* Row, const FMargin& RowPadding)
{
	if (!ScalarRowsStack || !Row) return;

	if (ScalarRowsStack->AddChildToVerticalBox(Row)) SetVerticalAutoSlot(Row, RowPadding);
}

UColorChannelRowWidget* UColorPickerPanelWidget::MakeChannelRow(
	UVerticalBox* Parent, FName ChannelId, const FString& Label,
	float InMinValue, float InMaxValue, int32 Decimals)
{
	UColorChannelRowWidget* Row = WidgetTree->ConstructWidget<UColorChannelRowWidget>(UColorChannelRowWidget::StaticClass());
	Row->ChannelId = ChannelId;
	Row->Label = FText::FromString(Label);
	Row->MinValue = InMinValue;
	Row->MaxValue = InMaxValue;
	Row->DecimalPlaces = Decimals;

	Parent->AddChildToVerticalBox(Row);
	SetVerticalAutoSlot(Row, FMargin(0.0f, 0.0f, 0.0f, 4.0f));
	return Row;
}
