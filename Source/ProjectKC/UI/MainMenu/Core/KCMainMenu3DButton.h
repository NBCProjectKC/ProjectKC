#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InputCoreTypes.h"
#include "KCMainMenu3DTypes.h"
#include "KCMainMenu3DButton.generated.h"

class AKCMainMenu3DManager;
class UBoxComponent;
class UMaterialInterface;
class UPrimitiveComponent;
class UText3DComponent;

UCLASS()
class PROJECTKC_API AKCMainMenu3DButton : public AActor
{
	GENERATED_BODY()

public:
	AKCMainMenu3DButton();

	virtual void OnConstruction(const FTransform& Transform) override;

	void InitializeButton(AKCMainMenu3DManager* InOwningManager);
	void SetHovered(bool bNewHovered);
	void SetPressed(bool bNewPressed);

	EKCMainMenu3DAction GetAction() const { return Action; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|MainMenu")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|MainMenu")
	TObjectPtr<UText3DComponent> Text3DComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|MainMenu")
	TObjectPtr<UBoxComponent> CollisionComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KC|MainMenu")
	EKCMainMenu3DAction Action = EKCMainMenu3DAction::CreateLobby;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KC|MainMenu")
	FText MenuText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KC|MainMenu")
	FVector CollisionBoxExtent = FVector(250.0, 50.0, 250.0);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KC|MainMenu")
	FVector CollisionRelativeOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KC|MainMenu|Visual")
	TObjectPtr<UMaterialInterface> NormalMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KC|MainMenu|Visual")
	TObjectPtr<UMaterialInterface> HoverMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KC|MainMenu|Visual")
	TObjectPtr<UMaterialInterface> PressedMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KC|MainMenu|Visual")
	FVector NormalScale = FVector(1.0);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KC|MainMenu|Visual")
	FVector HoverScale = FVector(2.0);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KC|MainMenu|Visual")
	FVector PressedScale = FVector(1.5);

private:
	UPROPERTY()
	TObjectPtr<AKCMainMenu3DManager> OwningManager;

	bool bIsHovered = false;
	bool bIsPressed = false;

	void ApplyTextSettings();
	void AlignCollisionToTextBounds();
	void ApplyVisualState();
	void ApplyMaterial(UMaterialInterface* Material);

	UFUNCTION()
	void HandleCursorOver(UPrimitiveComponent* TouchedComponent);

	UFUNCTION()
	void HandleCursorOut(UPrimitiveComponent* TouchedComponent);

	UFUNCTION()
	void HandleClicked(UPrimitiveComponent* TouchedComponent, FKey ButtonPressed);
};
