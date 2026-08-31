#include "ProjectKC/UI/World/Player/Widget/KCPlayerOverHeadWidget.h"

#include "Components/TextBlock.h"
#include "ProjectKC/UI/World/Player/ViewModel/KCPlayerOverHeadViewModel.h"

void UKCPlayerOverHeadWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BindViewModel();
	RefreshFromViewModel();
}

void UKCPlayerOverHeadWidget::NativeDestruct()
{
	UnbindViewModel();

	Super::NativeDestruct();
}

void UKCPlayerOverHeadWidget::SetViewModel(UKCPlayerOverHeadViewModel* InViewModel)
{
	if (PlayerOverHeadViewModel == InViewModel)
	{
		return;
	}

	UnbindViewModel();
	PlayerOverHeadViewModel = InViewModel;
	BindViewModel();
	RefreshFromViewModel();
}

void UKCPlayerOverHeadWidget::BindViewModel()
{
	if (!PlayerOverHeadViewModel)
	{
		return;
	}

	PlayerOverHeadViewModel->OnPlayerNameChangedNative.AddUObject(
		this,
		&ThisClass::HandlePlayerNameChanged);
	PlayerOverHeadViewModel->OnTeamIdChangedNative.AddUObject(
		this,
		&ThisClass::HandleTeamIdChanged);
	PlayerOverHeadViewModel->OnVisibilityChangedNative.AddUObject(
		this,
		&ThisClass::HandleVisibilityChanged);
}

void UKCPlayerOverHeadWidget::UnbindViewModel()
{
	if (!PlayerOverHeadViewModel)
	{
		return;
	}

	PlayerOverHeadViewModel->OnPlayerNameChangedNative.RemoveAll(this);
	PlayerOverHeadViewModel->OnTeamIdChangedNative.RemoveAll(this);
	PlayerOverHeadViewModel->OnVisibilityChangedNative.RemoveAll(this);
}

void UKCPlayerOverHeadWidget::RefreshFromViewModel()
{
	if (!PlayerOverHeadViewModel)
	{
		SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	if (PlayerNameText)
	{
		PlayerNameText->SetText(PlayerOverHeadViewModel->GetPlayerName());
	}

	SetVisibility(PlayerOverHeadViewModel->IsOverHeadVisible()
		? ESlateVisibility::SelfHitTestInvisible
		: ESlateVisibility::Hidden);
}

void UKCPlayerOverHeadWidget::HandlePlayerNameChanged(const FText& NewPlayerName)
{
	if (PlayerNameText)
	{
		PlayerNameText->SetText(NewPlayerName);
	}
}

void UKCPlayerOverHeadWidget::HandleTeamIdChanged(int32 NewTeamId)
{
	RefreshFromViewModel();
}

void UKCPlayerOverHeadWidget::HandleVisibilityChanged(bool bNewVisible)
{
	SetVisibility(bNewVisible
		? ESlateVisibility::SelfHitTestInvisible
		: ESlateVisibility::Hidden);
}
