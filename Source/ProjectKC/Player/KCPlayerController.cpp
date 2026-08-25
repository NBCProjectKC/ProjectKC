#include "Player/KCPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Player/KCPlayerCharacter.h"

AKCPlayerController::AKCPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;
}

void AKCPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
			LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (PlayerMappingContext)
			{
				InputSubsystem->AddMappingContext(PlayerMappingContext, 0);
			}
		}
	}

}

void AKCPlayerController::BeginUseHeldItem(const FInputActionValue& InputValue)
{
	if (!InputValue.Get<bool>())
	{
		return;
	}

	if (AKCPlayerCharacter* PlayerCharacter = Cast<AKCPlayerCharacter>(GetPawn()))
	{
		PlayerCharacter->BeginUseHeldItem();
	}
}

void AKCPlayerController::EndUseHeldItem(const FInputActionValue& InputValue)
{
	if (AKCPlayerCharacter* PlayerCharacter = Cast<AKCPlayerCharacter>(GetPawn()))
	{
		PlayerCharacter->EndUseHeldItem();
	}
}

void AKCPlayerController::Interact(const FInputActionValue& InputValue)
{
	if (!InputValue.Get<bool>())
	{
		return;
	}

	if (AKCPlayerCharacter* PlayerCharacter = Cast<AKCPlayerCharacter>(GetPawn()))
	{
		PlayerCharacter->RequestInteract();
	}
}

void AKCPlayerController::DropHeldItem(const FInputActionValue& InputValue)
{
	if (!InputValue.Get<bool>())
	{
		return;
	}

	if (AKCPlayerCharacter* PlayerCharacter = Cast<AKCPlayerCharacter>(GetPawn()))
	{
		PlayerCharacter->RequestDropHeldItem();
	}
}

void AKCPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent);
	if (MoveAction)
	{
		EnhancedInputComponent->BindAction(
			MoveAction, ETriggerEvent::Triggered, this, &AKCPlayerController::Move);
	}

	if (AttackAction)
	{
		EnhancedInputComponent->BindAction(
			AttackAction,
			ETriggerEvent::Started,
			this,
			&AKCPlayerController::BeginUseHeldItem);
		EnhancedInputComponent->BindAction(
			AttackAction,
			ETriggerEvent::Completed,
			this,
			&AKCPlayerController::EndUseHeldItem);
		EnhancedInputComponent->BindAction(
			AttackAction,
			ETriggerEvent::Canceled,
			this,
			&AKCPlayerController::EndUseHeldItem);
	}

	if (InteractAction)
	{
		EnhancedInputComponent->BindAction(
			InteractAction, ETriggerEvent::Started, this, &AKCPlayerController::Interact);
	}

	if (DropHeldItemAction)
	{
		EnhancedInputComponent->BindAction(
			DropHeldItemAction,
			ETriggerEvent::Started,
			this,
			&AKCPlayerController::DropHeldItem);
	}
}

void AKCPlayerController::PlayerTick(const float DeltaSeconds)
{
	Super::PlayerTick(DeltaSeconds);

	if (IsLocalController())
	{
		UpdateCharacterFacing(DeltaSeconds);
	}
}

void AKCPlayerController::Move(const FInputActionValue& InputValue)
{
	if (AKCPlayerCharacter* PlayerCharacter = Cast<AKCPlayerCharacter>(GetPawn()))
	{
		const FVector2D MovementInput = InputValue.Get<FVector2D>();
		PlayerCharacter->MoveInWorldDirection(FVector::ForwardVector, MovementInput.Y);
		PlayerCharacter->MoveInWorldDirection(FVector::RightVector, MovementInput.X);
	}
}

void AKCPlayerController::UpdateCharacterFacing(const float DeltaSeconds)
{
	AKCPlayerCharacter* PlayerCharacter = Cast<AKCPlayerCharacter>(GetPawn());
	if (!PlayerCharacter)
	{
		return;
	}

	FVector MouseWorldLocation;
	FVector MouseWorldDirection;
	if (!DeprojectMousePositionToWorld(MouseWorldLocation, MouseWorldDirection))
	{
		return;
	}

	if (FMath::IsNearlyZero(MouseWorldDirection.Z))
	{
		return;
	}

	const FVector CharacterLocation = PlayerCharacter->GetActorLocation();
	const float DistanceToCharacterPlane =
		(CharacterLocation.Z - MouseWorldLocation.Z) / MouseWorldDirection.Z;
	if (DistanceToCharacterPlane <= 0.0f)
	{
		return;
	}

	const FVector MousePlaneLocation =
		MouseWorldLocation + MouseWorldDirection * DistanceToCharacterPlane;
	PlayerCharacter->UpdateFacingDirection(MousePlaneLocation - CharacterLocation, DeltaSeconds);
}
