#include "ProjectKC/UI/Indicator/Widget/KCWorldIndicatorWidget.h"

void UKCWorldIndicatorWidget::SetTargetActor(AActor* InTargetActor)
{
	TargetActor = InTargetActor;
	OnTargetActorChanged(InTargetActor);
}
