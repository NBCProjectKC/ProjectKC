#include "ProjectKC/UI/Interaction/ViewModel/KCInteractionPromptViewModel.h"

void UKCInteractionPromptViewModel::SetVisible(bool bNewVisible)
{
	UE_MVVM_SET_PROPERTY_VALUE(bVisible, bNewVisible);
}

void UKCInteractionPromptViewModel::SetInputText(const FText& NewInputText)
{
	UE_MVVM_SET_PROPERTY_VALUE(InputText, NewInputText);
}

void UKCInteractionPromptViewModel::SetActionText(const FText& NewActionText)
{
	UE_MVVM_SET_PROPERTY_VALUE(ActionText, NewActionText);
}

void UKCInteractionPromptViewModel::SetTargetActor(AActor* NewTargetActor)
{
	UE_MVVM_SET_PROPERTY_VALUE(TargetActor, NewTargetActor);
}

void UKCInteractionPromptViewModel::SetPreviewData(bool bNewVisible, const FText& NewInputText, const FText& NewActionText)
{
	SetVisible(bNewVisible);
	SetInputText(NewInputText);
	SetActionText(NewActionText);
	SetTargetActor(nullptr);
}
