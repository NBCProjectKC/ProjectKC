#include "ProjectKC/UI/World/Player/ViewModel/KCPlayerOverHeadViewModel.h"

void UKCPlayerOverHeadViewModel::SetPlayerName(const FText& NewPlayerName)
{
	if (PlayerName.EqualTo(NewPlayerName))
	{
		return;
	}

	PlayerName = NewPlayerName;
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(PlayerName);
	OnPlayerNameChangedNative.Broadcast(PlayerName);
}

void UKCPlayerOverHeadViewModel::SetTeamId(int32 NewTeamId)
{
	if (TeamId == NewTeamId)
	{
		return;
	}

	TeamId = NewTeamId;
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(TeamId);
	OnTeamIdChangedNative.Broadcast(TeamId);
}

void UKCPlayerOverHeadViewModel::SetOverHeadVisible(bool bNewVisible)
{
	if (bOverHeadVisible == bNewVisible)
	{
		return;
	}

	bOverHeadVisible = bNewVisible;
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(bOverHeadVisible);
	OnVisibilityChangedNative.Broadcast(bOverHeadVisible);
}

void UKCPlayerOverHeadViewModel::SetStamina(float NewStamina, float NewMaxStamina)
{
	const bool bNewStaminaVisible = NewMaxStamina > 0.0f;
	const float NewStaminaPercent = bNewStaminaVisible
		? FMath::Clamp(NewStamina / NewMaxStamina, 0.0f, 1.0f)
		: 0.0f;

	if (FMath::IsNearlyEqual(StaminaPercent, NewStaminaPercent) &&
		bStaminaVisible == bNewStaminaVisible)
	{
		return;
	}

	StaminaPercent = NewStaminaPercent;
	bStaminaVisible = bNewStaminaVisible;
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(StaminaPercent);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(bStaminaVisible);
	OnStaminaChangedNative.Broadcast(StaminaPercent, bStaminaVisible);
}

void UKCPlayerOverHeadViewModel::SetPlayerDisplayInfo(
	const FKCPlayerDisplayInfoStruct& NewDisplayInfo)
{
	if (PlayerDisplayInfo == NewDisplayInfo)
	{
		return;
	}

	PlayerDisplayInfo = NewDisplayInfo;
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(PlayerDisplayInfo);

	SetPlayerName(PlayerDisplayInfo.DisplayName);
	SetTeamId(PlayerDisplayInfo.TeamId);
	SetOverHeadVisible(PlayerDisplayInfo.bVisible);
}

void UKCPlayerOverHeadViewModel::ClearPlayerDisplayInfo()
{
	SetPlayerDisplayInfo(FKCPlayerDisplayInfoStruct());
}
