#include "ProjectKC/AbilitySystem/Struct/KCActionHookStruct.h"

/**
 * @brief Validates the action hook and its fragments, reporting the first validation failure.
 *
 * @param OutError Receives the validation error message, or is cleared when validation succeeds.
 * @return true if the hook tag and all fragments are valid, false otherwise.
 */
bool FKCActionHookStruct::Validate(FString& OutError) const
{
	OutError.Reset();
	if (!HookTag.IsValid())
	{
		OutError = TEXT("HookTag가 비어 있습니다.");
		return false;
	}

	if (Fragments.IsEmpty())
	{
		OutError = FString::Printf(
			TEXT("Action Hook '%s'에 Fragment가 없습니다."),
			*HookTag.ToString());
		return false;
	}

	for (int32 Index = 0; Index < Fragments.Num(); ++Index)
	{
		const UKCActionFragment* Fragment = Fragments[Index];
		if (!IsValid(Fragment))
		{
			OutError = FString::Printf(
				TEXT("Action Hook '%s'의 Fragments[%d]가 비어 있습니다."),
				*HookTag.ToString(),
				Index);
			return false;
		}

		FString FragmentError;
		if (!Fragment->Validate(FragmentError))
		{
			OutError = FString::Printf(
				TEXT("Action Hook '%s'의 Fragments[%d] '%s'가 유효하지 않습니다: %s"),
				*HookTag.ToString(),
				Index,
				*GetNameSafe(Fragment),
				*FragmentError);
			return false;
		}
	}

	return true;
}

/**
 * @brief Determines whether any valid fragment declares the specified data tag.
 *
 * @param DataTag Data tag to search for.
 * @return true if a valid fragment declares the data tag, false otherwise.
 */
bool FKCActionHookStruct::DeclaresSetByCallerTag(FGameplayTag DataTag) const
{
	return Fragments.ContainsByPredicate(
		[DataTag](const UKCActionFragment* Fragment)
		{
			return IsValid(Fragment) &&
				Fragment->DeclaresSetByCallerTag(DataTag);
		});
}
