#pragma once

#include "CoreMinimal.h"
#include "KCProjectileConfigStruct.generated.h"

class AKCActionProjectile;
class UNiagaraSystem;
class USoundBase;
class UStaticMesh;

/** 투사체가 폭발하는 시점을 정한다. */
UENUM(BlueprintType)
enum class EKCProjectileDetonationMode : uint8
{
	/** 처음 발생한 Blocking Hit에서 즉시 폭발한다. */
	OnImpact UMETA(DisplayName = "On Impact"),

	/** 지형과 Pawn에 튕기며 설정한 퓨즈가 끝났을 때 폭발한다. */
	OnFuse UMETA(DisplayName = "On Fuse"),

	/** Blocking Hit 또는 퓨즈 중 먼저 발생한 시점에 폭발한다. */
	OnImpactOrFuse UMETA(DisplayName = "On Impact Or Fuse")
};

/** Throw Projectile Fragment가 투사체를 생성하고 날리는 데 필요한 설정이다. */
USTRUCT(BlueprintType)
struct PROJECTKC_API FKCProjectileLaunchConfigStruct
{
	GENERATED_BODY()

	/** 비어 있으면 실행할 수 없다. BP 파생 클래스로 외형이나 추가 연출을 확장할 수 있다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	TSubclassOf<AKCActionProjectile> ProjectileClass;

	/** 생성된 투사체에 표시할 메시다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UStaticMesh> ProjectileMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	FVector ProjectileMeshScale = FVector::OneVector;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Projectile",
		meta = (ClampMin = "1.0", UIMin = "1.0"))
	float CollisionRadius = 18.0f;

	/** 소스 Actor의 전방을 기준으로 한 생성 거리다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Launch")
	float SpawnForwardOffset = 75.0f;

	/** 소스 Actor의 위쪽을 기준으로 한 생성 높이다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Launch")
	float SpawnUpOffset = 20.0f;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Launch",
		meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ForwardSpeed = 1200.0f;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Launch",
		meta = (ClampMin = "0.0", UIMin = "0.0"))
	float UpwardSpeed = 350.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float GravityScale = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	bool bShouldBounce = true;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Movement",
		meta = (EditCondition = "bShouldBounce", ClampMin = "0.0", ClampMax = "1.0"))
	float Bounciness = 0.55f;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Movement",
		meta = (EditCondition = "bShouldBounce", ClampMin = "0.0", ClampMax = "1.0"))
	float Friction = 0.2f;

	bool Validate(FString& OutError) const;
};

/** 생성 뒤 원본 아이템과 독립적으로 유지되는 폭발 결과 설정이다. */
USTRUCT(BlueprintType)
struct PROJECTKC_API FKCProjectileExplosionConfigStruct
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Detonation")
	EKCProjectileDetonationMode DetonationMode =
		EKCProjectileDetonationMode::OnFuse;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Detonation",
		meta = (
			EditCondition = "DetonationMode != EKCProjectileDetonationMode::OnImpact",
			EditConditionHides,
			ClampMin = "0.01"))
	float FuseDuration = 2.5f;

	/** 충돌하지 못한 투사체가 영구히 남지 않게 하는 안전 수명이다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Detonation",
		meta = (ClampMin = "0.1"))
	float MaximumLifetime = 15.0f;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Explosion",
		meta = (ClampMin = "1.0", UIMin = "1.0"))
	float ExplosionRadius = 300.0f;

	/** 0이면 반경 안의 모든 유효한 ASC 대상을 처리한다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Explosion",
		meta = (ClampMin = "0"))
	int32 MaximumTargets = 0;

	/** false면 던진 플레이어는 폭발 결과에서 제외한다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Explosion")
	bool bAffectInstigator = false;

	/** true면 Visibility Trace가 가려진 대상에는 Target Fragment를 실행하지 않는다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Explosion")
	bool bRequireLineOfSight = false;

	/** 폭발 위치에서 한 번 재생할 사운드다. 비워도 된다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Explosion|Effects")
	TObjectPtr<USoundBase> ExplosionSound;

	/** 폭발 위치에서 한 번 생성할 Niagara 시스템이다. 비워도 된다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Explosion|Effects")
	TObjectPtr<UNiagaraSystem> ExplosionVFX;

	bool UsesFuse() const;
	bool ExplodesOnImpact() const;
	bool Validate(FString& OutError) const;
};
