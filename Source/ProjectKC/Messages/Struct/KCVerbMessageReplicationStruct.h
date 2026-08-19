/**
 * @file KCVerbMessageReplicationStruct.h
 * @brief 서버에서 발생한 VerbMessage 목록을 클라이언트로 복제하고 로컬 메시지 라우터로 재전송하는 컨테이너 구조체 정의
 */

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "ProjectKC/Messages/Struct/KCVerbMessageStruct.h"
#include "ProjectKC/Messages/Struct/KCVerbMessageReplicationEntryStruct.h"
#include "KCVerbMessageReplicationStruct.generated.h"

struct FNetDeltaSerializeInfo;

/**
 * @struct FKCVerbMessageReplicationStruct
 * @brief 서버에서 발생한 VerbMessage 목록을 클라이언트로 복제하고 로컬 메시지 라우터로 재전송하는 컨테이너
 */
USTRUCT(BlueprintType)
struct PROJECTKC_API FKCVerbMessageReplicationStruct : public FFastArraySerializer
{
	GENERATED_BODY()

	FKCVerbMessageReplicationStruct()
	{
	}

public:
	/** @brief 소유자 액터/오브젝트 설정 (월드 및 GameplayMessageSubsystem 접근용) */
	void SetOwner(UObject* InOwner)
	{
		Owner = InOwner;
	}

	/** @brief 서버에서 새 메시지를 추가하여 클라이언트로 복제 시작 */
	void AddMessage(const FKCVerbMessageStruct& Message);

	//~FFastArraySerializer contract
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);
	//~End of FFastArraySerializer contract

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FKCVerbMessageReplicationEntryStruct, FKCVerbMessageReplicationStruct>(CurrentMessages, DeltaParms, *this);
	}

private:
	/** @brief 클라이언트에서 수신된 메시지를 로컬 GameplayMessageSubsystem으로 재브로드캐스트 */
	void RebroadcastMessage(const FKCVerbMessageStruct& Message);

private:
	UPROPERTY()
	TArray<FKCVerbMessageReplicationEntryStruct> CurrentMessages;

	UPROPERTY()
	TObjectPtr<UObject> Owner = nullptr;
};

template<>
struct TStructOpsTypeTraits<FKCVerbMessageReplicationStruct> : public TStructOpsTypeTraitsBase2<FKCVerbMessageReplicationStruct>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};
