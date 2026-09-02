// Copyright Shared Orbit 2026. All Rights Reserved.
#include "Widgets/ColorHistoryBarWidget.h"

#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/Application/SlateApplication.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "Layout/ArrangedChildren.h"
#include "Misc/ConfigCacheIni.h"
#include "Styling/AppStyle.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/STextEntryPopup.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SPanel.h"
#include "Widgets/SToolTip.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "MeshPaintingCoreColorHistoryBar"

namespace
{
	constexpr int32 MaxRecentRows = 3;
	constexpr int32 FirstRecentRowColors = 14;
	constexpr int32 ColorsPerFullRow = 16;
	constexpr int32 MaxRecentColors = FirstRecentRowColors + ((MaxRecentRows - 1) * ColorsPerFullRow);
	constexpr int32 ThemeStorageVersion = 1;
	const TCHAR* ThemeConfigSection = TEXT("MeshPaintingCore.ColorPickerThemes.Default");

	FLinearColor NormalizeHistoryColor(FLinearColor Color)
	{
		Color.R = FMath::Clamp(Color.R, 0.0f, 1.0f);
		Color.G = FMath::Clamp(Color.G, 0.0f, 1.0f);
		Color.B = FMath::Clamp(Color.B, 0.0f, 1.0f);
		Color.A = 1.0f;
		return Color;
	}

	void RemoveMatchingHistoryColor(TArray<FLinearColor>& Colors, const FLinearColor& Color)
	{
		for (int32 Index = Colors.Num() - 1; Index >= 0; --Index)
		{
			if (Colors[Index].Equals(Color, 0.003f)) Colors.RemoveAt(Index, 1, EAllowShrinking::No);
		}
	}

	FText FormatColorValue(const FText& Identifier, float Value)
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("Identifier"), Identifier);

		if (Value >= 0.0f)
		{
			static const float LogToLog10 = 1.0f / FMath::Loge(10.0f);
			const int32 PreRadixDigits = FMath::Max(0, static_cast<int32>(FMath::Loge(Value + KINDA_SMALL_NUMBER) * LogToLog10));
			const int32 Precision = FMath::Max(0, 2 - PreRadixDigits);

			FNumberFormattingOptions FormatRules;
			FormatRules.MinimumFractionalDigits = Precision;
			Args.Add(TEXT("Value"), FText::AsNumber(Value, &FormatRules));
		}
		else
		{
			Args.Add(TEXT("Value"), FText::GetEmpty());
		}

		return FText::Format(LOCTEXT("ToolTipFormat", "{Identifier}: {Value}"), Args);
	}

	FString ThemeColorToConfigString(const FLinearColor& Color)
	{
		return FString::Printf(TEXT("%.9g|%.9g|%.9g|%.9g"), Color.R, Color.G, Color.B, Color.A);
	}

	bool ThemeConfigStringToColor(const FString& Value, FLinearColor& OutColor)
	{
		TArray<FString> Parts;
		Value.ParseIntoArray(Parts, TEXT("|"), false);
		if (Parts.Num() != 4) return false;

		OutColor.R = FCString::Atof(*Parts[0]);
		OutColor.G = FCString::Atof(*Parts[1]);
		OutColor.B = FCString::Atof(*Parts[2]);
		OutColor.A = FCString::Atof(*Parts[3]);
		OutColor = NormalizeHistoryColor(OutColor);
		return true;
	}
}

class SRuntimeColorHistoryBlock : public SCompoundWidget
{
public:
	DECLARE_DELEGATE_OneParam(FOnColorBlockSelected, int32)

	SLATE_BEGIN_ARGS(SRuntimeColorHistoryBlock)
		: _Color(FLinearColor::White)
		, _ColorIndex(INDEX_NONE)
		, _UseSRGB(true)
		, _UseAlpha(true)
	{}
		SLATE_ATTRIBUTE(FLinearColor, Color)
		SLATE_ARGUMENT(int32, ColorIndex)
		SLATE_ATTRIBUTE(bool, UseSRGB)
		SLATE_ATTRIBUTE(bool, UseAlpha)
		SLATE_EVENT(FOnColorBlockSelected, OnSelected)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		Color = InArgs._Color;
		ColorIndex = InArgs._ColorIndex;
		bUseSRGB = InArgs._UseSRGB;
		bUseAlpha = InArgs._UseAlpha;
		OnSelected = InArgs._OnSelected;

		const FSlateFontInfo SmallLayoutFont = FAppStyle::Get().GetFontStyle("Regular");
		const FLinearColor LinearColor = GetColor();
		const FLinearColor HSVColor = LinearColor.LinearRGBToHSV();

		TSharedPtr<SToolTip> ColorTooltip =
			SNew(SToolTip)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(2.0f)
							[
								SNew(SBox)
									.WidthOverride(110.0f)
									.HeightOverride(110.0f)
									[
										SNew(SColorBlock)
											.Color(LinearColor)
											.AlphaDisplayMode(this, &SRuntimeColorHistoryBlock::GetAlphaDisplayMode)
											.ShowBackgroundForAlpha(this, &SRuntimeColorHistoryBlock::ShouldShowAlphaBackground)
											.UseSRGB(bUseSRGB)
									]
							]

						+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(2.0f)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
									.FillWidth(1.0f)
									[
										SNew(SVerticalBox)
										+ SVerticalBox::Slot().AutoHeight().Padding(3.0f)
										[
											SNew(STextBlock).Font(SmallLayoutFont).Text(FormatColorValue(LOCTEXT("Red", "R"), LinearColor.R))
										]
										+ SVerticalBox::Slot().AutoHeight().Padding(3.0f)
										[
											SNew(STextBlock).Font(SmallLayoutFont).Text(FormatColorValue(LOCTEXT("Green", "G"), LinearColor.G))
										]
										+ SVerticalBox::Slot().AutoHeight().Padding(3.0f)
										[
											SNew(STextBlock).Font(SmallLayoutFont).Text(FormatColorValue(LOCTEXT("Blue", "B"), LinearColor.B))
										]
									]

								+ SHorizontalBox::Slot()
									.FillWidth(1.0f)
									[
										SNew(SVerticalBox)
										+ SVerticalBox::Slot().AutoHeight().Padding(3.0f)
										[
											SNew(STextBlock).Font(SmallLayoutFont).Text(FormatColorValue(LOCTEXT("Hue", "H"), FMath::RoundToFloat(HSVColor.R)))
										]
										+ SVerticalBox::Slot().AutoHeight().Padding(3.0f)
										[
											SNew(STextBlock).Font(SmallLayoutFont).Text(FormatColorValue(LOCTEXT("Saturation", "S"), HSVColor.G))
										]
										+ SVerticalBox::Slot().AutoHeight().Padding(3.0f)
										[
											SNew(STextBlock).Font(SmallLayoutFont).Text(FormatColorValue(LOCTEXT("Value", "V"), HSVColor.B))
										]
									]
							]

						+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(2.0f)
							.HAlign(HAlign_Center)
							[
								SNew(STextBlock)
									.Font(SmallLayoutFont)
									.Text(FormatColorValue(LOCTEXT("Alpha", "A"), LinearColor.A))
									.Visibility(this, &SRuntimeColorHistoryBlock::GetAlphaTextVisibility)
							]
					]
			];

		ChildSlot
		[
			SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("ColorPicker.MultipleValuesBackground"))
				.Padding(FMargin(1.0f))
				.ToolTip(ColorTooltip)
				[
					SNew(SColorBlock)
						.Color(this, &SRuntimeColorHistoryBlock::GetColor)
						.AlphaDisplayMode(this, &SRuntimeColorHistoryBlock::GetSmallBlockAlphaDisplayMode)
						.ShowBackgroundForAlpha(this, &SRuntimeColorHistoryBlock::ShouldShowAlphaBackground)
						.UseSRGB(bUseSRGB)
						.Size(FVector2D(22.0f, 22.0f))
						.CornerRadius(FVector4(4.0f, 4.0f, 4.0f, 4.0f))
				]
		];
	}

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton) return FReply::Handled().CaptureMouse(SharedThis(this));

		return FReply::Unhandled();
	}

	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && MyGeometry.IsUnderLocation(MouseEvent.GetScreenSpacePosition()))
		{
			OnSelected.ExecuteIfBound(ColorIndex);
			return FReply::Handled().ReleaseMouseCapture();
		}

		return FReply::Unhandled();
	}

private:
	FLinearColor GetColor() const
	{
		return Color.Get(FLinearColor::White);
	}

	EColorBlockAlphaDisplayMode GetAlphaDisplayMode() const
	{
		return bUseAlpha.Get(true) ? EColorBlockAlphaDisplayMode::Combined : EColorBlockAlphaDisplayMode::Ignore;
	}

	EColorBlockAlphaDisplayMode GetSmallBlockAlphaDisplayMode() const
	{
		return bUseAlpha.Get(true) ? EColorBlockAlphaDisplayMode::SeparateReverse : EColorBlockAlphaDisplayMode::Ignore;
	}

	bool ShouldShowAlphaBackground() const
	{
		return bUseAlpha.Get(true);
	}

	EVisibility GetAlphaTextVisibility() const
	{
		return bUseAlpha.Get(true) ? EVisibility::Visible : EVisibility::Collapsed;
	}

	TAttribute<FLinearColor> Color;
	TAttribute<bool> bUseSRGB;
	TAttribute<bool> bUseAlpha;
	FOnColorBlockSelected OnSelected;
	int32 ColorIndex = INDEX_NONE;
};

class SRuntimeColorHistoryBar : public SPanel
{
public:
	SLATE_BEGIN_ARGS(SRuntimeColorHistoryBar) {}
	SLATE_END_ARGS()

	SRuntimeColorHistoryBar()
		: Children(this)
	{
	}

	void Construct(const FArguments& InArgs, UColorHistoryBarWidget* InOwner)
	{
		Owner = InOwner;
		Refresh();
	}

	void Refresh()
	{
		Children.Empty();
		ColorBlocks.Empty();

		ThemeButton = MakeThemeButton();
		Children.Add(ThemeButton.ToSharedRef());

		const UColorHistoryBarWidget* OwnerWidget = Owner.Get();
		if (!OwnerWidget) return;

		if (!OwnerWidget->IsShowingRecents())
		{
			AddDeleteOverlay = MakeAddDeleteOverlay();
			Children.Add(AddDeleteOverlay.ToSharedRef());
		}
		else
		{
			AddDeleteOverlay.Reset();
		}

		const TArray<FLinearColor>& DisplayedColors = OwnerWidget->GetDisplayedColors();
		const int32 ColorCount = FMath::Min(DisplayedColors.Num(), OwnerWidget->MaxColors);
		for (int32 ColorIndex = 0; ColorIndex < ColorCount; ++ColorIndex)
		{
			ColorBlocks.Add(
				SNew(SRuntimeColorHistoryBlock)
					.Color(DisplayedColors[ColorIndex])
					.ColorIndex(ColorIndex)
					.UseSRGB(OwnerWidget->bUseSRGB)
					.UseAlpha(OwnerWidget->bUseAlpha)
					.OnSelected(this, &SRuntimeColorHistoryBar::HandleColorBlockSelected));

			Children.Add(ColorBlocks.Last().ToSharedRef());
		}

		Invalidate(EInvalidateWidgetReason::LayoutAndVolatility);
	}

	virtual void OnArrangeChildren(const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren) const override
	{
		if (!ThemeButton.IsValid()) return;

		const FVector2D BlockSize(24.0f, 24.0f);
		constexpr float Padding = 2.0f;

		ArrangedChildren.AddWidget(AllottedGeometry.MakeChild(ThemeButton.ToSharedRef(), FVector2D(0.0f, 0.0f), FVector2D(50.0f, 24.0f)));

		int32 OccupiedGridSlots = 2;
		if (AddDeleteOverlay.IsValid())
		{
			ArrangedChildren.AddWidget(AllottedGeometry.MakeChild(AddDeleteOverlay.ToSharedRef(), FVector2D(52.0f, 0.0f), BlockSize));
			++OccupiedGridSlots;
		}

		const int32 NumColorBlocks = ColorBlocks.Num();
		const int32 NumGridBlocks = NumColorBlocks + OccupiedGridSlots;

		int32 ColorIndex = 0;
		for (int32 GridIndex = OccupiedGridSlots; GridIndex < NumGridBlocks; ++GridIndex)
		{
			const float XOffset = (GridIndex % 16) * (BlockSize.X + Padding);
			const float YOffset = (GridIndex / 16) * (BlockSize.Y + Padding);
			ArrangedChildren.AddWidget(AllottedGeometry.MakeChild(ColorBlocks[ColorIndex].ToSharedRef(), FVector2D(XOffset, YOffset), BlockSize));
			++ColorIndex;
		}
	}

	virtual FVector2D ComputeDesiredSize(float) const override
	{
		const int32 NumColorBlocks = ColorBlocks.Num();
		const int32 OccupiedGridSlots = AddDeleteOverlay.IsValid() ? 3 : 2;
		const int32 NumGridBlocks = NumColorBlocks + OccupiedGridSlots;
		const FVector2D BlockSize(24.0f, 24.0f);
		constexpr float Padding = 2.0f;

		const int32 NumColorRows = ((NumGridBlocks - 1) / 16) + 1;
		const float SizeY = (NumColorRows * (BlockSize.Y + Padding)) - Padding;
		const float SizeX = (16 * (BlockSize.X + Padding)) - Padding;

		return FVector2D(SizeX, SizeY);
	}

	virtual FChildren* GetChildren() override
	{
		return &Children;
	}

private:
	TSharedRef<SWidget> MakeThemeButton()
	{
		return SNew(SComboButton)
			.ComboButtonStyle(&FAppStyle::Get().GetWidgetStyle<FComboButtonStyle>("ColorPicker.ThemesComboButton"))
			.OnGetMenuContent(this, &SRuntimeColorHistoryBar::MakeThemeMenu)
			.ToolTipText(LOCTEXT("ColorThemeComboButtonToolTip", "Color Theme Options"))
			.VAlign(VAlign_Center)
			.ButtonContent()
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
					.MinWidth(16.0f)
					.MaxWidth(16.0f)
					[
						SNew(SImage)
							.Image(this, &SRuntimeColorHistoryBar::GetComboButtonImage)
					]
			];
	}

	TSharedRef<SWidget> MakeThemeMenu()
	{
		FMenuBuilder MenuBuilder(false, nullptr);

		MenuBuilder.BeginSection("RecentsSection");
		{
			MenuBuilder.AddMenuEntry(
				LOCTEXT("RecentsTheme", "Recents"),
				LOCTEXT("RecentsThemeToolTip", "Recently used colors"),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Recent"),
				FUIAction(
					FExecuteAction::CreateSP(this, &SRuntimeColorHistoryBar::SelectRecents),
					FCanExecuteAction(),
					FIsActionChecked::CreateSP(this, &SRuntimeColorHistoryBar::IsRecentsSelected)),
				NAME_None,
				EUserInterfaceActionType::RadioButton);
		}
		MenuBuilder.EndSection();

		MenuBuilder.BeginSection("SavedThemes", LOCTEXT("SavedThemes", "Saved Color Themes"));
		if (const UColorHistoryBarWidget* OwnerWidget = Owner.Get())
		{
			for (int32 ThemeIndex = 0; ThemeIndex < OwnerWidget->SavedThemes.Num(); ++ThemeIndex)
			{
				MenuBuilder.AddMenuEntry(
					FText::FromString(OwnerWidget->SavedThemes[ThemeIndex].Name),
					FText::FromString(OwnerWidget->SavedThemes[ThemeIndex].Name),
					FSlateIcon(FAppStyle::GetAppStyleSetName(), "ColorPicker.ColorThemesSmall"),
					FUIAction(
						FExecuteAction::CreateSP(this, &SRuntimeColorHistoryBar::SelectTheme, ThemeIndex),
						FCanExecuteAction(),
						FIsActionChecked::CreateSP(this, &SRuntimeColorHistoryBar::IsThemeSelected, ThemeIndex)),
					NAME_None,
					EUserInterfaceActionType::RadioButton);
			}
		}
		MenuBuilder.EndSection();

		MenuBuilder.AddMenuSeparator();

		MenuBuilder.BeginSection("AddThemeSection");
		{
			MenuBuilder.AddMenuEntry(
				LOCTEXT("CreateNewTheme", "Create New Theme"),
				LOCTEXT("CreateNewThemeTooltip", "Create New Theme"),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.PlusCircle"),
				FUIAction(FExecuteAction::CreateSP(this, &SRuntimeColorHistoryBar::CreateNewTheme)),
				NAME_None,
				EUserInterfaceActionType::CollapsedButton);
		}
		MenuBuilder.EndSection();

		MenuBuilder.BeginSection("EditCurrentThemeSection", LOCTEXT("EditThemeSection", "Edit Current Theme"));
		{
			MenuBuilder.AddMenuEntry(
				LOCTEXT("RenameTheme", "Rename"),
				LOCTEXT("RenameThemeToolTip", "Rename the currently selected color theme"),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCommands.Rename"),
				FUIAction(
					FExecuteAction::CreateSP(this, &SRuntimeColorHistoryBar::StartRenameCurrentTheme),
					FCanExecuteAction::CreateSP(this, &SRuntimeColorHistoryBar::CanEditCurrentTheme)),
				NAME_None,
				EUserInterfaceActionType::CollapsedButton);

			MenuBuilder.AddMenuEntry(
				LOCTEXT("DuplicateTheme", "Duplicate"),
				LOCTEXT("DuplicateThemeTooltip", "Duplicate the currently selected color theme"),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Duplicate"),
				FUIAction(
					FExecuteAction::CreateSP(this, &SRuntimeColorHistoryBar::DuplicateCurrentTheme),
					FCanExecuteAction::CreateSP(this, &SRuntimeColorHistoryBar::CanEditCurrentTheme)),
				NAME_None,
				EUserInterfaceActionType::CollapsedButton);

			MenuBuilder.AddMenuEntry(
				LOCTEXT("DeleteTheme", "Delete"),
				LOCTEXT("DeleteThemeTooltip", "Delete the currently selected color theme"),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Delete"),
				FUIAction(
					FExecuteAction::CreateSP(this, &SRuntimeColorHistoryBar::DeleteCurrentTheme),
					FCanExecuteAction::CreateSP(this, &SRuntimeColorHistoryBar::CanEditCurrentTheme)),
				NAME_None,
				EUserInterfaceActionType::CollapsedButton);
		}
		MenuBuilder.EndSection();

		return MenuBuilder.MakeWidget();
	}

	TSharedRef<SWidget> MakeAddDeleteOverlay() const
	{
		return SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SButton)
					.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("ColorPicker.AddButton"))
					.ContentPadding(FMargin(4.0f))
					.ToolTipText(LOCTEXT("AddToThemeTooltip", "Add the currently selected color to the current color theme"))
					.OnClicked(this, &SRuntimeColorHistoryBar::HandleAddButtonClicked)
					.Content()
					[
						SNew(SImage)
							.Image(FAppStyle::Get().GetBrush("Icons.Plus"))
					]
			];
	}

	const FSlateBrush* GetComboButtonImage() const
	{
		const UColorHistoryBarWidget* OwnerWidget = Owner.Get();
		return (!OwnerWidget || OwnerWidget->IsShowingRecents())
			? FAppStyle::Get().GetBrush("Icons.Recent")
			: FAppStyle::Get().GetBrush("ColorPicker.ColorThemesSmall");
	}

	void HandleColorBlockSelected(int32 ColorIndex) const
	{
		if (UColorHistoryBarWidget* OwnerWidget = Owner.Get()) OwnerWidget->HandleColorBlockClicked(ColorIndex);
	}

	FReply HandleAddButtonClicked() const
	{
		if (UColorHistoryBarWidget* OwnerWidget = Owner.Get()) OwnerWidget->AddActiveColorToCurrentTheme();
		return FReply::Handled();
	}

	void SelectRecents() const
	{
		if (UColorHistoryBarWidget* OwnerWidget = Owner.Get()) OwnerWidget->SelectRecents();
	}

	void SelectTheme(int32 ThemeIndex) const
	{
		if (UColorHistoryBarWidget* OwnerWidget = Owner.Get()) OwnerWidget->SelectTheme(ThemeIndex);
	}

	void CreateNewTheme() const
	{
		if (UColorHistoryBarWidget* OwnerWidget = Owner.Get()) OwnerWidget->CreateNewTheme();
	}

	void StartRenameCurrentTheme()
	{
		const UColorHistoryBarWidget* OwnerWidget = Owner.Get();
		if (!OwnerWidget || !OwnerWidget->SavedThemes.IsValidIndex(OwnerWidget->SelectedThemeIndex)) return;

		TSharedRef<STextEntryPopup> TextEntry =
			SNew(STextEntryPopup)
				.Label(LOCTEXT("ThemeNameLabel", "Theme Name"))
				.DefaultText(FText::FromString(OwnerWidget->SavedThemes[OwnerWidget->SelectedThemeIndex].Name))
				.SelectAllTextWhenFocused(true)
				.ClearKeyboardFocusOnCommit(false)
				.OnTextCommitted(this, &SRuntimeColorHistoryBar::CommitThemeRename);

		FSlateApplication::Get().PushMenu(
			AsShared(),
			FWidgetPath(),
			TextEntry,
			FSlateApplication::Get().GetCursorPos(),
			FPopupTransitionEffect(FPopupTransitionEffect::TypeInPopup));
	}

	void CommitThemeRename(const FText& NewThemeName, ETextCommit::Type CommitInfo) const
	{
		if (CommitInfo == ETextCommit::OnEnter)
		{
			if (UColorHistoryBarWidget* OwnerWidget = Owner.Get()) OwnerWidget->RenameCurrentTheme(NewThemeName.ToString());
		}

		FSlateApplication::Get().DismissAllMenus();
	}

	void DuplicateCurrentTheme() const
	{
		if (UColorHistoryBarWidget* OwnerWidget = Owner.Get()) OwnerWidget->DuplicateCurrentTheme();
	}

	void DeleteCurrentTheme() const
	{
		if (UColorHistoryBarWidget* OwnerWidget = Owner.Get()) OwnerWidget->DeleteCurrentTheme();
	}

	bool IsRecentsSelected() const
	{
		const UColorHistoryBarWidget* OwnerWidget = Owner.Get();
		return !OwnerWidget || OwnerWidget->IsShowingRecents();
	}

	bool IsThemeSelected(int32 ThemeIndex) const
	{
		const UColorHistoryBarWidget* OwnerWidget = Owner.Get();
		return OwnerWidget && OwnerWidget->SelectedThemeIndex == ThemeIndex;
	}

	bool CanEditCurrentTheme() const
	{
		const UColorHistoryBarWidget* OwnerWidget = Owner.Get();
		return OwnerWidget && !OwnerWidget->IsShowingRecents();
	}

	TWeakObjectPtr<UColorHistoryBarWidget> Owner;
	TSlotlessChildren<SWidget> Children;
	TSharedPtr<SWidget> ThemeButton;
	TSharedPtr<SWidget> AddDeleteOverlay;
	TArray<TSharedPtr<SRuntimeColorHistoryBlock>> ColorBlocks;
};

UColorHistoryBarWidget::UColorHistoryBarWidget()
	: MaxColors(MaxRecentColors)
	, bUseSRGB(true)
	, bUseAlpha(false)
	, ActiveColor(FLinearColor::White)
	, SelectedThemeIndex(INDEX_NONE)
{
}

void UColorHistoryBarWidget::SetHistoryColors(const TArray<FLinearColor>& NewHistoryColors)
{
	HistoryColors.Reset();
	for (FLinearColor Color : NewHistoryColors)
	{
		Color = NormalizeHistoryColor(Color);
		RemoveMatchingHistoryColor(HistoryColors, Color);
		HistoryColors.Add(Color);
	}

	if (HistoryColors.Num() > MaxColors) HistoryColors.SetNum(MaxColors);
	RefreshHistoryBar();
}

void UColorHistoryBarWidget::SetUseSRGB(bool bNewUseSRGB)
{
	if (bUseSRGB == bNewUseSRGB) return;

	bUseSRGB = bNewUseSRGB;
	RefreshHistoryBar();
}

void UColorHistoryBarWidget::SetUseAlpha(bool bNewUseAlpha)
{
	if (bUseAlpha == bNewUseAlpha) return;

	bUseAlpha = bNewUseAlpha;
	RefreshHistoryBar();
}

void UColorHistoryBarWidget::SetActiveColor(FLinearColor NewActiveColor)
{
	ActiveColor = NewActiveColor;
}

void UColorHistoryBarWidget::SelectRecents()
{
	SelectedThemeIndex = INDEX_NONE;
	SaveThemesToConfig();
	RefreshHistoryBar();
}

void UColorHistoryBarWidget::SelectTheme(int32 ThemeIndex)
{
	if (!SavedThemes.IsValidIndex(ThemeIndex)) return;

	SelectedThemeIndex = ThemeIndex;
	SaveThemesToConfig();
	RefreshHistoryBar();
}

void UColorHistoryBarWidget::CreateNewTheme()
{
	FColorHistoryTheme NewTheme;
	NewTheme.Name = MakeUniqueThemeName(LOCTEXT("NewThemeName", "New Theme").ToString());
	SavedThemes.Add(NewTheme);
	SelectedThemeIndex = SavedThemes.Num() - 1;
	SaveThemesToConfig();
	RefreshHistoryBar();
}

void UColorHistoryBarWidget::RenameCurrentTheme(const FString& NewThemeName)
{
	if (!SavedThemes.IsValidIndex(SelectedThemeIndex)) return;

	const FString TrimmedThemeName = NewThemeName.TrimStartAndEnd();
	if (TrimmedThemeName.IsEmpty() || SavedThemes[SelectedThemeIndex].Name == TrimmedThemeName) return;

	SavedThemes[SelectedThemeIndex].Name = FString();
	SavedThemes[SelectedThemeIndex].Name = MakeUniqueThemeName(TrimmedThemeName);
	SaveThemesToConfig();
	RefreshHistoryBar();
}

void UColorHistoryBarWidget::DuplicateCurrentTheme()
{
	if (!SavedThemes.IsValidIndex(SelectedThemeIndex)) return;

	FColorHistoryTheme NewTheme = SavedThemes[SelectedThemeIndex];
	const FText CopyName = FText::Format(LOCTEXT("CopyThemeNameAppend", "{0} Copy"), FText::FromString(NewTheme.Name));
	NewTheme.Name = MakeUniqueThemeName(CopyName.ToString());
	SavedThemes.Add(NewTheme);
	SelectedThemeIndex = SavedThemes.Num() - 1;
	SaveThemesToConfig();
	RefreshHistoryBar();
}

void UColorHistoryBarWidget::DeleteCurrentTheme()
{
	if (!SavedThemes.IsValidIndex(SelectedThemeIndex)) return;

	SavedThemes.RemoveAt(SelectedThemeIndex);
	if (SavedThemes.Num() == 0)
	{
		FColorHistoryTheme DefaultTheme;
		DefaultTheme.Name = MakeUniqueThemeName(LOCTEXT("NewThemeName", "New Theme").ToString());
		SavedThemes.Add(DefaultTheme);
	}

	SelectedThemeIndex = INDEX_NONE;
	SaveThemesToConfig();
	RefreshHistoryBar();
}

void UColorHistoryBarWidget::AddActiveColorToCurrentTheme()
{
	if (!SavedThemes.IsValidIndex(SelectedThemeIndex)) return;

	TArray<FLinearColor>& Colors = SavedThemes[SelectedThemeIndex].Colors;
	const FLinearColor NormalizedColor = NormalizeHistoryColor(ActiveColor);
	RemoveMatchingHistoryColor(Colors, NormalizedColor);
	Colors.Insert(NormalizedColor, 0);
	if (Colors.Num() > MaxColors) Colors.SetNum(MaxColors);

	SaveThemesToConfig();
	RefreshHistoryBar();
}

void UColorHistoryBarWidget::LoadThemesFromConfig()
{
	SavedThemes.Reset();
	int32 LoadedSelectedThemeIndex = INDEX_NONE;

	if (GConfig)
	{
		int32 StoredThemeCount = 0;
		GConfig->GetInt(ThemeConfigSection, TEXT("ThemeCount"), StoredThemeCount, GGameUserSettingsIni);
		GConfig->GetInt(ThemeConfigSection, TEXT("SelectedThemeIndex"), LoadedSelectedThemeIndex, GGameUserSettingsIni);
		StoredThemeCount = FMath::Max(0, StoredThemeCount);

		for (int32 ThemeIndex = 0; ThemeIndex < StoredThemeCount; ++ThemeIndex)
		{
			FColorHistoryTheme Theme;
			const FString ThemePrefix = FString::Printf(TEXT("Theme%d"), ThemeIndex);
			GConfig->GetString(ThemeConfigSection, *(ThemePrefix + TEXT(".Name")), Theme.Name, GGameUserSettingsIni);
			if (Theme.Name.TrimStartAndEnd().IsEmpty())
			{
				Theme.Name = MakeUniqueThemeName(LOCTEXT("NewThemeName", "New Theme").ToString());
			}
			else
			{
				Theme.Name = MakeUniqueThemeName(Theme.Name.TrimStartAndEnd());
			}

			int32 StoredColorCount = 0;
			GConfig->GetInt(ThemeConfigSection, *(ThemePrefix + TEXT(".ColorCount")), StoredColorCount, GGameUserSettingsIni);
			StoredColorCount = FMath::Max(0, StoredColorCount);

			for (int32 ColorIndex = 0; ColorIndex < StoredColorCount; ++ColorIndex)
			{
				FString StoredColor;
				const FString ColorKey = FString::Printf(TEXT("%s.Color%d"), *ThemePrefix, ColorIndex);
				if (!GConfig->GetString(ThemeConfigSection, *ColorKey, StoredColor, GGameUserSettingsIni)) continue;

				FLinearColor ParsedColor;
				if (ThemeConfigStringToColor(StoredColor, ParsedColor))
				{
					RemoveMatchingHistoryColor(Theme.Colors, ParsedColor);
					Theme.Colors.Add(ParsedColor);
				}
			}

			if (Theme.Colors.Num() > MaxColors) Theme.Colors.SetNum(MaxColors);
			SavedThemes.Add(Theme);
		}
	}

	if (SavedThemes.Num() == 0)
	{
		FColorHistoryTheme DefaultTheme;
		DefaultTheme.Name = MakeUniqueThemeName(LOCTEXT("NewThemeName", "New Theme").ToString());
		SavedThemes.Add(DefaultTheme);
	}

	SelectedThemeIndex = SavedThemes.IsValidIndex(LoadedSelectedThemeIndex) ? LoadedSelectedThemeIndex : INDEX_NONE;
}

void UColorHistoryBarWidget::SaveThemesToConfig() const
{
	if (!GConfig) return;

	GConfig->SetInt(ThemeConfigSection, TEXT("Version"), ThemeStorageVersion, GGameUserSettingsIni);
	GConfig->SetInt(ThemeConfigSection, TEXT("ThemeCount"), SavedThemes.Num(), GGameUserSettingsIni);
	GConfig->SetInt(ThemeConfigSection, TEXT("SelectedThemeIndex"), SelectedThemeIndex, GGameUserSettingsIni);

	const int32 EffectiveMaxColors = FMath::Max(1, MaxColors);
	for (int32 ThemeIndex = 0; ThemeIndex < SavedThemes.Num(); ++ThemeIndex)
	{
		const FColorHistoryTheme& Theme = SavedThemes[ThemeIndex];
		const FString ThemePrefix = FString::Printf(TEXT("Theme%d"), ThemeIndex);
		const int32 ColorCount = FMath::Min(Theme.Colors.Num(), EffectiveMaxColors);

		GConfig->SetString(ThemeConfigSection, *(ThemePrefix + TEXT(".Name")), *Theme.Name, GGameUserSettingsIni);
		GConfig->SetInt(ThemeConfigSection, *(ThemePrefix + TEXT(".ColorCount")), ColorCount, GGameUserSettingsIni);

		for (int32 ColorIndex = 0; ColorIndex < ColorCount; ++ColorIndex)
		{
			const FString ColorKey = FString::Printf(TEXT("%s.Color%d"), *ThemePrefix, ColorIndex);
			GConfig->SetString(ThemeConfigSection, *ColorKey, *ThemeColorToConfigString(NormalizeHistoryColor(Theme.Colors[ColorIndex])), GGameUserSettingsIni);
		}
	}

	GConfig->Flush(false, GGameUserSettingsIni);
}

void UColorHistoryBarWidget::HandleColorBlockClicked(int32 ColorIndex)
{
	const TArray<FLinearColor>& DisplayedColors = GetDisplayedColors();
	if (DisplayedColors.IsValidIndex(ColorIndex)) OnColorSelected.Broadcast(DisplayedColors[ColorIndex], ColorIndex);
}

const TArray<FLinearColor>& UColorHistoryBarWidget::GetDisplayedColors() const
{
	if (SavedThemes.IsValidIndex(SelectedThemeIndex)) return SavedThemes[SelectedThemeIndex].Colors;

	return HistoryColors;
}

TSharedRef<SWidget> UColorHistoryBarWidget::RebuildWidget()
{
	if (SavedThemes.Num() == 0) LoadThemesFromConfig();

	return SNew(SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("ColorPicker.RecessedBackground"))
		.Padding(FMargin(8.0f, 8.0f))
		[
			SAssignNew(MyHistoryBar, SRuntimeColorHistoryBar, this)
		];
}

void UColorHistoryBarWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyHistoryBar.Reset();
}

void UColorHistoryBarWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	if (SavedThemes.Num() == 0) LoadThemesFromConfig();
	RefreshHistoryBar();
}

void UColorHistoryBarWidget::RefreshHistoryBar()
{
	if (MyHistoryBar.IsValid()) MyHistoryBar->Refresh();
}

int32 UColorHistoryBarWidget::FindThemeByName(const FString& ThemeName) const
{
	for (int32 ThemeIndex = 0; ThemeIndex < SavedThemes.Num(); ++ThemeIndex)
	{
		if (SavedThemes[ThemeIndex].Name == ThemeName) return ThemeIndex;
	}

	return INDEX_NONE;
}

FString UColorHistoryBarWidget::MakeUniqueThemeName(const FString& ThemeName) const
{
	int32 ThemeId = 1;
	FString NewThemeName = ThemeName;
	while (FindThemeByName(NewThemeName) != INDEX_NONE)
	{
		NewThemeName = FString::Printf(TEXT("%s %d"), *ThemeName, ThemeId);
		++ThemeId;
	}

	return NewThemeName;
}

#if WITH_EDITOR
const FText UColorHistoryBarWidget::GetPaletteCategory()
{
	return NSLOCTEXT("MeshPaintingCore", "ColorPickerPaletteCategory", "Color Picker");
}
#endif

#undef LOCTEXT_NAMESPACE
