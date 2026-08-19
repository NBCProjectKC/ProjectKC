#include "ProjectKC/AbilitySystem/Config/KCActionConfig.h"

/**
 * @brief Validates the action configuration.
 *
 * @param OutError Receives an error description when validation fails.
 * @return true.
 */
bool UKCActionConfig::Validate(FString& OutError) const
{
	OutError.Reset();
	return true;
}
