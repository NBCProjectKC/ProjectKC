#include "ProjectKC/UI/Common/Widget/KCUserWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "ProjectKC/UI/Common/Core/KCUISettings.h"
#include "ProjectKC/UI/Common/Style/KCColorStyle.h"
#include "ProjectKC/UI/Common/Style/KCUIFontStyle.h"

void UKCUserWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	UKCColorStyle* ResolvedColorStyle = GetColorStyle();
	NativeApplyColorStyle(ResolvedColorStyle);
	BP_ApplyColorStyle(ResolvedColorStyle);

	NativeApplyFontStyle(GetFontStyle());
}

UKCColorStyle* UKCUserWidget::GetColorStyle() const
{
	const UKCUISettings* UISettings = GetDefault<UKCUISettings>();
	return UISettings ? UISettings->DefaultColorStyle.LoadSynchronous() : nullptr;
}

UKCUIFontStyle* UKCUserWidget::GetFontStyle() const
{
	const UKCUISettings* UISettings = GetDefault<UKCUISettings>();
	return UISettings ? UISettings->DefaultFontStyle.LoadSynchronous() : nullptr;
}

void UKCUserWidget::NativeApplyColorStyle(const UKCColorStyle* InColorStyle)
{
}

void UKCUserWidget::NativeApplyFontStyle(const UKCUIFontStyle* InFontStyle)
{
	ApplyFontStyleToWidgetTree(InFontStyle);
}

void UKCUserWidget::ApplyFontStyleToWidgetTree(const UKCUIFontStyle* InFontStyle)
{
	if (!WidgetTree || !InFontStyle)
	{
		return;
	}

	WidgetTree->ForEachWidgetAndDescendants([this, InFontStyle](UWidget* Widget)
	{
		ApplyFontStyleToWidget(Widget, InFontStyle);
	});
}

void UKCUserWidget::ApplyFontStyleToWidget(
	UWidget* Widget,
	const UKCUIFontStyle* InFontStyle)
{
	if (UTextBlock* TextBlock = Cast<UTextBlock>(Widget))
	{
		ApplyFontStyleToTextBlock(TextBlock, InFontStyle);
	}
}

void UKCUserWidget::ApplyFontStyleToTextBlock(
	UTextBlock* TextBlock,
	const UKCUIFontStyle* InFontStyle)
{
	if (!TextBlock || !InFontStyle)
	{
		return;
	}

	FSlateFontInfo ResolvedFont;
	if (!InFontStyle->FindFont(ResolveFontIdForTextBlock(TextBlock), ResolvedFont))
	{
		return;
	}

	FSlateFontInfo Font = TextBlock->GetFont();
	Font.FontObject = ResolvedFont.FontObject;
	Font.CompositeFont = ResolvedFont.CompositeFont;
	Font.TypefaceFontName = ResolvedFont.TypefaceFontName;
	TextBlock->SetFont(Font);
}

FName UKCUserWidget::ResolveFontIdForTextBlock(const UTextBlock* TextBlock) const
{
	if (!TextBlock)
	{
		return DefaultFontId;
	}

	const FName TextWidgetName = TextBlock->GetFName();
	for (const FKCWidgetFontOverride& FontOverride : FontOverrides)
	{
		if (FontOverride.TextWidgetName == TextWidgetName)
		{
			return FontOverride.FontId;
		}
	}

	return DefaultFontId;
}
