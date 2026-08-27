#include "ProjectKC/UI/HUD/ViewModel/KCPlayerStatusViewModel.h"

void UKCPlayerStatusViewModel::SetDisplayName(const FText& NewDisplayName)
{
	UE_MVVM_SET_PROPERTY_VALUE(DisplayName, NewDisplayName);
}

void UKCPlayerStatusViewModel::SetHealthRatio(float NewHealthRatio)
{
	UE_MVVM_SET_PROPERTY_VALUE(HealthRatio, NewHealthRatio);
}

void UKCPlayerStatusViewModel::SetDowned(bool bNewDowned)
{
	UE_MVVM_SET_PROPERTY_VALUE(bDowned, bNewDowned);
}

void UKCPlayerStatusViewModel::SetHeldItemName(const FText& NewHeldItemName)
{
	UE_MVVM_SET_PROPERTY_VALUE(HeldItemName, NewHeldItemName);
}

void UKCPlayerStatusViewModel::SetPreviewData(const FText& NewDisplayName, float NewHealthRatio, bool bNewDowned, const FText& NewHeldItemName)
{
	SetDisplayName(NewDisplayName);
	SetHealthRatio(NewHealthRatio);
	SetDowned(bNewDowned);
	SetHeldItemName(NewHeldItemName);
}
