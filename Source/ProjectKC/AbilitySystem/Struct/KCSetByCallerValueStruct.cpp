#include "ProjectKC/AbilitySystem/Struct/KCSetByCallerValueStruct.h"

/**
 * @brief Validates the set-by-caller data.
 *
 * @param OutError Receives an error message when validation fails; cleared before validation.
 * @return true if the data tag is valid and the magnitude is finite, false otherwise.
 */
bool FKCSetByCallerValueStruct::Validate(FString& OutError) const
{
	OutError.Reset();
	if (!DataTag.IsValid())
	{
		OutError = TEXT("SetByCaller DataTag가 비어 있습니다.");
		return false;
	}

	if (!FMath::IsFinite(Magnitude))
	{
		OutError = FString::Printf(
			TEXT("SetByCaller '%s'의 Magnitude가 유한한 수가 아닙니다."),
			*DataTag.ToString());
		return false;
	}

	return true;
}
