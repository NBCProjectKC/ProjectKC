#include "ProjectKC/AbilitySystem/Fragment/KCKnockbackFragment.h"

#include "ProjectKC/AbilitySystem/Component/KCKnockbackComponent.h"
#include "ProjectKC/AbilitySystem/Fragment/KCActionExecutionContext.h"
#include "GameFramework/Actor.h"

/**
 * @brief Validates the configured horizontal and vertical knockback speeds.
 *
 * @param OutError Receives a localized error message when validation fails.
 * @return true if both speeds are finite, nonnegative, and at least one is nonzero; false otherwise.
 */
bool UKCKnockbackFragment::Validate(FString& OutError) const
{
	OutError.Reset();
	if (!FMath::IsFinite(HorizontalSpeed) || HorizontalSpeed < 0.0f)
	{
		OutError = TEXT("HorizontalSpeed는 0 이상의 유한한 수여야 합니다.");
		return false;
	}

	if (!FMath::IsFinite(VerticalSpeed) || VerticalSpeed < 0.0f)
	{
		OutError = TEXT("VerticalSpeed는 0 이상의 유한한 수여야 합니다.");
		return false;
	}

	if (FMath::IsNearlyZero(HorizontalSpeed) &&
		FMath::IsNearlyZero(VerticalSpeed))
	{
		OutError = TEXT("넉백의 수평·수직 속도가 모두 0입니다.");
		return false;
	}

	return true;
}

/**
 * @brief Determines whether the configured knockback can be applied to the target.
 *
 * @param Context Execution context containing the authority and target information.
 * @param OutError Receives an error message when execution cannot proceed.
 * @return true if the target can receive the knockback, false otherwise.
 */
bool UKCKnockbackFragment::CanExecute(
	const FKCActionExecutionContext& Context,
	FString& OutError) const
{
	OutError.Reset();
	if (!Context.IsAuthoritative() || !IsValid(Context.TargetActor))
	{
		OutError = TEXT("넉백을 적용할 권한 또는 Target Actor가 없습니다.");
		return false;
	}

	UKCKnockbackComponent* KnockbackComponent =
		Context.TargetActor->FindComponentByClass<UKCKnockbackComponent>();
	FKCKnockbackRequest Request;
	if (!KnockbackComponent || !BuildRequest(Context, Request) ||
		!KnockbackComponent->CanApplyKnockback(Request))
	{
		OutError = TEXT("Target이 현재 넉백 요청을 처리할 수 없습니다.");
		return false;
	}

	return true;
}

/**
 * @brief Applies the configured knockback to the target actor.
 *
 * @param Context Execution context containing the target actor and authority state.
 * @return true if the knockback request is built and applied successfully, false otherwise.
 */
bool UKCKnockbackFragment::Execute(
	const FKCActionExecutionContext& Context) const
{
	if (!Context.IsAuthoritative() || !IsValid(Context.TargetActor))
	{
		return false;
	}

	UKCKnockbackComponent* KnockbackComponent =
		Context.TargetActor->FindComponentByClass<UKCKnockbackComponent>();
	if (!KnockbackComponent)
	{
		return false;
	}

	FKCKnockbackRequest Request;
	return BuildRequest(Context, Request) &&
		KnockbackComponent->ApplyKnockback(Request);
}

/**
 * @brief Builds a knockback request from the execution context and configured speeds.
 *
 * @param Context Execution context used to resolve the knockback direction.
 * @param OutRequest Request populated with the resolved direction, speeds, and velocity override settings.
 * @return true if the request is populated successfully, false if the resolved direction is nearly zero.
 */
bool UKCKnockbackFragment::BuildRequest(
	const FKCActionExecutionContext& Context,
	FKCKnockbackRequest& OutRequest) const
{
	const FVector Direction = ResolveDirection(Context);
	if (Direction.IsNearlyZero())
	{
		return false;
	}

	OutRequest.Direction = Direction;
	OutRequest.HorizontalSpeed = HorizontalSpeed;
	OutRequest.VerticalSpeed = VerticalSpeed;
	OutRequest.bOverrideHorizontalVelocity = bOverrideHorizontalVelocity;
	OutRequest.bOverrideVerticalVelocity = bOverrideVerticalVelocity;
	return true;
}

/**
 * @brief Resolves a normalized horizontal knockback direction from the configured direction mode.
 *
 * Falls back to the source actor's horizontal forward direction when the configured direction
 * cannot be normalized.
 *
 * @param Context Execution context containing the source actor, target actor, and hit result.
 * @return FVector Normalized horizontal direction, or a zero vector when no valid direction is available.
 */
FVector UKCKnockbackFragment::ResolveDirection(
	const FKCActionExecutionContext& Context) const
{
	FVector Direction = FVector::ZeroVector;
	switch (DirectionMode)
	{
	case EKCKnockbackDirectionMode::SourceForward:
		if (Context.SourceActor)
		{
			Direction = Context.SourceActor->GetActorForwardVector();
		}
		break;

	case EKCKnockbackDirectionMode::InverseHitNormal:
		if (Context.bHasHitResult)
		{
			Direction = -Context.HitResult.ImpactNormal;
		}
		break;

	case EKCKnockbackDirectionMode::SourceToTarget:
	default:
		if (Context.SourceActor && Context.TargetActor)
		{
			Direction = Context.TargetActor->GetActorLocation() -
				Context.SourceActor->GetActorLocation();
		}
		break;
	}

	Direction.Z = 0.0f;
	if (!Direction.Normalize() && Context.SourceActor)
	{
		Direction = Context.SourceActor->GetActorForwardVector();
		Direction.Z = 0.0f;
		Direction.Normalize();
	}
	return Direction;
}
