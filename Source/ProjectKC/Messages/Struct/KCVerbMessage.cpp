/**
 * @file KCVerbMessage.cpp
 * @brief FKCVerbMessage 문자열 변환 구현부
 */

#include "ProjectKC/Messages/Struct/KCVerbMessage.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(KCVerbMessage)

FString FKCVerbMessage::ToString() const
{
	FString HumanReadableMessage;
	FKCVerbMessage::StaticStruct()->ExportText(HumanReadableMessage, this, nullptr, nullptr, PPF_None, nullptr);
	return HumanReadableMessage;
}
