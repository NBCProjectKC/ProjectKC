#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "KCViewModelBase.generated.h"

UCLASS(Abstract, BlueprintType, Blueprintable, meta = (MVVMAllowedContextCreationType = "Manual|CreateInstance"))
class PROJECTKC_API UKCViewModelBase : public UMVVMViewModelBase
{
	GENERATED_BODY()
};
