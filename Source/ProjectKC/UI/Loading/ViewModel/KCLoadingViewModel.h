#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/UI/Common/Core/KCViewModelBase.h"
#include "KCLoadingViewModel.generated.h"

UCLASS(BlueprintType)
class PROJECTKC_API UKCLoadingViewModel : public UKCViewModelBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	float Progress = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	FText LoadingText;

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	FText TipText;
};
