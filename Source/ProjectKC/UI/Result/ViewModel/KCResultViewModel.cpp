#include "ProjectKC/UI/Result/ViewModel/KCResultViewModel.h"

void UKCResultViewModel::SetTeams(const TArray<FKCResultTeamViewData>& NewTeams)
{
	Teams = NewTeams;
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Teams);
}

void UKCResultViewModel::SetPreviewData(const TArray<FKCResultTeamViewData>& NewTeams)
{
	SetTeams(NewTeams);
}
