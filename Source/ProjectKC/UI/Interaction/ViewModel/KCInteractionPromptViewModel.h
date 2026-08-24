#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/UI/Common/Core/KCViewModelBase.h"
#include "KCInteractionPromptViewModel.generated.h"

UCLASS(BlueprintType)
class PROJECTKC_API UKCInteractionPromptViewModel : public UKCViewModelBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	bool bVisible = false;

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	FText InputText;

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	FText ActionText;

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	TObjectPtr<AActor> TargetActor;
};
