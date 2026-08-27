#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/UI/Common/Core/KCViewModelBase.h"
#include "KCResultViewModel.generated.h"

USTRUCT(BlueprintType)
struct FKCResultTeamViewData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	int32 TeamId = INDEX_NONE;

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	int32 Score = 0;

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	int32 Rank = 0;
};

UCLASS(BlueprintType, Blueprintable, meta = (MVVMAllowedContextCreationType = "Manual|CreateInstance"))
class PROJECTKC_API UKCResultViewModel : public UKCViewModelBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "KC|UI")
	const TArray<FKCResultTeamViewData>& GetTeams() const { return Teams; }

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetTeams(const TArray<FKCResultTeamViewData>& NewTeams);

	UFUNCTION(BlueprintCallable, Category = "KC|UI|Preview")
	void SetPreviewData(const TArray<FKCResultTeamViewData>& NewTeams);

private:
	UPROPERTY(BlueprintReadWrite, FieldNotify, Getter, Setter, Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	TArray<FKCResultTeamViewData> Teams;
};
