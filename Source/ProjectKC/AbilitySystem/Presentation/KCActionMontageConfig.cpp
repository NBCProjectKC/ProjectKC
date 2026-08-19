#include "ProjectKC/AbilitySystem/Presentation/KCActionMontageConfig.h"

#include "Animation/AnimMontage.h"

bool UKCActionMontageConfig::Validate(FString& OutError) const
{
	OutError.Reset();
	if (!IsValid(Montage))
	{
		OutError = TEXT("Montage가 비어 있습니다.");
		return false;
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
