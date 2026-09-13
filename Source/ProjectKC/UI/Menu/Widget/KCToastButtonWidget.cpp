#include "ProjectKC/UI/Menu/Widget/KCToastButtonWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"

void UKCToastButtonWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (PositiveButton)
	{
		PositiveButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandlePositiveClicked);
	}

	if (NegativeButton)
	{
		NegativeButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleNegativeClicked);
	}
}

void UKCToastButtonWidget::NativeDestruct()
{
	if (PositiveButton)
	{
		PositiveButton->OnClicked.RemoveAll(this);
	}

	if (NegativeButton)
	{
		NegativeButton->OnClicked.RemoveAll(this);
	}

	PositiveAction.Unbind();
	Super::NativeDestruct();
}

void UKCToastButtonWidget::Configure(const FText& Message, FSimpleDelegate InPositiveAction)
{
	PositiveAction = MoveTemp(InPositiveAction);

	if (ContentText)
	{
		ContentText->SetText(Message);
	}
}

void UKCToastButtonWidget::HandlePositiveClicked()
{
	FSimpleDelegate ActionToExecute = PositiveAction;
	RemoveFromParent();

	if (ActionToExecute.IsBound())
	{
		ActionToExecute.Execute();
	}
}

void UKCToastButtonWidget::HandleNegativeClicked()
{
	RemoveFromParent();
}