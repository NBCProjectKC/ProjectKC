/**
 * @file KCVerbMessageStruct.cpp
 * @brief FKCVerbMessage 문자열 변환 구현부
 */

#include "ProjectKC/Messages/Struct/KCVerbMessageStruct.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(KCVerbMessageStruct)

FString FKCVerbMessageStruct::ToString() const
{
	FString HumanReadableMessage;
	FKCVerbMessageStruct::StaticStruct()->ExportText(HumanReadableMessage, this, nullptr, nullptr, PPF_None, nullptr);
	return HumanReadableMessage;
}
