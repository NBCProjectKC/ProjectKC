#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "KCGameplayTagBlueprintLibrary.generated.h"

/** 등록된 Gameplay Tag를 이름으로 안전하게 조회하는 공용 Blueprint/Python 진입점이다. */
UCLASS()
class PROJECTKC_API UKCGameplayTagBlueprintLibrary
	: public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** 등록되지 않은 이름이면 ensure나 오류 로그 없이 Invalid Tag를 반환한다. */
	UFUNCTION(
		BlueprintPure,
		Category = "KC|GameplayTag",
		meta = (BlueprintThreadSafe))
	static FGameplayTag RequestRegisteredGameplayTag(FName TagName);
};
