#include "ProjectKC/Item/Definition/KCItemDefinition.h"

#include "Misc/DataValidation.h"
#include "ProjectKC/AbilitySystem/Definition/KCAbilityDefinition.h"

#if WITH_EDITOR
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

bool UKCItemDefinition::IsUsable() const
{
	return IsValid(UseAction);
}

bool UKCItemDefinition::Validate(FString& OutError) const
{
	OutError.Reset();
	if (!ItemId.IsValid())
	{
		OutError = TEXT("ItemId가 비어 있습니다.");
		return false;
	}

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
	}

	return true;
}
