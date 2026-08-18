#include "Player/KCPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Player/Combat/KCPlayerCombatComponent.h"
#include "Player/Interaction/KCPlayerInteractionComponent.h"

namespace
{
	constexpr float FacingReplicationInterval = 1.0f / 30.0f;
	constexpr float MinimumFacingReplicationAngle = 0.5f;
}

AKCPlayerCharacter::AKCPlayerCharacter()
{
	bReplicates = true;
	SetReplicateMovement(true);
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);
	GetMesh()->VisibilityBasedAnimTickOption =
		EVisibilityBasedAnimTickOption::OnlyTickMontagesWhenNotRendered;
	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -96.0f));
	GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

	UCharacterMovementComponent* CharacterMovementComponent = GetCharacterMovement();
	CharacterMovementComponent->bOrientRotationToMovement = false;
	CharacterMovementComponent->bUseControllerDesiredRotation = false;
	CharacterMovementComponent->MaxWalkSpeed = 600.0f;

	CameraBoomComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoomComponent"));
	CameraBoomComponent->SetupAttachment(RootComponent);
	CameraBoomComponent->SetUsingAbsoluteRotation(true);
	CameraBoomComponent->SetRelativeRotation(FRotator(-60.0f, 0.0f, 0.0f));
	CameraBoomComponent->TargetArmLength = 1200.0f;
	CameraBoomComponent->bDoCollisionTest = false;
	CameraBoomComponent->bUsePawnControlRotation = false;

	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCameraComponent"));
	TopDownCameraComponent->SetupAttachment(CameraBoomComponent, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false;

	CombatComponent = CreateDefaultSubobject<UKCPlayerCombatComponent>(TEXT("CombatComponent"));
	InteractionComponent = CreateDefaultSubobject<UKCPlayerInteractionComponent>(
		TEXT("InteractionComponent"));
	InteractionComponent->SetupAttachment(RootComponent);
}

void AKCPlayerCharacter::TryAttack()
{
	if (CombatComponent)
	{
		CombatComponent->TryAttack();
	}
}

void AKCPlayerCharacter::RequestInteract()
{
	if (IsLocallyControlled())
	{
		if (InteractionComponent)
		{
			InteractionComponent->TryInteract();
		}

		OnInteractInputRequested.Broadcast();
	}
}

void AKCPlayerCharacter::RequestDropHeldItem()
{
	if (IsLocallyControlled())
	{
		OnDropHeldItemInputRequested.Broadcast();
	}
}

float AKCPlayerCharacter::TakeDamage(
	const float DamageAmount,
	const FDamageEvent& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	const float ActualDamage =
		Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	return CombatComponent
		? CombatComponent->HandleDamage(ActualDamage, EventInstigator, DamageCauser)
		: ActualDamage;
}

UKCPlayerCombatComponent* AKCPlayerCharacter::GetCombatComponent() const
{
	return CombatComponent;
}

UKCPlayerInteractionComponent* AKCPlayerCharacter::GetInteractionComponent() const
{
	return InteractionComponent;
}

void AKCPlayerCharacter::MoveInWorldDirection(const FVector& WorldDirection, const float ScaleValue)
{
	if (!FMath::IsNearlyZero(ScaleValue))
	{
		AddMovementInput(WorldDirection, ScaleValue);
	}
}

void AKCPlayerCharacter::UpdateFacingDirection(const FVector& WorldDirection, const float DeltaSeconds)
{
	const FVector FlatDirection = FVector(WorldDirection.X, WorldDirection.Y, 0.0f).GetSafeNormal();
	if (FlatDirection.IsNearlyZero())
	{
		return;
	}

	const float FacingYaw = FlatDirection.Rotation().Yaw;
	ApplyFacingYaw(FacingYaw);

	if (HasAuthority())
	{
		return;
	}

	FacingReplicationElapsed += DeltaSeconds;
	const float FacingDelta = FMath::Abs(FMath::FindDeltaAngleDegrees(LastSentFacingYaw, FacingYaw));
	if (FacingReplicationElapsed >= FacingReplicationInterval
		&& FacingDelta >= MinimumFacingReplicationAngle)
	{
		ServerSetFacingYaw(FacingYaw);
		LastSentFacingYaw = FacingYaw;
		FacingReplicationElapsed = 0.0f;
	}
}

void AKCPlayerCharacter::ApplyFacingYaw(const float FacingYaw)
{
	SetActorRotation(FRotator(0.0f, FMath::UnwindDegrees(FacingYaw), 0.0f));
}

void AKCPlayerCharacter::ServerSetFacingYaw_Implementation(const float FacingYaw)
{
	ApplyFacingYaw(FacingYaw);
}
