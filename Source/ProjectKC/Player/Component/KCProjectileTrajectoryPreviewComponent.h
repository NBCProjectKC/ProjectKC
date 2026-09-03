#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KCProjectileTrajectoryPreviewComponent.generated.h"

class AKCWorldItemActor;
class UKCThrowProjectileFragment;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;

/** 소유 플레이어에게만 충전 투척의 첫 Blocking Hit까지를 표시한다. */
UCLASS(ClassGroup = (KC), meta = (BlueprintSpawnableComponent))
class PROJECTKC_API UKCProjectileTrajectoryPreviewComponent
	: public UActorComponent
{
	GENERATED_BODY()

public:
	UKCProjectileTrajectoryPreviewComponent();

	bool BeginPreview(AKCWorldItemActor* SourceItem);
	void EndPreview();

	UFUNCTION(BlueprintPure, Category = "KC|Projectile|Preview")
	bool IsPreviewActive() const;

	UFUNCTION(BlueprintPure, Category = "KC|Projectile|Preview")
	float GetCurrentChargeAlpha() const;

	UFUNCTION(BlueprintPure, Category = "KC|Projectile|Preview")
	bool HasPredictedImpact() const;

	UFUNCTION(BlueprintPure, Category = "KC|Projectile|Preview")
	FVector GetPredictedImpactLocation() const;

protected:
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, Category = "KC|Projectile|Preview|Trajectory")
	TObjectPtr<UStaticMesh> TrajectoryPointMesh;

	UPROPERTY(EditDefaultsOnly, Category = "KC|Projectile|Preview|Trajectory")
	TObjectPtr<UMaterialInterface> TrajectoryPointMaterial;

	UPROPERTY(
		EditDefaultsOnly,
		Category = "KC|Projectile|Preview|Trajectory",
		meta = (ClampMin = "0.001", UIMin = "0.01", UIMax = "0.2"))
	float TrajectoryPointScale = 0.055f;

	UPROPERTY(EditDefaultsOnly, Category = "KC|Projectile|Preview|Impact")
	TObjectPtr<UStaticMesh> ImpactMarkerMesh;

	UPROPERTY(EditDefaultsOnly, Category = "KC|Projectile|Preview|Impact")
	TObjectPtr<UMaterialInterface> ImpactMarkerMaterial;

	UPROPERTY(
		EditDefaultsOnly,
		Category = "KC|Projectile|Preview|Impact",
		meta = (ClampMin = "1.0", UIMin = "10.0", UIMax = "100.0"))
	float ImpactMarkerRadius = 40.0f;

	UPROPERTY(
		EditDefaultsOnly,
		Category = "KC|Projectile|Preview|Impact",
		meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "10.0"))
	float ImpactMarkerSurfaceOffset = 2.0f;

private:
	const UKCThrowProjectileFragment* ResolvePreviewFragment(
		const AKCWorldItemActor* SourceItem) const;
	void EnsureVisualizationComponents();
	void UpdatePreview();
	void ClearVisualization();

	TWeakObjectPtr<AKCWorldItemActor> ActiveSourceItem;
	TWeakObjectPtr<UKCThrowProjectileFragment> ActiveThrowFragment;
	double ChargeStartTimeSeconds = 0.0;
	float CurrentChargeAlpha = 0.0f;
	bool bHasPredictedImpact = false;
	FVector PredictedImpactLocation = FVector::ZeroVector;

	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> TrajectoryPointInstances;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> ImpactMarkerComponent;
};
