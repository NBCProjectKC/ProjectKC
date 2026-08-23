#include "ProjectKC/UI/Common/Widget/KCActivatableWidget.h"

void UKCActivatableWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	OnKCActivated();
}

void UKCActivatableWidget::NativeOnDeactivated()
{
	OnKCDeactivated();

	Super::NativeOnDeactivated();
}
