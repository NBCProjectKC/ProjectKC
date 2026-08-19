/**
 * @file KCGameplayTagStackStruct.cpp
 * @brief FKCGameplayTagStackStruct 구현부
 */

#include "ProjectKC/Messages/Struct/KCGameplayTagStackStruct.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(KCGameplayTagStackStruct)

FString FKCGameplayTagStackStruct::GetDebugString() const
{
	return FString::Printf(TEXT("%sx%d"), *Tag.ToString(), StackCount);
}

