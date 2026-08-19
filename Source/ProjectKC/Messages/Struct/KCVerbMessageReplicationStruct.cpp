/**
 * @file KCVerbMessageReplicationStruct.cpp
 * @brief FKCVerbMessageReplicationStruct의 복제 및 로컬 재브로드캐스트 구현부
 */

#include "ProjectKC/Messages/Struct/KCVerbMessageReplicationStruct.h"
#include "GameFramework/GameplayMessageSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(KCVerbMessageReplicationStruct)

void FKCVerbMessageReplicationStruct::AddMessage(const FKCVerbMessageStruct& Message)
{
	FKCVerbMessageReplicationEntryStruct& NewEntry = CurrentMessages.Emplace_GetRef(Message);
	MarkItemDirty(NewEntry);
}

void FKCVerbMessageReplicationStruct::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
}

void FKCVerbMessageReplicationStruct::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	for (int32 Index : AddedIndices)
	{
		const FKCVerbMessageReplicationEntryStruct& Entry = CurrentMessages[Index];
		RebroadcastMessage(Entry.Message);
	}
}

void FKCVerbMessageReplicationStruct::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	for (int32 Index : ChangedIndices)
	{
		const FKCVerbMessageReplicationEntryStruct& Entry = CurrentMessages[Index];
		RebroadcastMessage(Entry.Message);
	}
}

void FKCVerbMessageReplicationStruct::RebroadcastMessage(const FKCVerbMessageStruct& Message)
{
	check(Owner);
	if (UGameplayMessageSubsystem::HasInstance(Owner))
	{
		UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(Owner);
		MessageSystem.BroadcastMessage(Message.Verb, Message);
	}
}
