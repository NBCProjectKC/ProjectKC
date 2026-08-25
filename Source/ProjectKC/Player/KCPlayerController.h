#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "KCPlayerController.generated.h"

struct FInputActionValue;
class UInputAction;
class UInputMappingContext;

UCLASS()
class PROJECTKC_API AKCPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AKCPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaSeconds) override;

private:
	void Move(const FInputActionValue& InputValue);
	void Dash(const FInputActionValue& InputValue);
	void BeginUseHeldItem(const FInputActionValue& InputValue);
	void EndUseHeldItem(const FInputActionValue& InputValue);
	void Interact(const FInputActionValue& InputValue);
	void DropHeldItem(const FInputActionValue& InputValue);
	void UpdateCharacterFacing(float DeltaSeconds);

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> PlayerMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> DashAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> AttackAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> DropHeldItemAction;
};
