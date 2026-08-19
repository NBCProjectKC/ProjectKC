#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "KCSetByCallerValueStruct.generated.h"

/** GameplayEffect의 SetByCaller 자리에 넣을 태그와 기본 수치다. */
USTRUCT(BlueprintType)
struct PROJECTKC_API FKCSetByCallerValueStruct
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Ability|Effect")
	FGameplayTag DataTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Ability|Effect")
	float Magnitude = 0.0f;

	bool Validate(FString& OutError) const;
};
