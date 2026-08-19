#include "ProjectKC/AbilitySystem/Struct/KCActionMontageSpec.h"

#include "Animation/AnimMontage.h"

/**
 * @brief Determines whether an animation montage is assigned and valid.
 *
 * @return `true` if the montage reference is valid, `false` otherwise.
 */
bool FKCActionMontageSpec::HasMontage() const
{
	return IsValid(Montage);
}

/**
 * @brief Validates the montage specification.
 *
 * Clears the output error and succeeds when no montage is assigned. When a
 * montage is assigned, validates the play rate and verifies that the start
 * section exists.
 *
 * @param OutError Receives an error message when validation fails.
 * @return true if the specification is valid, false otherwise.
 */
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
