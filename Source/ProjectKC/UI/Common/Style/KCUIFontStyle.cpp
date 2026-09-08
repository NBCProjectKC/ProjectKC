#include "ProjectKC/UI/Common/Style/KCUIFontStyle.h"

bool UKCUIFontStyle::HasDefaultFontOverride() const
{
	return HasFontOverride(DefaultFont);
}

bool UKCUIFontStyle::FindFont(FName FontId, FSlateFontInfo& OutFont) const
{
	if (!FontId.IsNone())
	{
		if (const FSlateFontInfo* FoundFont = Fonts.Find(FontId))
		{
			if (HasFontOverride(*FoundFont))
			{
				OutFont = *FoundFont;
				return true;
			}
		}
	}

	if (HasDefaultFontOverride())
	{
		OutFont = DefaultFont;
		return true;
	}

	return false;
}

bool UKCUIFontStyle::HasFontOverride(const FSlateFontInfo& Font)
{
	return Font.FontObject || Font.CompositeFont.IsValid();
}
