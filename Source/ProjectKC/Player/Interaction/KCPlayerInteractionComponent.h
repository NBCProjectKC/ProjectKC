#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "Engine/EngineTypes.h"
#include "KCPlayerInteractionComponent.generated.h"

class AActor;
class UPrimitiveComponent;

UCLASS(ClassGroup = (Player), meta = (BlueprintSpawnableComponent))
class PROJECTKC_API UKCPlayerInteractionComponent : public USphereComponent
{
	GENERATED_BODY()

public:
	UKCPlayerInteractionComponent();

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void TryInteract();

	UFUNCTION(BlueprintPure, Category = "Interaction")
	AActor* GetBestInteractable() const;

private:
	UFUNCTION(Server, Reliable)
	void ServerTryInteract(AActor* TargetActor);

	UPrimitiveComponent* FindInteractionComponent(
		AActor* TargetActor,
		bool bCheckLineOfSight) const;
	bool IsValidInteractionComponent(
		UPrimitiveComponent* TargetComponent,
		bool bCheckLineOfSight) const;
	bool HasLineOfSightTo(const UPrimitiveComponent* TargetComponent) const;

	UPROPERTY(EditAnywhere, Category = "Interaction")
	FName InteractableComponentTag = TEXT("Interactable");

	UPROPERTY(EditAnywhere, Category = "Interaction", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float MinimumForwardDot = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Interaction")
	bool bRequireLineOfSight = true;

	UPROPERTY(EditAnywhere, Category = "Interaction")
	TEnumAsByte<ECollisionChannel> InteractionTraceChannel = ECC_Visibility;
};
