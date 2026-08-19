#include "ProjectKC/Item/Definition/KCItemDefinition.h"

#include "Misc/DataValidation.h"
#include "ProjectKC/AbilitySystem/Definition/KCAbilityDefinition.h"

#if WITH_EDITOR
/**
 * @brief Validates the item definition and reports any validation error.
 *
 * @param Context Context used to record validation errors.
 * @return The superclass validation result, or `Invalid` when item validation fails.
 */
EDataValidationResult UKCItemDefinition::IsDataValid(
	FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	FString Error;
	if (!Validate(Error))
	{
		Context.AddError(FText::FromString(Error));
		return EDataValidationResult::Invalid;
	}

	return Result == EDataValidationResult::NotValidated
		? EDataValidationResult::Valid
		: Result;
}
#endif

/**
 * @brief Determines whether the item has a valid use action configured.
 *
 * @return `true` if the use action is valid, `false` otherwise.
 */
bool UKCItemDefinition::IsUsable() const
{
	return IsValid(UseAction);
}

/**
 * @brief Validates the item definition and reports the first validation error.
 *
 * @param OutError Receives a descriptive error message when validation fails; cleared before validation.
 * @return true if the item definition is valid, false otherwise.
 */
bool UKCItemDefinition::Validate(FString& OutError) const
{
	OutError.Reset();
	if (DisplayName.IsEmptyOrWhitespace())
	{
		OutError = TEXT("DisplayName이 비어 있습니다.");
		return false;
	}

	FString PresentationError;
	if (!Presentation.Validate(PresentationError))
	{
		OutError = FString::Printf(
			TEXT("Presentation이 유효하지 않습니다: %s"),
			*PresentationError);
		return false;
	}

	if (UseAction)
	{
		FString AbilityError;
		if (!UseAction->ValidateWithActionContract(AbilityError))
		{
			OutError = FString::Printf(
				TEXT("UseAction이 유효하지 않습니다: %s"),
				*AbilityError);
			return false;
		}

		// 사용 가능한 아이템은 자기 고유의 사용 동작을 반드시 보여 준다.
		if (!UseAction->ActionMontage.HasMontage())
		{
			OutError = TEXT("사용 가능한 아이템의 UseAction에는 사용 Montage가 필요합니다.");
			return false;
		}
	}

	return true;
}
