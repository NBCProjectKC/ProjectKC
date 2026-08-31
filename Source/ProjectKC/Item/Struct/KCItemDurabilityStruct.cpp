#include "ProjectKC/Item/Struct/KCItemDurabilityStruct.h"

bool FKCItemDurabilityStruct::IsEnabled() const
{
	return ConsumeMode != EKCItemDurabilityConsumeMode::None;
}

bool FKCItemDurabilityStruct::Validate(FString& OutError) const
{
	OutError.Reset();
	if (!IsEnabled())
	{
		return true;
	}

	if (!FMath::IsFinite(ConsumeAmount) || ConsumeAmount <= 0.0f ||
		ConsumeAmount > MaximumDurability)
	{
		OutError = FString::Printf(
			TEXT("ConsumeAmount는 0보다 크고 %.0f 이하여야 합니다."),
			MaximumDurability);
		return false;
	}

	return true;
}
