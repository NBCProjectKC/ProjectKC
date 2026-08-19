#include "ProjectKC/Player/KCTempCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "ProjectKC/AbilitySystem/Component/KCAbilitySystemComponent.h"
#include "ProjectKC/AbilitySystem/Component/KCKnockbackComponent.h"
#include "ProjectKC/Item/Component/KCHeldItemComponent.h"
#include "ProjectKC/Item/Definition/KCItemDefinition.h"
#if WITH_EDITOR
#include "UObject/UnrealType.h"
#endif

AKCTempCharacter::AKCTempCharacter()
{
	bReplicates = true;

	AbilitySystemComponent =
		CreateDefaultSubobject<UKCAbilitySystemComponent>(TEXT("AbilitySystem"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(
		EGameplayEffectReplicationMode::Mixed);

	HeldItemComponent =
		CreateDefaultSubobject<UKCHeldItemComponent>(TEXT("HeldItem"));
	KnockbackComponent =
		CreateDefaultSubobject<UKCKnockbackComponent>(TEXT("Knockback"));

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->SetRelativeRotation(FRotator(-60.0f, 0.0f, 0.0f));
	CameraBoom->TargetArmLength = 1200.0f;
	CameraBoom->bDoCollisionTest = false;

	TopDownCamera =
		CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCamera->SetupAttachment(
		CameraBoom,
		USpringArmComponent::SocketName);

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

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
}

UAbilitySystemComponent* AKCTempCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AKCTempCharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshHeldItemPreview();
}

UKCAbilitySystemComponent* AKCTempCharacter::GetKCAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UKCHeldItemComponent* AKCTempCharacter::GetHeldItemComponent() const
{
	return HeldItemComponent;
}

void AKCTempCharacter::TryInteract()
{
	if (HeldItemComponent)
	{
		HeldItemComponent->TryInteract();
	}
}

void AKCTempCharacter::TryDropHeldItem()
{
	if (HeldItemComponent)
	{
		HeldItemComponent->TryDropHeldItem();
	}
}

bool AKCTempCharacter::TryUseHeldItem()
{
	return HeldItemComponent && HeldItemComponent->UseHeldItem();
}

void AKCTempCharacter::RefreshHeldItemPreview()
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

	bool bAttached = true;
	if (HeldItemPreviewMesh->IsRegistered())
	{
		bAttached = HeldItemPreviewMesh->AttachToComponent(
			Attachment,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			HandSocketName);
	}
	else
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
void AKCTempCharacter::PostEditChangeProperty(
	FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RefreshHeldItemPreview();
}
#endif

void AKCTempCharacter::BeginPlay()
{
	Super::BeginPlay();
	InitializeAbilityActorInfo();
}

void AKCTempCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	InitializeAbilityActorInfo();
}

void AKCTempCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();
	InitializeAbilityActorInfo();
}

void AKCTempCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();
	InitializeAbilityActorInfo();
	InitializeInputMapping();
}

void AKCTempCharacter::SetupPlayerInputComponent(
	UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInput =
		Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInput)
	{
		return;
	}

	if (MoveAction)
	{
		EnhancedInput->BindAction(
			MoveAction,
			ETriggerEvent::Triggered,
			this,
			&AKCTempCharacter::HandleMoveInput);
	}
	if (InteractAction)
	{
		EnhancedInput->BindAction(
			InteractAction,
			ETriggerEvent::Started,
			this,
			&AKCTempCharacter::HandleInteractInput);
	}
	if (UseAction)
	{
		EnhancedInput->BindAction(
			UseAction,
			ETriggerEvent::Started,
			this,
			&AKCTempCharacter::HandleUseInput);
	}
	if (DropAction)
	{
		EnhancedInput->BindAction(
			DropAction,
			ETriggerEvent::Started,
			this,
			&AKCTempCharacter::HandleDropInput);
	}
}

void AKCTempCharacter::HandleMoveInput(const FInputActionValue& InputValue)
{
	const FVector2D MoveValue = InputValue.Get<FVector2D>();
	AddMovementInput(FVector::ForwardVector, MoveValue.Y);
	AddMovementInput(FVector::RightVector, MoveValue.X);
}

void AKCTempCharacter::HandleInteractInput(
	const FInputActionValue& InputValue)
{
	TryInteract();
}

void AKCTempCharacter::HandleUseInput(const FInputActionValue& InputValue)
{
	TryUseHeldItem();
}

void AKCTempCharacter::HandleDropInput(const FInputActionValue& InputValue)
{
	TryDropHeldItem();
}

void AKCTempCharacter::InitializeAbilityActorInfo()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}

void AKCTempCharacter::InitializeInputMapping()
{
	APlayerController* PlayerController =
		Cast<APlayerController>(GetController());
	if (!PlayerController || !PlayerController->IsLocalController() ||
		!DefaultMappingContext)
	{
		return;
	}

	ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer
		? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(
			LocalPlayer)
		: nullptr;
	if (InputSubsystem)
	{
		InputSubsystem->RemoveMappingContext(DefaultMappingContext);
		InputSubsystem->AddMappingContext(DefaultMappingContext, 0);
	}
}
