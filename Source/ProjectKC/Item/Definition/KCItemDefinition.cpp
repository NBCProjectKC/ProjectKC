#include "ProjectKC/Item/Definition/KCItemDefinition.h"

#include "Misc/DataValidation.h"
#include "ProjectKC/AbilitySystem/Definition/KCChannelActionDefinition.h"
#include "ProjectKC/AbilitySystem/Definition/KCAbilityDefinition.h"
#include "ProjectKC/AbilitySystem/Targeting/KCActionTargeting.h"

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

	FString DurabilityError;
	if (!Durability.Validate(DurabilityError))
	{
		OutError = FString::Printf(
			TEXT("Durability가 유효하지 않습니다: %s"),
			*DurabilityError);
		return false;
	}

	if (Durability.IsEnabled() && !UseAction)
	{
		OutError = TEXT("내구도를 사용하는 아이템에는 UseAction이 필요합니다.");
		return false;
	}

	if (Durability.ConsumeMode ==
			EKCItemDurabilityConsumeMode::OnFirstHit &&
		(!UseAction ||
		 !UseAction->ActionTargeting ||
		 !UseAction->ActionTargeting->ProducesHitResults()))
	{
		OutError = TEXT(
			"OnFirstHit 내구도는 HitResult를 제공하는 Targeting을 사용하는 Action에만 적용할 수 있습니다.");
		return false;
	}

	if (Durability.ConsumeMode ==
			EKCItemDurabilityConsumeMode::WhileActive &&
		(!UseAction || !UseAction->IsA<UKCChannelActionDefinition>()))
	{
		OutError = TEXT(
			"WhileActive 내구도는 Channel Action에만 적용할 수 있습니다.");
		return false;
	}

	return true;
}
