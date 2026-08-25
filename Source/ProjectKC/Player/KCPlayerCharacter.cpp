#include "Player/KCPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "ProjectKC/AbilitySystem/Component/KCAbilitySystemComponent.h"
#include "ProjectKC/AbilitySystem/Component/KCKnockbackComponent.h"
#include "ProjectKC/Item/Component/KCHeldItemComponent.h"
#include "ProjectKC/Item/Definition/KCItemDefinition.h"
#include "Player/Interaction/KCPlayerInteractionComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float FacingReplicationInterval = 1.0f / 30.0f;
	constexpr float MinimumFacingReplicationAngle = 0.5f;

	void ConfigureAvatarPart(UStaticMeshComponent* Component, UStaticMesh* Mesh)
	{
		if (!Component)
		{
			return;
		}

		Component->SetStaticMesh(Mesh);
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Component->SetGenerateOverlapEvents(false);
		Component->SetCanEverAffectNavigation(false);
		Component->SetReceivesDecals(false);
	}
}

AKCPlayerCharacter::AKCPlayerCharacter()
{
	bReplicates = true;
	SetReplicateMovement(true);
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);
	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -96.0f));
	GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshFinder(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	UStaticMesh* SphereMesh = SphereMeshFinder.Succeeded()
		? SphereMeshFinder.Object
		: nullptr;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PillBodyMeshFinder(
		TEXT("/Game/KC/Player/Avatar/SM_PillBody.SM_PillBody"));
	UStaticMesh* PillBodyMesh = SphereMesh;
	if (PillBodyMeshFinder.Succeeded())
	{
		PillBodyMesh = PillBodyMeshFinder.Object;
	}

	AvatarBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AvatarBody"));
	AvatarBody->SetupAttachment(GetCapsuleComponent());
	AvatarBody->SetRelativeScale3D(FVector(1.0f, 1.0f, 0.86f));
	ConfigureAvatarPart(AvatarBody, PillBodyMesh);

	AvatarHandLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AvatarHandLeft"));
	AvatarHandLeft->SetupAttachment(GetMesh(), TEXT("hand_l"));
	AvatarHandLeft->SetRelativeLocation(FVector(0.0f, 30.0f, 0.0f));
	AvatarHandLeft->SetRelativeScale3D(FVector(0.26f));
	ConfigureAvatarPart(AvatarHandLeft, SphereMesh);

	AvatarHandRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AvatarHandRight"));
	AvatarHandRight->SetupAttachment(GetMesh(), TEXT("hand_r"));
	AvatarHandRight->SetRelativeLocation(FVector(0.0f, -25.0f, 0.0f));
	AvatarHandRight->SetRelativeScale3D(FVector(0.26f));
	ConfigureAvatarPart(AvatarHandRight, SphereMesh);

	AvatarFootLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AvatarFootLeft"));
	AvatarFootLeft->SetupAttachment(GetMesh(), TEXT("foot_l"));
	AvatarFootLeft->SetRelativeLocation(FVector(0.0f, -12.0f, -14.0f));
	AvatarFootLeft->SetRelativeScale3D(FVector(0.30f));
	ConfigureAvatarPart(AvatarFootLeft, SphereMesh);

	AvatarFootRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AvatarFootRight"));
	AvatarFootRight->SetupAttachment(GetMesh(), TEXT("foot_r"));
	AvatarFootRight->SetRelativeLocation(FVector(0.0f, -28.0f, -2.0f));
	AvatarFootRight->SetRelativeScale3D(FVector(0.30f));
	ConfigureAvatarPart(AvatarFootRight, SphereMesh);

	FaceAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("FaceAnchor"));
	FaceAnchor->SetupAttachment(AvatarBody);
	FaceAnchor->SetRelativeLocation(FVector(49.0f, 0.0f, 25.0f));

	ConfigureDriverMesh();

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

UAbilitySystemComponent* AKCPlayerCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AKCPlayerCharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ConfigureDriverMesh();
	RefreshHeldItemPreview();
}

void AKCPlayerCharacter::ConfigureDriverMesh()
{
	USkeletalMeshComponent* DriverMesh = GetMesh();
	if (!DriverMesh)
	{
		return;
	}

	DriverMesh->VisibilityBasedAnimTickOption =
		EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	DriverMesh->SetVisibility(false, false);
	DriverMesh->SetHiddenInGame(true, false);
	DriverMesh->SetCastShadow(false);
}

bool AKCPlayerCharacter::BeginUseHeldItem()
{
	return IsLocallyControlled() && HeldItemComponent &&
		HeldItemComponent->PressHeldItemUse();
}

void AKCPlayerCharacter::EndUseHeldItem()
{
	if (IsLocallyControlled() && HeldItemComponent)
	{
		HeldItemComponent->ReleaseHeldItemUse();
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
	}
}

void AKCPlayerCharacter::RequestDropHeldItem()
{
	if (IsLocallyControlled() && HeldItemComponent)
	{
		HeldItemComponent->TryDropHeldItem();
	}
}

UKCAbilitySystemComponent* AKCPlayerCharacter::GetKCAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UKCHeldItemComponent* AKCPlayerCharacter::GetHeldItemComponent() const
{
	return HeldItemComponent;
}

UKCKnockbackComponent* AKCPlayerCharacter::GetKnockbackComponent() const
{
	return KnockbackComponent;
}

UKCPlayerInteractionComponent* AKCPlayerCharacter::GetInteractionComponent() const
{
	return InteractionComponent;
}

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
void AKCPlayerCharacter::PostEditChangeProperty(
	FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RefreshHeldItemPreview();
}
#endif

void AKCPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	InitializeAbilityActorInfo();
}

void AKCPlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	InitializeAbilityActorInfo();
}

void AKCPlayerCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();
	InitializeAbilityActorInfo();
}

void AKCPlayerCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();
	InitializeAbilityActorInfo();
}

void AKCPlayerCharacter::InitializeAbilityActorInfo()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
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
