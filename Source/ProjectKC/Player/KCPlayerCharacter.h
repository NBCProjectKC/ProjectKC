#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "KCPlayerCharacter.generated.h"

class UCameraComponent;
class UKCPlayerCombatComponent;
class USpringArmComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FKCPlayerInputRequestedSignature);

UCLASS()
class PROJECTKC_API AKCPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AKCPlayerCharacter();

	void MoveInWorldDirection(const FVector& WorldDirection, float ScaleValue);
	void UpdateFacingDirection(const FVector& WorldDirection, float DeltaSeconds);
	void TryAttack();
	void RequestInteract();
	void RequestDropHeldItem();

	UPROPERTY(BlueprintAssignable, Category = "Input|Interaction")
	FKCPlayerInputRequestedSignature OnInteractInputRequested;

	UPROPERTY(BlueprintAssignable, Category = "Input|Interaction")
	FKCPlayerInputRequestedSignature OnDropHeldItemInputRequested;

	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		AController* EventInstigator,
		AActor* DamageCauser) override;

	UFUNCTION(BlueprintPure, Category = "Combat")
	UKCPlayerCombatComponent* GetCombatComponent() const;

private:
	void ApplyFacingYaw(float FacingYaw);

	UFUNCTION(Server, Unreliable)
	void ServerSetFacingYaw(float FacingYaw);

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoomComponent;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> TopDownCameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UKCPlayerCombatComponent> CombatComponent;

	float FacingReplicationElapsed = 0.0f;
	float LastSentFacingYaw = 0.0f;
};
