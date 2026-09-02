// Copyright Shared Orbit 2026. All Rights Reserved.
#include "Widgets/HexColorInputWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/NativeWidgetHost.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Styling/AppStyle.h"
#include "Templates/UnrealTemplate.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "MeshPaintingCoreHexColorInput"

namespace
{
	bool IsHexInputDigit(TCHAR Character)
	{
		return (Character >= TEXT('0') && Character <= TEXT('9'))
			|| (Character >= TEXT('a') && Character <= TEXT('f'))
			|| (Character >= TEXT('A') && Character <= TEXT('F'));
	}

	FString SanitizeHexText(const FString& Text)
	{
		FString Sanitized = Text.TrimStartAndEnd();
		if (Sanitized.StartsWith(TEXT("#"))) Sanitized.RightChopInline(1);
		return Sanitized;
	}

	bool IsSupportedHexLength(int32 Length)
	{
		return Length == 3 || Length == 6 || Length == 8;
	}

	FLinearColor DecodeHexColor(const FString& Text, EHexColorInputMode InputMode)
	{
		const FColor EncodedColor = FColor::FromHex(Text);
		float Red = EncodedColor.R / 255.0f;
		float Green = EncodedColor.G / 255.0f;
		float Blue = EncodedColor.B / 255.0f;
		const float Alpha = EncodedColor.A / 255.0f;

		if (InputMode == EHexColorInputMode::SRGB)
		{
			Red = Red <= 0.04045f ? Red / 12.92f : FMath::Pow((Red + 0.055f) / 1.055f, 2.4f);
			Green = Green <= 0.04045f ? Green / 12.92f : FMath::Pow((Green + 0.055f) / 1.055f, 2.4f);
			Blue = Blue <= 0.04045f ? Blue / 12.92f : FMath::Pow((Blue + 0.055f) / 1.055f, 2.4f);
		}

		return FLinearColor(Red, Green, Blue, Alpha);
	}
}

UHexColorInputWidget::UHexColorInputWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, InputMode(EHexColorInputMode::SRGB)
	, HexText(TEXT("FFFFFFFF"))
	, bIsValid(true)
	, CachedColor(FLinearColor::White)
	, bIsSynchronizing(false)
{
}

void UHexColorInputWidget::SetHexText(const FString& NewHexText, bool bBroadcastValidity)
{
	HexText = NewHexText;

	FLinearColor ParsedColor;
	bool bHasAlpha = false;
	const bool bNewValid = ValidateHexText(HexText, ParsedColor, bHasAlpha);
	if (bNewValid) CachedColor = ParsedColor;

	UpdateValidationState(bNewValid, bBroadcastValidity);
	SynchronizeSlateWidget();
}

void UHexColorInputWidget::SetHexFromColor(FLinearColor Color, bool bIncludeAlpha)
{
	CachedColor = Color;
	HexText = Color.ToFColor(InputMode == EHexColorInputMode::SRGB).ToHex();
	UpdateValidationState(true, false);
	SynchronizeSlateWidget();
}

void UHexColorInputWidget::SetInputMode(EHexColorInputMode NewInputMode)
{
	if (InputMode == NewInputMode) return;

	FLinearColor ParsedColor;
	bool bHasAlpha = false;
	if (ValidateHexText(HexText, ParsedColor, bHasAlpha)) CachedColor = ParsedColor;

	InputMode = NewInputMode;
	HexText = CachedColor.ToFColor(InputMode == EHexColorInputMode::SRGB).ToHex();
	UpdateValidationState(true, false);
	SynchronizeSlateWidget();
}

bool UHexColorInputWidget::ValidateHexText(const FString& TextToValidate, FLinearColor& OutColor, bool& bOutHasAlpha) const
{
	const FString Sanitized = SanitizeHexText(TextToValidate);
	if (!IsSupportedHexLength(Sanitized.Len()))
	{
		OutColor = FLinearColor::Transparent;
		bOutHasAlpha = false;
		return false;
	}

	for (const TCHAR Character : Sanitized)
	{
		if (!IsHexInputDigit(Character))
		{
			OutColor = FLinearColor::Transparent;
			bOutHasAlpha = false;
			return false;
		}
	}

	bOutHasAlpha = Sanitized.Len() == 8;
	OutColor = DecodeHexColor(Sanitized, InputMode);
	return true;
}

TSharedRef<SWidget> UHexColorInputWidget::RebuildWidget()
{
	TSharedRef<SWidget> HexWidget = BuildSlateWidget();

	if (WidgetTree)
	{
		NativeWidgetHost = WidgetTree->ConstructWidget<UNativeWidgetHost>(UNativeWidgetHost::StaticClass());
		NativeWidgetHost->SetContent(HexWidget);
		WidgetTree->RootWidget = NativeWidgetHost;
	}

	return Super::RebuildWidget();
}

void UHexColorInputWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	TextBox.Reset();
}

void UHexColorInputWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	SynchronizeSlateWidget();
}

void UHexColorInputWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	SynchronizeSlateWidget();
}

TSharedRef<SWidget> UHexColorInputWidget::BuildSlateWidget()
{
	const FSlateFontInfo SmallFont = FAppStyle::Get().GetFontStyle("ColorPicker.SmallFont");

	return SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.AutoWidth()
		[
			SNew(SComboButton)
			.ComboButtonStyle(&FAppStyle::Get().GetWidgetStyle<FComboButtonStyle>("ColorPicker.HexMode"))
			.MenuContent()
			[
				MakeHexModeMenu()
			]
			.ButtonContent()
			[
				SNew(STextBlock)
				.Font(SmallFont)
				.Text_Lambda([this]() { return GetModeButtonText(); })
			]
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Right)
		[
			SAssignNew(TextBox, SEditableTextBox)
			.MinDesiredWidth(109.0f)
			.Text_Lambda([this]() { return GetHexBoxText(); })
			.Font(SmallFont)
			.Padding(FMargin(8.0f, 4.0f, 8.0f, 4.0f))
			.OnTextChanged_UObject(this, &UHexColorInputWidget::HandleTextChanged)
			.OnTextCommitted_UObject(this, &UHexColorInputWidget::HandleTextCommitted)
		];
}

TSharedRef<SWidget> UHexColorInputWidget::MakeHexModeMenu()
{
	FMenuBuilder MenuBuilder(true, nullptr);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("HexMenuText_SRGB", "Hex sRGB"),
		LOCTEXT("HexMenuToolTip_SRGB", "Represents the color using sRGB encoding."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateUObject(this, &UHexColorInputWidget::HandleModeSelected, EHexColorInputMode::SRGB),
			FCanExecuteAction(),
			FIsActionChecked::CreateLambda([this]() { return InputMode == EHexColorInputMode::SRGB; })),
		NAME_None,
		EUserInterfaceActionType::RadioButton);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("HexMenuText_Linear", "Hex Linear"),
		LOCTEXT("HexMenuToolTip_Linear", "Represents the color using linear color values."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateUObject(this, &UHexColorInputWidget::HandleModeSelected, EHexColorInputMode::Linear),
			FCanExecuteAction(),
			FIsActionChecked::CreateLambda([this]() { return InputMode == EHexColorInputMode::Linear; })),
		NAME_None,
		EUserInterfaceActionType::RadioButton);

	return MenuBuilder.MakeWidget();
}

FText UHexColorInputWidget::GetModeButtonText() const
{
	return InputMode == EHexColorInputMode::Linear
		? LOCTEXT("HexMenuText_Linear", "Hex Linear")
		: LOCTEXT("HexMenuText_SRGB", "Hex sRGB");
}

FText UHexColorInputWidget::GetHexBoxText() const
{
	return FText::FromString(HexText);
}

void UHexColorInputWidget::SynchronizeSlateWidget()
{
	TGuardValue<bool> Guard(bIsSynchronizing, true);

	if (!TextBox.IsValid()) return;

	TextBox->SetText(FText::FromString(HexText));
	if (bIsValid) TextBox->SetError(FText::GetEmpty());
	else
	{
		TextBox->SetError(LOCTEXT("InvalidHexColorError", "Use FFF, FFFFFF, FFFFFFFF, or the same values with #."));
	}
}

void UHexColorInputWidget::UpdateValidationState(bool bNewValid, bool bBroadcast)
{
	if (bIsValid == bNewValid)
	{
		bIsValid = bNewValid;
		return;
	}

	bIsValid = bNewValid;
	if (bBroadcast) OnValidityChanged.Broadcast(bIsValid);
}

void UHexColorInputWidget::HandleModeSelected(EHexColorInputMode NewInputMode)
{
	SetInputMode(NewInputMode);
}

void UHexColorInputWidget::HandleTextChanged(const FText& NewText)
{
	if (bIsSynchronizing) return;

	HexText = NewText.ToString();
	FLinearColor ParsedColor;
	bool bHasAlpha = false;
	const bool bNewValid = ValidateHexText(HexText, ParsedColor, bHasAlpha);
	if (bNewValid) CachedColor = ParsedColor;

	UpdateValidationState(bNewValid, true);
	if (TextBox.IsValid()) TextBox->SetError(bIsValid ? FText::GetEmpty() : LOCTEXT("InvalidHexColorShortError", "Invalid hex color."));
}

void UHexColorInputWidget::HandleTextCommitted(const FText& NewText, ETextCommit::Type CommitMethod)
{
	if (bIsSynchronizing) return;

	HexText = NewText.ToString();
	FLinearColor ParsedColor;
	bool bHasAlpha = false;
	const bool bValidCommit = ValidateHexText(HexText, ParsedColor, bHasAlpha);
	if (bValidCommit) CachedColor = ParsedColor;

	UpdateValidationState(bValidCommit, true);
	SynchronizeSlateWidget();
	OnHexCommitted.Broadcast(HexText, ParsedColor, bValidCommit, bHasAlpha);
}

#undef LOCTEXT_NAMESPACE
