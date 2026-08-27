#include "ProjectKC/UI/Common/Widget/KCUserWidget.h"

#include "ProjectKC/UI/Common/Core/KCUISettings.h"
#include "ProjectKC/UI/Common/Style/KCColorStyle.h"

void UKCUserWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	UKCColorStyle* ResolvedColorStyle = GetColorStyle();
	NativeApplyColorStyle(ResolvedColorStyle);
	BP_ApplyColorStyle(ResolvedColorStyle);
}

UKCColorStyle* UKCUserWidget::GetColorStyle() const
{
	if (ColorStyle)
	{
		return ColorStyle;
	}

	const UKCUISettings* UISettings = GetDefault<UKCUISettings>();
	return UISettings ? UISettings->DefaultColorStyle.LoadSynchronous() : nullptr;
}

void UKCUserWidget::NativeApplyColorStyle(const UKCColorStyle* InColorStyle)
{
}
