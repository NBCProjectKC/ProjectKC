/**
 * @file KCVerbMessageReplicationEntryStruct.h
 * @brief FastArraySerializer로 복제되는 단일 VerbMessage 엔트리 구조체 정의
 */

#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "ProjectKC/Messages/Struct/KCVerbMessageStruct.h"
#include "KCVerbMessageReplicationEntryStruct.generated.h"

struct FKCVerbMessageReplicationStruct;

/**
 * @struct FKCVerbMessageReplicationEntryStruct
 * @brief FastArraySerializer로 복제되는 단일 VerbMessage 엔트리
 */
USTRUCT(BlueprintType)
struct PROJECTKC_API FKCVerbMessageReplicationEntryStruct : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FKCVerbMessageReplicationEntryStruct()
	{
	}

	FKCVerbMessageReplicationEntryStruct(const FKCVerbMessageStruct& InMessage)
		: Message(InMessage)
	{
	}

	FString GetDebugString() const;

private:
	friend FKCVerbMessageReplicationStruct;

	UPROPERTY()
	FKCVerbMessageStruct Message;
};
