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

UCLASS(BlueprintType)
class PROJECTKC_API UKCResultViewModel : public UKCViewModelBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	TArray<FKCResultTeamViewData> Teams;
};
