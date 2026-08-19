#include "Player/KCPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "ProjectKC/AbilitySystem/Component/KCAbilitySystemComponent.h"
#include "ProjectKC/AbilitySystem/Component/KCKnockbackComponent.h"
#include "ProjectKC/Item/Component/KCHeldItemComponent.h"
#include "ProjectKC/Item/Definition/KCItemDefinition.h"
#include "Player/Interaction/KCPlayerInteractionComponent.h"

namespace
{
	constexpr float FacingReplicationInterval = 1.0f / 30.0f;
	constexpr float MinimumFacingReplicationAngle = 0.5f;
}

/**
 * @brief Initializes the replicated player character and its gameplay, movement, camera, interaction, and editor preview components.
 */
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

	AbilitySystemComponent =
		CreateDefaultSubobject<UKCAbilitySystemComponent>(TEXT("AbilitySystem"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	HeldItemComponent =
		CreateDefaultSubobject<UKCHeldItemComponent>(TEXT("HeldItem"));
	KnockbackComponent =
		CreateDefaultSubobject<UKCKnockbackComponent>(TEXT("Knockback"));

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

	InteractionComponent = CreateDefaultSubobject<UKCPlayerInteractionComponent>(
		TEXT("InteractionComponent"));
	InteractionComponent->SetupAttachment(RootComponent);

#if WITH_EDITORONLY_DATA
	HeldItemPreviewMesh =
		CreateEditorOnlyDefaultSubobject<UStaticMeshComponent>(
			TEXT("HeldItemPreview"));
	if (HeldItemPreviewMesh)
	{
		HeldItemPreviewMesh->SetupAttachment(
			GetMesh(),
			HeldItemComponent->GetHandSocketName());
		HeldItemPreviewMesh->SetMobility(EComponentMobility::Movable);
		HeldItemPreviewMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		HeldItemPreviewMesh->SetGenerateOverlapEvents(false);
		HeldItemPreviewMesh->SetSimulatePhysics(false);
		HeldItemPreviewMesh->SetCanEverAffectNavigation(false);
		HeldItemPreviewMesh->SetHiddenInGame(true);
		HeldItemPreviewMesh->SetVisibility(false);
	}
#endif
}

/**
 * @brief Retrieves the character's ability system component.
 *
 * @return UAbilitySystemComponent* The character's ability system component.
 */
UAbilitySystemComponent* AKCPlayerCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

/**
 * @brief Refreshes the editor held-item preview after construction.
 *
 * @param Transform Transform supplied for the construction update.
 */
void AKCPlayerCharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshHeldItemPreview();
}

/**
 * @brief Attempts to use the currently held item.
 *
 * @return true if the held item is used successfully, false otherwise.
 */
bool AKCPlayerCharacter::TryUseHeldItem()
{
	return HeldItemComponent && HeldItemComponent->UseHeldItem();
}

/**
 * @brief Requests an interaction for the locally controlled character.
 */
void AKCPlayerCharacter::RequestInteract()
{
	if (IsLocallyControlled())
	{
		if (InteractionComponent)
		{
			InteractionComponent->TryInteract();
		}
	}
}

/**
 * @brief Requests that the locally controlled character drop its held item.
 */
void AKCPlayerCharacter::RequestDropHeldItem()
{
	if (IsLocallyControlled() && HeldItemComponent)
	{
		HeldItemComponent->TryDropHeldItem();
	}
}

/**
 * @brief Gets the character's ability system component.
 *
 * @return UKCAbilitySystemComponent* The character's ability system component.
 */
UKCAbilitySystemComponent* AKCPlayerCharacter::GetKCAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

/**
 * @brief Retrieves the held-item component.
 *
 * @return UKCHeldItemComponent* The character's held-item component.
 */
UKCHeldItemComponent* AKCPlayerCharacter::GetHeldItemComponent() const
{
	return HeldItemComponent;
}

/**
 * @brief Retrieves the character's knockback component.
 *
 * @return UKCKnockbackComponent* The knockback component.
 */
UKCKnockbackComponent* AKCPlayerCharacter::GetKnockbackComponent() const
{
	return KnockbackComponent;
}

/**
 * @brief Retrieves the player interaction component.
 *
 * @return UKCPlayerInteractionComponent* The interaction component.
 */
UKCPlayerInteractionComponent* AKCPlayerCharacter::GetInteractionComponent() const
{
	return InteractionComponent;
}

/**
 * @brief Refreshes the editor-only held-item preview mesh using the configured item and hand attachment.
 */
void AKCPlayerCharacter::RefreshHeldItemPreview()
{
#if WITH_EDITORONLY_DATA
	if (!HeldItemPreviewMesh)
	{
		return;
	}

	HeldItemPreviewMesh->SetVisibility(false);
	HeldItemPreviewMesh->SetStaticMesh(nullptr);
	HeldItemPreviewMesh->SetRelativeTransform(FTransform::Identity);

	USceneComponent* Attachment = HeldItemComponent
		? HeldItemComponent->GetAttachmentComponent()
		: nullptr;
	const FName HandSocketName = HeldItemComponent
		? HeldItemComponent->GetHandSocketName()
		: NAME_None;
	if (!PreviewItemDefinition || !Attachment || HandSocketName.IsNone() ||
		!Attachment->DoesSocketExist(HandSocketName))
	{
		return;
	}

	FTransform AlignmentTransform;
	if (!PreviewItemDefinition->Presentation.TryGetGripAlignmentTransform(
		AlignmentTransform))
	{
		return;
	}

	const bool bAttached = HeldItemPreviewMesh->IsRegistered()
		? HeldItemPreviewMesh->AttachToComponent(
			Attachment,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			HandSocketName)
		: true;
	if (!HeldItemPreviewMesh->IsRegistered())
	{
		HeldItemPreviewMesh->SetupAttachment(Attachment, HandSocketName);
	}
	if (!bAttached)
	{
		return;
	}

	HeldItemPreviewMesh->SetStaticMesh(
		PreviewItemDefinition->Presentation.StaticMesh);
	HeldItemPreviewMesh->SetRelativeTransform(AlignmentTransform);
	HeldItemPreviewMesh->SetVisibility(true);
#endif
}

#if WITH_EDITOR
/**
 * @brief Refreshes the held-item preview after an editor property changes.
 */
void AKCPlayerCharacter::PostEditChangeProperty(
	FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RefreshHeldItemPreview();
}
#endif

/**
 * @brief Initializes the character's ability actor information when play begins.
 */
void AKCPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	InitializeAbilityActorInfo();
}

/**
 * @brief Initializes ability actor information after the character is possessed.
 *
 * @param NewController Controller that possesses the character.
 */
void AKCPlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	InitializeAbilityActorInfo();
}

/**
 * @brief Initializes ability actor information after the controller is replicated.
 */
void AKCPlayerCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();
	InitializeAbilityActorInfo();
}

/**
 * @brief Reinitializes ability actor information after the client restarts this pawn.
 */
void AKCPlayerCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();
	InitializeAbilityActorInfo();
}

/**
 * @brief Initializes the ability system actor information for this character.
 */
void AKCPlayerCharacter::InitializeAbilityActorInfo()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}

/**
 * @brief Adds movement input in the specified world direction.
 *
 * @param WorldDirection Direction in world space.
 * @param ScaleValue Movement input scale.
 */
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
