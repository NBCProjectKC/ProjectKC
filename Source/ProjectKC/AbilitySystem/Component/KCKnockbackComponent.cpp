#include "ProjectKC/AbilitySystem/Component/KCKnockbackComponent.h"

#include "Components/PrimitiveComponent.h"
#include "GameFramework/Character.h"

/**
 * @brief Determines whether a knockback request can be applied by the owning actor.
 *
 * The request must contain finite, nonnegative speeds and a valid nonzero velocity.
 * The owner must have authority, and non-character owners must have a physics
 * component that simulates physics.
 *
 * @param Request Knockback speeds and direction to validate.
 * @return true if the request can be applied, false otherwise.
 */
bool UKCKnockbackComponent::CanApplyKnockback(
	const FKCKnockbackRequest& Request) const
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() ||
		Request.Direction.ContainsNaN() ||
		!FMath::IsFinite(Request.HorizontalSpeed) ||
		!FMath::IsFinite(Request.VerticalSpeed) ||
		Request.HorizontalSpeed < 0.0f ||
		Request.VerticalSpeed < 0.0f ||
		(FMath::IsNearlyZero(Request.HorizontalSpeed) &&
			FMath::IsNearlyZero(Request.VerticalSpeed)))
	{
		return false;
	}

	if (!FMath::IsNearlyZero(Request.HorizontalSpeed))
	{
		FVector HorizontalDirection = Request.Direction;
		HorizontalDirection.Z = 0.0f;
		if (!HorizontalDirection.Normalize())
		{
			return false;
		}
	}

	if (Owner->IsA<ACharacter>())
	{
		return true;
	}

	const UPrimitiveComponent* PrimitiveComponent = ResolvePhysicsComponent();
	return PrimitiveComponent && PrimitiveComponent->IsSimulatingPhysics();
}

/**
 * @brief Applies the requested knockback to the owning character or physics component.
 *
 * @param Request Knockback speeds, direction, and velocity override settings.
 * @return `true` if the knockback is applied; `false` if the request is invalid or no suitable target exists.
 */
bool UKCKnockbackComponent::ApplyKnockback(
	const FKCKnockbackRequest& Request)
{
	AActor* Owner = GetOwner();
	if (!CanApplyKnockback(Request))
	{
		return false;
	}

	FVector LaunchVelocity = FVector::UpVector * Request.VerticalSpeed;
	FVector HorizontalDirection = Request.Direction;
	HorizontalDirection.Z = 0.0f;
	if (!FMath::IsNearlyZero(Request.HorizontalSpeed))
	{
		HorizontalDirection.Normalize();
		LaunchVelocity += HorizontalDirection * Request.HorizontalSpeed;
	}

	if (ACharacter* Character = Cast<ACharacter>(Owner))
	{
		Character->LaunchCharacter(
			LaunchVelocity,
			Request.bOverrideHorizontalVelocity,
			Request.bOverrideVerticalVelocity);
		return true;
	}

	if (UPrimitiveComponent* PrimitiveComponent = ResolvePhysicsComponent())
	{
		PrimitiveComponent->AddImpulse(
			LaunchVelocity,
			NAME_None,
			true);
		return true;
	}

	return false;
}

/**
 * @brief Resolves the primitive component used for physics-based knockback.
 *
 * @return UPrimitiveComponent* The configured physics component when available and compatible; otherwise, the owner's root primitive component, or nullptr if no suitable component exists.
 */
UPrimitiveComponent* UKCKnockbackComponent::ResolvePhysicsComponent() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	if (UActorComponent* ReferencedComponent =
		PhysicsComponent.GetComponent(Owner))
	{
		return Cast<UPrimitiveComponent>(ReferencedComponent);
	}

	return Cast<UPrimitiveComponent>(Owner->GetRootComponent());
}
