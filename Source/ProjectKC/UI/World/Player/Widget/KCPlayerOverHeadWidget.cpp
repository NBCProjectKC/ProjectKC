#include "ProjectKC/UI/World/Player/Widget/KCPlayerOverHeadWidget.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "ProjectKC/UI/Common/Style/KCColorStyle.h"
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
	PlayerOverHeadViewModel->OnStaminaChangedNative.AddUObject(
		this,
		&ThisClass::HandleStaminaChanged);
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
	PlayerOverHeadViewModel->OnStaminaChangedNative.RemoveAll(this);
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

	RefreshTeamColor();
	RefreshStamina();

	SetVisibility(PlayerOverHeadViewModel->IsOverHeadVisible()
		? ESlateVisibility::SelfHitTestInvisible
		: ESlateVisibility::Hidden);
}

void UKCPlayerOverHeadWidget::NativeApplyColorStyle(const UKCColorStyle* InColorStyle)
{
	Super::NativeApplyColorStyle(InColorStyle);
	RefreshTeamColor();
	RefreshStamina();
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
	RefreshTeamColor();
	RefreshStamina();
}

void UKCPlayerOverHeadWidget::HandleVisibilityChanged(bool bNewVisible)
{
	SetVisibility(bNewVisible
		? ESlateVisibility::SelfHitTestInvisible
		: ESlateVisibility::Hidden);
}

void UKCPlayerOverHeadWidget::HandleStaminaChanged(float NewStaminaPercent, bool bNewVisible)
{
	RefreshStamina();
}

void UKCPlayerOverHeadWidget::RefreshTeamColor()
{
	if (!PlayerNameText || !PlayerOverHeadViewModel)
	{
		return;
	}

	const UKCColorStyle* CurrentColorStyle = GetColorStyle();
	const int32 TeamId = PlayerOverHeadViewModel->GetTeamId();
	if (CurrentColorStyle && CurrentColorStyle->TeamColors.IsValidIndex(TeamId))
	{
		PlayerNameText->SetColorAndOpacity(FSlateColor(CurrentColorStyle->TeamColors[TeamId]));
	}
}

void UKCPlayerOverHeadWidget::RefreshStamina()
{
	if (!SteminaProgressBar || !PlayerOverHeadViewModel)
	{
		return;
	}

	const UKCColorStyle* CurrentColorStyle = GetColorStyle();
	const int32 TeamId = PlayerOverHeadViewModel->GetTeamId();
	if (CurrentColorStyle && CurrentColorStyle->TeamColors.IsValidIndex(TeamId))
	{
		SteminaProgressBar->SetFillColorAndOpacity(CurrentColorStyle->TeamColors[TeamId]);
	}

	SteminaProgressBar->SetPercent(PlayerOverHeadViewModel->GetStaminaPercent());
	SteminaProgressBar->SetVisibility(PlayerOverHeadViewModel->IsStaminaVisible()
		? ESlateVisibility::SelfHitTestInvisible
		: ESlateVisibility::Hidden);
}