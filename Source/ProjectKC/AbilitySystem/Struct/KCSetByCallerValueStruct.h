#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "KCSetByCallerValueStruct.generated.h"

/** GameplayEffect의 SetByCaller 자리에 넣을 태그와 기본 수치다. */
USTRUCT(BlueprintType)
struct /**
 * Validates the SetByCaller gameplay tag and magnitude values.
 * @param OutError Receives a description of the validation failure.
 * @returns `true` if the values are valid, `false` otherwise.
 */
PROJECTKC_API FKCSetByCallerValueStruct
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Ability|Effect")
	FGameplayTag DataTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Ability|Effect")
	float Magnitude = 0.0f;

	bool Validate(FString& OutError) const;
};
