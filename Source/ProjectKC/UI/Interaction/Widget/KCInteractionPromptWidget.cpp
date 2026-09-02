#include "ProjectKC/UI/Interaction/Widget/KCInteractionPromptWidget.h"

#include "Components/TextBlock.h"
#include "ProjectKC/UI/Interaction/ViewModel/KCInteractionPromptViewModel.h"

void UKCInteractionPromptWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	RefreshFromViewModel();
}

void UKCInteractionPromptWidget::SetViewModel(
	UKCInteractionPromptViewModel* NewViewModel)
{
	if (ViewModel == NewViewModel)
	{
		RefreshFromViewModel();
		return;
	}

	ViewModel = NewViewModel;
	RefreshFromViewModel();
	BP_OnViewModelChanged(ViewModel);
}

void UKCInteractionPromptWidget::RefreshFromViewModel()
{
	if (!ViewModel)
	{
		if (InteractionKeyText)
		{
			InteractionKeyText->SetText(FText::GetEmpty());
		}

		if (PromptText)
		{
			PromptText->SetText(FText::GetEmpty());
		}
		return;
	}

	if (InteractionKeyText)
	{
		const FText KeyText = ViewModel->UsesInputKey()
			? ViewModel->GetInputKey().GetDisplayName()
			: ViewModel->GetInputText();
		InteractionKeyText->SetText(KeyText);
	}

	if (PromptText)
	{
		PromptText->SetText(ViewModel->GetActionText());
	}
}
