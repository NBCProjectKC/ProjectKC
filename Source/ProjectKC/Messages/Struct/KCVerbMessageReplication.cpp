/**
 * @file KCVerbMessageReplication.cpp
 * @brief FKCVerbMessageReplication의 복제 및 로컬 재브로드캐스트 구현부
 */

#include "ProjectKC/Messages/Struct/KCVerbMessageReplication.h"
#include "GameFramework/GameplayMessageSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(KCVerbMessageReplication)

FString FKCVerbMessageReplicationEntry::GetDebugString() const
{
	return Message.ToString();
}

void FKCVerbMessageReplication::AddMessage(const FKCVerbMessage& Message)
{
	FKCVerbMessageReplicationEntry& NewEntry = CurrentMessages.Emplace_GetRef(Message);
	MarkItemDirty(NewEntry);
}

void FKCVerbMessageReplication::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
}

void FKCVerbMessageReplication::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	for (int32 Index : AddedIndices)
	{
		const FKCVerbMessageReplicationEntry& Entry = CurrentMessages[Index];
		RebroadcastMessage(Entry.Message);
	}
}

void FKCVerbMessageReplication::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	for (int32 Index : ChangedIndices)
	{
		const FKCVerbMessageReplicationEntry& Entry = CurrentMessages[Index];
		RebroadcastMessage(Entry.Message);
	}
}

void FKCVerbMessageReplication::RebroadcastMessage(const FKCVerbMessage& Message)
{
	check(Owner);
	if (UGameplayMessageSubsystem::HasInstance(Owner))
	{
		UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(Owner);
		MessageSystem.BroadcastMessage(Message.Verb, Message);
	}
}
