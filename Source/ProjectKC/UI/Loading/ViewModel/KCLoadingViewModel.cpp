#include "ProjectKC/UI/Loading/ViewModel/KCLoadingViewModel.h"

void UKCLoadingViewModel::SetProgress(float NewProgress)
{
	UE_MVVM_SET_PROPERTY_VALUE(Progress, NewProgress);
}

void UKCLoadingViewModel::SetLoadingText(const FText& NewLoadingText)
{
	UE_MVVM_SET_PROPERTY_VALUE(LoadingText, NewLoadingText);
}

void UKCLoadingViewModel::SetTipText(const FText& NewTipText)
{
	UE_MVVM_SET_PROPERTY_VALUE(TipText, NewTipText);
}

void UKCLoadingViewModel::SetPreviewData(float NewProgress, const FText& NewLoadingText, const FText& NewTipText)
{
	SetProgress(NewProgress);
	SetLoadingText(NewLoadingText);
	SetTipText(NewTipText);
}
