#include "ProjectKC/UI/Loading/Screen/KCLoadingScreen.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "ProjectKC/UI/Loading/ViewModel/KCLoadingViewModel.h"

void UKCLoadingScreen::NativeConstruct()
{
	Super::NativeConstruct();

	RefreshFromViewModel();
}

void UKCLoadingScreen::SetLoadingViewModel(UKCLoadingViewModel* InLoadingViewModel)
{
	LoadingViewModel = InLoadingViewModel;
	RefreshFromViewModel();
}

void UKCLoadingScreen::RefreshFromViewModel()
{
	if (!LoadingViewModel)
	{
		return;
	}

	if (ProgressBar_286)
	{
		ProgressBar_286->SetPercent(LoadingViewModel->GetProgress());
	}

	if (Tip_Text)
	{
		Tip_Text->SetText(LoadingViewModel->GetTipText());
	}

	if (TextBlock_72)
	{
		TextBlock_72->SetText(LoadingViewModel->GetLoadingText());
	}
}
