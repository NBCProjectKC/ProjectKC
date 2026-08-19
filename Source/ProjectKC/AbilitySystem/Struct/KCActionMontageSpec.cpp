#include "ProjectKC/AbilitySystem/Struct/KCActionMontageSpec.h"

#include "Animation/AnimMontage.h"

bool FKCActionMontageSpec::HasMontage() const
{
	return IsValid(Montage);
}

bool FKCActionMontageSpec::Validate(FString& OutError) const
{
	OutError.Reset();
	if (!HasMontage())
	{
		return true;
	}

	if (!FMath::IsFinite(PlayRate) || PlayRate <= 0.0f)
	{
		OutError = TEXT("PlayRate는 0보다 큰 유한한 수여야 합니다.");
		return false;
	}

	if (!StartSection.IsNone() &&
		Montage->GetSectionIndex(StartSection) == INDEX_NONE)
	{
		OutError = FString::Printf(
			TEXT("Montage '%s'에 StartSection '%s'가 없습니다."),
			*Montage->GetName(),
			*StartSection.ToString());
		return false;
	}

	return true;
}
