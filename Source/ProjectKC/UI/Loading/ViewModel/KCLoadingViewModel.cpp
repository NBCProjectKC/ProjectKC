#include "ProjectKC/UI/Loading/ViewModel/KCLoadingViewModel.h"
#include "ProjectKC/UI/Loading/Tip/KCLoadingTipDataAsset.h"

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

void UKCLoadingViewModel::PickRandomTip(const UKCLoadingTipDataAsset* TipsAsset)
{
	if (!TipsAsset || TipsAsset->Tips.Num() == 0)
	{
		return;
	}

	const int32 RandomIndex = FMath::RandHelper(TipsAsset->Tips.Num());
	SetTipText(TipsAsset->Tips[RandomIndex].Text);
}