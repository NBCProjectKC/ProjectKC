#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "ProjectKC/AbilitySystem/Struct/KCProjectileConfigStruct.h"
#include "KCActionProjectile.generated.h"

class APawn;
class UAbilitySystemComponent;
class UNiagaraSystem;
class UProjectileMovementComponent;
class USoundBase;
class USphereComponent;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * 아이템 수명과 분리되어 서버에서 비행·충돌·폭발 결과를 처리하는 공용 투사체다.
 * Throw Projectile Fragment가 설정을 주입하며 BP 파생 클래스로 표현을 확장할 수 있다.
 */
UCLASS(Blueprintable)
class PROJECTKC_API AKCActionProjectile : public AActor
{
	GENERATED_BODY()

public:
	AKCActionProjectile();

	/** FinishSpawning 이후 서버에서 한 번 호출한다. */
	bool InitializeProjectile(
		const FKCProjectileLaunchConfigStruct& LaunchConfig,
		const FKCProjectileExplosionConfigStruct& ExplosionConfig,
		UAbilitySystemComponent* SourceAbilitySystem,
		UObject* EffectSourceObject,
		AActor* SourceActor,
		APawn* SourcePawn,
		const FVector& InitialVelocity);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "KC|Projectile")
	bool Detonate();

	UFUNCTION(BlueprintPure, Category = "KC|Projectile")
	USphereComponent* GetCollisionComponent() const;

	UFUNCTION(BlueprintPure, Category = "KC|Projectile")
	UProjectileMovementComponent* GetProjectileMovement() const;

	UFUNCTION(BlueprintPure, Category = "KC|Projectile")
	AActor* GetIgnoredSourceActor() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnRep_Owner() override;
	virtual void OnRep_Instigator() override;
	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void HandleBlockingHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		FVector NormalImpulse,
		const FHitResult& Hit);

	UFUNCTION()
	void OnRep_Presentation();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayExplosionEffects(
		FVector_NetQuantize ExplosionLocation,
		FRotator ExplosionRotation,
		USoundBase* ExplosionSound,
		UNiagaraSystem* ExplosionVFX);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|Projectile")
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|Projectile")
	TObjectPtr<UStaticMeshComponent> ProjectileMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovementComponent;

private:
	void HandleFuseExpired();
	void ApplyPresentation();
	void GatherExplosionTargets(TArray<AActor*>& OutTargets) const;
	bool HasLineOfSightTo(const AActor* TargetActor) const;
	void ApplyExplosionToTarget(AActor* TargetActor) const;
	void RefreshSourceMovementIgnore(AActor* SourceActor, APawn* SourcePawn);
	void ClearSourceMovementIgnore();

	UPROPERTY(ReplicatedUsing = OnRep_Presentation)
	TObjectPtr<UStaticMesh> ReplicatedProjectileMesh;

	UPROPERTY(ReplicatedUsing = OnRep_Presentation)
	FVector ReplicatedProjectileMeshScale = FVector::OneVector;

	UPROPERTY(ReplicatedUsing = OnRep_Presentation)
	float ReplicatedCollisionRadius = 18.0f;

	UPROPERTY(Transient)
	FKCProjectileExplosionConfigStruct ActiveExplosionConfig;

	UPROPERTY(Transient)
	TObjectPtr<UObject> ActiveEffectSourceObject;

	UPROPERTY(Transient)
	TObjectPtr<UAbilitySystemComponent> ActiveSourceAbilitySystem;

	FGameplayEffectSpecHandle ExplosionEffectSpec;
	TWeakObjectPtr<AActor> IgnoredSourceActor;
	TWeakObjectPtr<APawn> IgnoredSourcePawn;
	FTimerHandle FuseTimerHandle;
	bool bInitialized = false;
	bool bHasDetonated = false;
};
