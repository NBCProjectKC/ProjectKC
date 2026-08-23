#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/UI/Common/Core/KCViewModelBase.h"
#include "KCPlayerStatusViewModel.generated.h"

UCLASS(BlueprintType)
class PROJECTKC_API UKCPlayerStatusViewModel : public UKCViewModelBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	FText DisplayName;

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	float HealthRatio = 1.0f;

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	bool bDowned = false;

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	FText HeldItemName;
};
