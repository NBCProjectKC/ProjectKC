// Copyright Shared Orbit 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GenericPlatform/ICursor.h"
#include "GameFramework/Pawn.h"
#include "InputCoreTypes.h"
#include "DronePawn.generated.h"

class APlayerController;
class UCameraComponent;
class USphereComponent;

UCLASS(BlueprintType, Blueprintable)
class MESHPAINTINGCORE_API ADronePawn : public APawn
{
	GENERATED_BODY()

public:
	ADronePawn();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drone")
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drone")
	TObjectPtr<UCameraComponent> CameraComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Flight", meta = (ClampMin = "1.0"))
	float FlightSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Flight", meta = (ClampMin = "1.0"))
	float MinFlightSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Flight", meta = (ClampMin = "1.0"))
	float MaxFlightSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Flight", meta = (ClampMin = "1.0"))
	float SpeedStep;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Flight", meta = (ClampMin = "1.0"))
	float FastSpeedMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Flight", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float SlowSpeedMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Flight")
	bool bRequireLookInputForMovement;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Flight")
	bool bSweepMovement;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Look")
	bool bRequireLookInputForLook;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Look")
	bool bBlockLookWhilePointerButtonDown;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Look")
	bool bBlockLookWhilePainting;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Look", meta = (ClampMin = "0.0"))
	float LookResumeDelayAfterPointerInput;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Look")
	FKey LookInputKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Look", meta = (ClampMin = "0.01"))
	float LookSensitivity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Look")
	bool bInvertLookY;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Look", meta = (ClampMin = "1.0", ClampMax = "89.0"))
	float MaxPitch;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Look")
	bool bManageMouseCursorDuringLook;

	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	virtual void BeginPlay() override;

private:
	void RefreshLookBlock(APlayerController* PlayerController);
	bool IsPaintingControllerBlockingLook() const;
	bool ShouldUpdateLook(APlayerController* PlayerController) const;
	void UpdateLook(APlayerController* PlayerController);
	void UpdateMovement(APlayerController* PlayerController, float DeltaSeconds);
	void UpdateFlightSpeed(APlayerController* PlayerController);
	bool ShouldIgnoreFlightSpeedScroll(APlayerController* PlayerController) const;
	void BeginLookInput(APlayerController* PlayerController);
	void EndLookInput(APlayerController* PlayerController);
	float GetEffectiveFlightSpeed(APlayerController* PlayerController) const;

	bool bWasUsingLookInput;
	bool bPreviousShowMouseCursor;
	EMouseCursor::Type PreviousMouseCursor;
	float LookBlockedUntilTime;
};
