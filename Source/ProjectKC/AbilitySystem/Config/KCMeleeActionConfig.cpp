#include "ProjectKC/AbilitySystem/Config/KCMeleeActionConfig.h"

#include "CollisionQueryParams.h"

/**
 * @brief Initializes default collision settings for melee actions.
 *
 * Targets pawn object types and uses the visibility channel for obstruction tracing.
 */
UKCMeleeActionConfig::UKCMeleeActionConfig()
{
	TargetObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
	ObstructionTraceChannel =
		UEngineTypes::ConvertToTraceType(ECC_Visibility);
}

/**
 * @brief Validates the melee action configuration and reports the first validation error.
 *
 * @param OutError Receives an error message when validation fails; cleared before validation.
 * @return true if the configuration is valid, false otherwise.
 */
bool UKCMeleeActionConfig::Validate(FString& OutError) const
{
	OutError.Reset();
	if (!FMath::IsFinite(SweepDistance) || SweepDistance <= 0.0f)
	{
		OutError = TEXT("SweepDistance는 0보다 큰 유한한 수여야 합니다.");
		return false;
	}

	if (!FMath::IsFinite(SweepRadius) || SweepRadius <= 0.0f)
	{
		OutError = TEXT("SweepRadius는 0보다 큰 유한한 수여야 합니다.");
		return false;
	}

	if (!FMath::IsFinite(StartForwardOffset) || StartForwardOffset < 0.0f)
	{
		OutError = TEXT("StartForwardOffset은 0 이상의 유한한 수여야 합니다.");
		return false;
	}

	if (!FMath::IsFinite(HeightOffset))
	{
		OutError = TEXT("HeightOffset은 유한한 수여야 합니다.");
		return false;
	}

	if (MaxTargets < 1 || MaxTargets > 32)
	{
		OutError = TEXT("MaxTargets는 1 이상 32 이하여야 합니다.");
		return false;
	}

	if (TargetObjectTypes.IsEmpty())
	{
		OutError = TEXT("TargetObjectTypes가 비어 있습니다.");
		return false;
	}

	TSet<ECollisionChannel> SeenChannels;
	for (const TEnumAsByte<EObjectTypeQuery> ObjectType : TargetObjectTypes)
	{
		const ECollisionChannel Channel =
			UEngineTypes::ConvertToCollisionChannel(ObjectType.GetValue());
		if (!FCollisionObjectQueryParams::IsValidObjectQuery(Channel))
		{
			OutError = TEXT("TargetObjectTypes에 유효하지 않은 Object Type이 있습니다.");
			return false;
		}

		if (SeenChannels.Contains(Channel))
		{
			OutError = TEXT("TargetObjectTypes에 같은 Object Type이 중복됩니다.");
			return false;
		}
		SeenChannels.Add(Channel);
	}

	if (bRequireUnobstructedPath)
	{
		const ECollisionChannel Channel =
			UEngineTypes::ConvertToCollisionChannel(
				ObstructionTraceChannel.GetValue());
		if (Channel < ECC_WorldStatic || Channel >= ECC_MAX)
		{
			OutError = TEXT("ObstructionTraceChannel이 유효하지 않습니다.");
			return false;
		}
	}

	return true;
}
