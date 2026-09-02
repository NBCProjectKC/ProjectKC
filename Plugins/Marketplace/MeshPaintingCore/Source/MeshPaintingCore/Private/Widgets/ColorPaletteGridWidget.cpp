// Copyright Shared Orbit 2026. All Rights Reserved.
#include "Widgets/ColorPaletteGridWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Widgets/ColorSwatchWidget.h"

UColorPaletteGridWidget::UColorPaletteGridWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, MaxColors(8)
	, Columns(8)
	, SwatchSize(18.0f)
	, SwatchWidgetClass(UColorSwatchWidget::StaticClass())
	, SelectedIndex(INDEX_NONE)
	, bNativeTreeBuilt(false)
{
	PaletteColors =
	{
		FLinearColor::Black,
		FLinearColor::White
	};
}

void UColorPaletteGridWidget::SetPaletteColors(const TArray<FLinearColor>& NewPaletteColors)
{
	PaletteColors = NewPaletteColors;
	SelectedIndex = INDEX_NONE;
	RebuildPalette();
}

void UColorPaletteGridWidget::RebuildPalette()
{
	if (!GridPanel || !WidgetTree) return;

	GridPanel->ClearChildren();
	Swatches.Reset();

	const int32 SafeColumns = FMath::Max(1, Columns);
	const int32 SafeMaxColors = FMath::Max(1, MaxColors);
	const TSubclassOf<UColorSwatchWidget> SafeSwatchClass = SwatchWidgetClass
		? SwatchWidgetClass
		: TSubclassOf<UColorSwatchWidget>(UColorSwatchWidget::StaticClass());

	GridPanel->SetMinDesiredSlotWidth(SwatchSize + 4.0f);
	GridPanel->SetMinDesiredSlotHeight(SwatchSize + 4.0f);

	for (int32 Index = 0; Index < FMath::Min(PaletteColors.Num(), SafeMaxColors); ++Index)
	{
		const FName SwatchBaseName(*FString::Printf(TEXT("PaletteSwatch_%d"), Index));
		const FName SwatchName = MakeUniqueObjectName(WidgetTree, *SafeSwatchClass, SwatchBaseName);
		UColorSwatchWidget* Swatch = WidgetTree->ConstructWidget<UColorSwatchWidget>(SafeSwatchClass, SwatchName);
		if (!Swatch) continue;

		Swatch->SetColor(PaletteColors[Index]);
		Swatch->SetSwatchIndex(Index);
		Swatch->SetSwatchSize(SwatchSize);
		Swatch->OnSwatchSelected.AddDynamic(this, &UColorPaletteGridWidget::HandleSwatchSelected);
		Swatches.Add(Swatch);

		const int32 Row = Index / SafeColumns;
		const int32 Column = Index % SafeColumns;
		UUniformGridSlot* GridSlot = GridPanel->AddChildToUniformGrid(Swatch, Row, Column);
		GridSlot->SetHorizontalAlignment(HAlign_Center);
		GridSlot->SetVerticalAlignment(VAlign_Center);
	}

	UpdateSelectedStates();
}

void UColorPaletteGridWidget::SetSelectedIndex(int32 NewSelectedIndex, bool bBroadcast)
{
	SelectedIndex = PaletteColors.IsValidIndex(NewSelectedIndex) ? NewSelectedIndex : INDEX_NONE;
	UpdateSelectedStates();

	if (bBroadcast && PaletteColors.IsValidIndex(SelectedIndex)) OnColorSelected.Broadcast(PaletteColors[SelectedIndex], SelectedIndex);
}

TSharedRef<SWidget> UColorPaletteGridWidget::RebuildWidget()
{
	BuildWidgetTree();
	return Super::RebuildWidget();
}

void UColorPaletteGridWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	RebuildPalette();
}

void UColorPaletteGridWidget::BuildWidgetTree()
{
	if (!WidgetTree) WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transactional);

	if (UUniformGridPanel* ExistingGridPanel = WidgetTree ? Cast<UUniformGridPanel>(WidgetTree->RootWidget) : nullptr)
	{
		GridPanel = ExistingGridPanel;
		bNativeTreeBuilt = true;
		RebuildPalette();
		return;
	}

	if (bNativeTreeBuilt && WidgetTree && WidgetTree->RootWidget) return;

	const FName GridPanelName = MakeUniqueObjectName(WidgetTree, UUniformGridPanel::StaticClass(), FName(TEXT("PaletteGridPanel")));
	GridPanel = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(), GridPanelName);
	if (!GridPanel) return;

	GridPanel->SetMinDesiredSlotWidth(SwatchSize + 4.0f);
	GridPanel->SetMinDesiredSlotHeight(SwatchSize + 4.0f);
	WidgetTree->RootWidget = GridPanel;

	bNativeTreeBuilt = true;
	RebuildPalette();
}

void UColorPaletteGridWidget::UpdateSelectedStates()
{
	for (int32 Index = 0; Index < Swatches.Num(); ++Index)
	{
		if (Swatches[Index]) Swatches[Index]->SetSelected(Index == SelectedIndex);
	}
}

void UColorPaletteGridWidget::HandleSwatchSelected(FLinearColor Color, int32 SwatchIndex)
{
	SelectedIndex = PaletteColors.IsValidIndex(SwatchIndex) ? SwatchIndex : INDEX_NONE;
	UpdateSelectedStates();

	if (PaletteColors.IsValidIndex(SelectedIndex)) OnColorSelected.Broadcast(PaletteColors[SelectedIndex], SelectedIndex);
	else
	{
		OnColorSelected.Broadcast(Color, SwatchIndex);
	}
}
