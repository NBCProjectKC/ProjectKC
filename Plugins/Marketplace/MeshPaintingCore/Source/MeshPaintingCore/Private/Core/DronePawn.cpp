// Copyright Shared Orbit 2026. All Rights Reserved.

#include "Core/DronePawn.h"
#include "Camera/CameraComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Painting/PaintingModeControllerComponent.h"

ADronePawn::ADronePawn()
	: FlightSpeed(600.0f)
	, MinFlightSpeed(100.0f)
	, MaxFlightSpeed(12000.0f)
	, SpeedStep(250.0f)
	, FastSpeedMultiplier(4.0f)
	, SlowSpeedMultiplier(0.25f)
	, bRequireLookInputForMovement(false)
	, bSweepMovement(false)
	, bRequireLookInputForLook(false)
	, bBlockLookWhilePointerButtonDown(true)
	, bBlockLookWhilePainting(true)
	, LookResumeDelayAfterPointerInput(0.08f)
	, LookInputKey(EKeys::RightMouseButton)
	, LookSensitivity(0.6f)
	, bInvertLookY(false)
	, MaxPitch(89.0f)
	, bManageMouseCursorDuringLook(false)
	, bWasUsingLookInput(false)
	, bPreviousShowMouseCursor(false)
	, PreviousMouseCursor(EMouseCursor::Default)
	, LookBlockedUntilTime(0.0f)
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	CollisionComponent->SetSphereRadius(16.0f);
	CollisionComponent->SetCollisionProfileName(TEXT("Pawn"));
	CollisionComponent->SetGenerateOverlapEvents(false);
	RootComponent = CollisionComponent;

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(RootComponent);
	CameraComponent->bUsePawnControlRotation = false;

	AutoPossessPlayer = EAutoReceiveInput::Player0;
}

void ADronePawn::BeginPlay()
{
	Super::BeginPlay();

	FlightSpeed = FMath::Clamp(FlightSpeed, MinFlightSpeed, MaxFlightSpeed);
}

void ADronePawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController || !PlayerController->IsLocalPlayerController())
	{
		if (bWasUsingLookInput) EndLookInput(PlayerController);
		return;
	}

	const bool bUsingLookInput = PlayerController->IsInputKeyDown(LookInputKey);
	const bool bShouldTrackLookInput = bRequireLookInputForLook || bRequireLookInputForMovement || bManageMouseCursorDuringLook;
	if (bShouldTrackLookInput && bUsingLookInput && !bWasUsingLookInput)
	{
		BeginLookInput(PlayerController);
	}
	else if ((!bShouldTrackLookInput || !bUsingLookInput) && bWasUsingLookInput)
	{
		EndLookInput(PlayerController);
	}

	RefreshLookBlock(PlayerController);
	if (ShouldUpdateLook(PlayerController)) UpdateLook(PlayerController);
	if (!bRequireLookInputForMovement || bUsingLookInput)
	{
		UpdateMovement(PlayerController, DeltaSeconds);
		UpdateFlightSpeed(PlayerController);
	}
}

void ADronePawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (bWasUsingLookInput) EndLookInput(PlayerController);
	}

	Super::EndPlay(EndPlayReason);
}

void ADronePawn::RefreshLookBlock(APlayerController* PlayerController)
{
	if (!PlayerController) return;

	const bool bPointerButtonBlocksLook =
		bBlockLookWhilePointerButtonDown &&
		(PlayerController->IsInputKeyDown(EKeys::LeftMouseButton) ||
			PlayerController->IsInputKeyDown(EKeys::MiddleMouseButton) ||
			PlayerController->IsInputKeyDown(EKeys::RightMouseButton));
	const bool bPaintingBlocksLook = bBlockLookWhilePainting && IsPaintingControllerBlockingLook();
	if (!bPointerButtonBlocksLook && !bPaintingBlocksLook) return;

	if (UWorld* World = GetWorld())
	{
		LookBlockedUntilTime = FMath::Max(
			LookBlockedUntilTime,
			World->GetTimeSeconds() + FMath::Max(0.0f, LookResumeDelayAfterPointerInput));
	}

	float IgnoredMouseDeltaX = 0.0f;
	float IgnoredMouseDeltaY = 0.0f;
	PlayerController->GetInputMouseDelta(IgnoredMouseDeltaX, IgnoredMouseDeltaY);
}

bool ADronePawn::IsPaintingControllerBlockingLook() const
{
	TArray<UPaintingModeControllerComponent*> PaintingControllers;
	GetComponents(PaintingControllers);
	for (const UPaintingModeControllerComponent* PaintingController : PaintingControllers)
	{
		if (!PaintingController) continue;
		if (!PaintingController->bIsPaintingModeActive) continue;
		if (PaintingController->bIsPainting ||
			PaintingController->bIsOrbiting ||
			PaintingController->bIsPanningCamera ||
			PaintingController->bIsAdjustingBrushSize)
		{
			return true;
		}
	}

	return false;
}

bool ADronePawn::ShouldUpdateLook(APlayerController* PlayerController) const
{
	if (!PlayerController) return false;
	if (bRequireLookInputForLook && !PlayerController->IsInputKeyDown(LookInputKey)) return false;
	if (const UWorld* World = GetWorld())
	{
		if (World->GetTimeSeconds() < LookBlockedUntilTime) return false;
	}

	if (bBlockLookWhilePointerButtonDown)
	{
		if (PlayerController->IsInputKeyDown(EKeys::LeftMouseButton)) return false;
		if (PlayerController->IsInputKeyDown(EKeys::MiddleMouseButton)) return false;
		if (PlayerController->IsInputKeyDown(EKeys::RightMouseButton)) return false;
	}

	return true;
}

void ADronePawn::UpdateLook(APlayerController* PlayerController)
{
	if (!PlayerController) return;

	float MouseDeltaX = 0.0f;
	float MouseDeltaY = 0.0f;
	PlayerController->GetInputMouseDelta(MouseDeltaX, MouseDeltaY);
	if (FMath::IsNearlyZero(MouseDeltaX) && FMath::IsNearlyZero(MouseDeltaY)) return;

	FRotator NewRotation = GetActorRotation();
	NewRotation.Yaw += MouseDeltaX * LookSensitivity;
	const float LookYSign = bInvertLookY ? -1.0f : 1.0f;
	NewRotation.Pitch = FMath::Clamp(
		FRotator::NormalizeAxis(NewRotation.Pitch + (MouseDeltaY * LookSensitivity * LookYSign)),
		-MaxPitch,
		MaxPitch);
	NewRotation.Roll = 0.0f;
	SetActorRotation(NewRotation);
}

void ADronePawn::UpdateMovement(APlayerController* PlayerController, float DeltaSeconds)
{
	if (!PlayerController || DeltaSeconds <= 0.0f) return;

	FVector MoveDirection = FVector::ZeroVector;
	if (PlayerController->IsInputKeyDown(EKeys::W)) MoveDirection += GetActorForwardVector();
	if (PlayerController->IsInputKeyDown(EKeys::S)) MoveDirection -= GetActorForwardVector();
	if (PlayerController->IsInputKeyDown(EKeys::D)) MoveDirection += GetActorRightVector();
	if (PlayerController->IsInputKeyDown(EKeys::A)) MoveDirection -= GetActorRightVector();
	if (PlayerController->IsInputKeyDown(EKeys::E) || PlayerController->IsInputKeyDown(EKeys::SpaceBar))
		MoveDirection += FVector::UpVector;
	if (PlayerController->IsInputKeyDown(EKeys::Q) || PlayerController->IsInputKeyDown(EKeys::C))
		MoveDirection -= FVector::UpVector;

	if (MoveDirection.IsNearlyZero()) return;

	FHitResult SweepHit;
	AddActorWorldOffset(
		MoveDirection.GetSafeNormal() * GetEffectiveFlightSpeed(PlayerController) * DeltaSeconds,
		bSweepMovement,
		bSweepMovement ? &SweepHit : nullptr);
}

void ADronePawn::UpdateFlightSpeed(APlayerController* PlayerController)
{
	if (!PlayerController) return;

	const bool bScrollUp = PlayerController->WasInputKeyJustPressed(EKeys::MouseScrollUp);
	const bool bScrollDown = PlayerController->WasInputKeyJustPressed(EKeys::MouseScrollDown);
	if (!bScrollUp && !bScrollDown) return;
	if (ShouldIgnoreFlightSpeedScroll(PlayerController)) return;

	if (bScrollUp)
	{
		FlightSpeed = FMath::Clamp(FlightSpeed + SpeedStep, MinFlightSpeed, MaxFlightSpeed);
	}
	if (bScrollDown)
	{
		FlightSpeed = FMath::Clamp(FlightSpeed - SpeedStep, MinFlightSpeed, MaxFlightSpeed);
	}
}

bool ADronePawn::ShouldIgnoreFlightSpeedScroll(APlayerController* PlayerController) const
{
	if (!PlayerController) return false;
	if (!PlayerController->IsInputKeyDown(EKeys::LeftControl) && !PlayerController->IsInputKeyDown(EKeys::RightControl))
	{
		return false;
	}

	TArray<UPaintingModeControllerComponent*> PaintingControllers;
	GetComponents(PaintingControllers);
	for (const UPaintingModeControllerComponent* PaintingController : PaintingControllers)
	{
		if (PaintingController && PaintingController->bIsPaintingModeActive)
		{
			return true;
		}
	}

	return false;
}

void ADronePawn::BeginLookInput(APlayerController* PlayerController)
{
	bWasUsingLookInput = true;
	if (!PlayerController || !bManageMouseCursorDuringLook) return;

	bPreviousShowMouseCursor = PlayerController->bShowMouseCursor;
	PreviousMouseCursor = PlayerController->CurrentMouseCursor.GetValue();
	PlayerController->bShowMouseCursor = false;

	FInputModeGameOnly InputMode;
	InputMode.SetConsumeCaptureMouseDown(false);
	PlayerController->SetInputMode(InputMode);
}

void ADronePawn::EndLookInput(APlayerController* PlayerController)
{
	bWasUsingLookInput = false;
	if (!PlayerController || !bManageMouseCursorDuringLook) return;

	PlayerController->bShowMouseCursor = bPreviousShowMouseCursor;
	PlayerController->CurrentMouseCursor = PreviousMouseCursor;
}

float ADronePawn::GetEffectiveFlightSpeed(APlayerController* PlayerController) const
{
	float EffectiveSpeed = FMath::Clamp(FlightSpeed, MinFlightSpeed, MaxFlightSpeed);
	if (!PlayerController) return EffectiveSpeed;

	if (PlayerController->IsInputKeyDown(EKeys::LeftShift) || PlayerController->IsInputKeyDown(EKeys::RightShift))
	{
		EffectiveSpeed *= FastSpeedMultiplier;
	}
	if (PlayerController->IsInputKeyDown(EKeys::LeftControl) || PlayerController->IsInputKeyDown(EKeys::RightControl))
	{
		EffectiveSpeed *= SlowSpeedMultiplier;
	}

	return EffectiveSpeed;
}
