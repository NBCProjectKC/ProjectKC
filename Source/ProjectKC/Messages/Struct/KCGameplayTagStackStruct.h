/**
 * @file KCGameplayTagStackStruct.h
 * @brief 단일 태그 스택 (태그 + 카운트) 구조체 정의
 */

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "KCGameplayTagStackStruct.generated.h"

struct FKCGameplayTagStackContainerStruct;

/**
 * @struct FKCGameplayTagStackStruct
 * @brief 단일 태그 스택 (태그 + 카운트)
 */
USTRUCT(BlueprintType)
struct PROJECTKC_API FKCGameplayTagStackStruct : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FKCGameplayTagStackStruct()
	{
	}

	FKCGameplayTagStackStruct(FGameplayTag InTag, int32 InStackCount)
		: Tag(InTag)
		, StackCount(InStackCount)
	{
	}

	FString GetDebugString() const;

private:
	friend FKCGameplayTagStackContainerStruct;

	UPROPERTY()
	FGameplayTag Tag;

	UPROPERTY()
	int32 StackCount = 0;
};

