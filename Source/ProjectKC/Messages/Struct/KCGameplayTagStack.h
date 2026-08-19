/**
 * @file KCGameplayTagStack.h
 * @brief 태그와 개수(Count)를 묶어 네트워크로 동기화(FastArraySerializer)하는 구조체 정의
 */

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "KCGameplayTagStack.generated.h"

struct FKCGameplayTagStackContainer;
struct FNetDeltaSerializeInfo;

/**
 * @struct FKCGameplayTagStack
 * @brief 단일 태그 스택 (태그 + 카운트)
 */
USTRUCT(BlueprintType)
struct PROJECTKC_API FKCGameplayTagStack : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FKCGameplayTagStack() {}

	FKCGameplayTagStack(FGameplayTag InTag, int32 InStackCount)
		: Tag(InTag)
		, StackCount(InStackCount)
	{
	}

	FString GetDebugString() const;

private:
	friend FKCGameplayTagStackContainer;

	UPROPERTY()
	FGameplayTag Tag;

	UPROPERTY()
	int32 StackCount = 0;
};

/**
 * @struct FKCGameplayTagStackContainer
 * @brief 여러 태그 스택을 관리하고 네트워크로 효율적으로 동기화하는 컨테이너
 */
USTRUCT(BlueprintType)
struct PROJECTKC_API FKCGameplayTagStackContainer : public FFastArraySerializer
{
	GENERATED_BODY()

	FKCGameplayTagStackContainer() {}

public:
	/** @brief 특정 태그의 스택 수를 추가합니다. */
	void AddStack(FGameplayTag Tag, int32 StackCount);

	/** @brief 특정 태그의 스택 수를 감소시킵니다. */
	void RemoveStack(FGameplayTag Tag, int32 StackCount);

	/** @brief 특정 태그의 현재 스택 수를 반환합니다. */
	int32 GetStackCount(FGameplayTag Tag) const
	{
		return TagToCountMap.FindRef(Tag);
	}

	/** @brief 특정 태그가 하나 이상 존재하는지 여부를 반환합니다. */
	bool ContainsTag(FGameplayTag Tag) const
	{
		return TagToCountMap.Contains(Tag);
	}

	//~FFastArraySerializer contract
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);
	//~End of FFastArraySerializer contract

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FKCGameplayTagStack, FKCGameplayTagStackContainer>(Stacks, DeltaParms, *this);
	}

private:
	UPROPERTY()
	TArray<FKCGameplayTagStack> Stacks;

	TMap<FGameplayTag, int32> TagToCountMap;
};

template<>
struct TStructOpsTypeTraits<FKCGameplayTagStackContainer> : public TStructOpsTypeTraitsBase2<FKCGameplayTagStackContainer>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};
