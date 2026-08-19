/**
 * @file KCVerbMessageReplication.h
 * @brief 서버의 VerbMessage를 클라이언트로 고속 복제(FastArraySerializer)하는 구조체 정의
 */

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "ProjectKC/Messages/Struct/KCVerbMessage.h"
#include "KCVerbMessageReplication.generated.h"

struct FKCVerbMessageReplication;
struct FNetDeltaSerializeInfo;

/**
 * @struct FKCVerbMessageReplicationEntry
 * @brief FastArraySerializer로 복제되는 단일 VerbMessage 엔트리
 */
USTRUCT(BlueprintType)
struct PROJECTKC_API FKCVerbMessageReplicationEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FKCVerbMessageReplicationEntry() {}

	FKCVerbMessageReplicationEntry(const FKCVerbMessage& InMessage)
		: Message(InMessage)
	{
	}

	FString GetDebugString() const;

private:
	friend FKCVerbMessageReplication;

	UPROPERTY()
	FKCVerbMessage Message;
};

/**
 * @struct FKCVerbMessageReplication
 * @brief 서버에서 발생한 VerbMessage 목록을 클라이언트로 복제하고 로컬 메시지 라우터로 재전송하는 컨테이너
 */
USTRUCT(BlueprintType)
struct PROJECTKC_API FKCVerbMessageReplication : public FFastArraySerializer
{
	GENERATED_BODY()

	FKCVerbMessageReplication() {}

public:
	/** @brief 소유자 액터/오브젝트 설정 (월드 및 GameplayMessageSubsystem 접근용) */
	void SetOwner(UObject* InOwner) { Owner = InOwner; }

	/** @brief 서버에서 새 메시지를 추가하여 클라이언트로 복제 시작 */
	void AddMessage(const FKCVerbMessage& Message);

	//~FFastArraySerializer contract
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);
	//~End of FFastArraySerializer contract

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FKCVerbMessageReplicationEntry, FKCVerbMessageReplication>(CurrentMessages, DeltaParms, *this);
	}

private:
	/** @brief 클라이언트에서 수신된 메시지를 로컬 GameplayMessageSubsystem으로 재브로드캐스트 */
	void RebroadcastMessage(const FKCVerbMessage& Message);

private:
	UPROPERTY()
	TArray<FKCVerbMessageReplicationEntry> CurrentMessages;

	UPROPERTY()
	TObjectPtr<UObject> Owner = nullptr;
};

template<>
struct TStructOpsTypeTraits<FKCVerbMessageReplication> : public TStructOpsTypeTraitsBase2<FKCVerbMessageReplication>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};
