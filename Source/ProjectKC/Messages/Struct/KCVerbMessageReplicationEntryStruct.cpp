/**
 * @file KCVerbMessageReplicationEntryStruct.cpp
 * @brief FKCVerbMessageReplicationEntryStruct 구현부
 */

#include "ProjectKC/Messages/Struct/KCVerbMessageReplicationEntryStruct.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(KCVerbMessageReplicationEntryStruct)

FString FKCVerbMessageReplicationEntryStruct::GetDebugString() const
{
	return Message.ToString();
}
