#include "ProjectKC/AbilitySystem/Projectile/KCActionProjectile.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraFunctionLibrary.h"
#include "ProjectKC/AbilitySystem/Component/KCKnockbackComponent.h"
#include "ProjectKC/AbilitySystem/Struct/KCSetByCallerValueStruct.h"
#include "Sound/SoundBase.h"

AKCActionProjectile::AKCActionProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	CollisionComponent =
		CreateDefaultSubobject<USphereComponent>(TEXT("ProjectileCollision"));
	SetRootComponent(CollisionComponent);
	CollisionComponent->InitSphereRadius(18.0f);
	CollisionComponent->SetCollisionProfileName(TEXT("KCProjectile"));
	CollisionComponent->SetGenerateOverlapEvents(false);
	CollisionComponent->SetNotifyRigidBodyCollision(true);
	CollisionComponent->OnComponentHit.AddDynamic(
		this,
		&AKCActionProjectile::HandleBlockingHit);

	ProjectileMeshComponent =
		CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMeshComponent->SetupAttachment(CollisionComponent);
	ProjectileMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProjectileMeshComponent->SetGenerateOverlapEvents(false);
	ProjectileMeshComponent->SetCanEverAffectNavigation(false);

	ProjectileMovementComponent =
		CreateDefaultSubobject<UProjectileMovementComponent>(
			TEXT("ProjectileMovement"));
	ProjectileMovementComponent->UpdatedComponent = CollisionComponent;
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->bShouldBounce = true;
	ProjectileMovementComponent->Bounciness = 0.55f;
	ProjectileMovementComponent->Friction = 0.2f;
	ProjectileMovementComponent->ProjectileGravityScale = 1.0f;
	ProjectileMovementComponent->InitialSpeed = 0.0f;
	ProjectileMovementComponent->MaxSpeed = 0.0f;
}

void AKCActionProjectile::BeginPlay()
{
	Super::BeginPlay();

	// 서버 위치 복제를 단일 진실로 사용하고 Simulated Proxy는 충돌 판정을 하지 않는다.
	if (!HasAuthority())
	{
		ProjectileMovementComponent->SetComponentTickEnabled(false);
		RefreshSourceMovementIgnore(
			GetOwner(),
			GetInstigator() ? GetInstigator() : Cast<APawn>(GetOwner()));
	}
}

void AKCActionProjectile::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FuseTimerHandle);
	}
	ClearSourceMovementIgnore();
	Super::EndPlay(EndPlayReason);
}

void AKCActionProjectile::OnRep_Owner()
{
	Super::OnRep_Owner();
	RefreshSourceMovementIgnore(
		GetOwner(),
		GetInstigator() ? GetInstigator() : Cast<APawn>(GetOwner()));
}

void AKCActionProjectile::OnRep_Instigator()
{
	Super::OnRep_Instigator();
	RefreshSourceMovementIgnore(
		GetOwner(),
		GetInstigator() ? GetInstigator() : Cast<APawn>(GetOwner()));
}

bool AKCActionProjectile::InitializeProjectile(
	const FKCProjectileLaunchConfigStruct& LaunchConfig,
	const FKCProjectileExplosionConfigStruct& ExplosionConfig,
	UAbilitySystemComponent* SourceAbilitySystem,
	UObject* EffectSourceObject,
	AActor* SourceActor,
	APawn* SourcePawn,
	const FVector& InitialVelocity)
{
	FString LaunchError;
	FString ExplosionError;
	if (!HasAuthority() || bInitialized || !SourceAbilitySystem ||
		!IsValid(SourceActor) || InitialVelocity.ContainsNaN() ||
		!LaunchConfig.Validate(LaunchError) ||
		!ExplosionConfig.Validate(ExplosionError))
	{
		return false;
	}

	FGameplayEffectContextHandle EffectContext =
		SourceAbilitySystem->MakeEffectContext();
	EffectContext.AddInstigator(SourceActor, this);
	EffectContext.AddSourceObject(EffectSourceObject ? EffectSourceObject : this);
	ExplosionEffectSpec = SourceAbilitySystem->MakeOutgoingSpec(
		ExplosionConfig.EffectRecipe.EffectClass,
		ExplosionConfig.EffectRecipe.EffectLevel,
		EffectContext);
	if (!ExplosionEffectSpec.IsValid())
	{
		return false;
	}

	ExplosionEffectSpec.Data->DynamicGrantedTags.AppendTags(
		ExplosionConfig.EffectRecipe.DynamicGrantedTags);
	for (const FKCSetByCallerValueStruct& Value :
		ExplosionConfig.EffectRecipe.SetByCallers)
	{
		ExplosionEffectSpec.Data->SetSetByCallerMagnitude(
			Value.DataTag,
			Value.Magnitude);
	}

	ActiveExplosionConfig = ExplosionConfig;
	ActiveEffectSourceObject = EffectSourceObject;
	ActiveSourceAbilitySystem = SourceAbilitySystem;
	ReplicatedProjectileMesh = LaunchConfig.ProjectileMesh;
	ReplicatedProjectileMeshScale = LaunchConfig.ProjectileMeshScale;
	ReplicatedCollisionRadius = LaunchConfig.CollisionRadius;
	ApplyPresentation();

	CollisionComponent->SetCollisionProfileName(TEXT("KCProjectile"));
	RefreshSourceMovementIgnore(SourceActor, SourcePawn);

	ProjectileMovementComponent->ProjectileGravityScale = LaunchConfig.GravityScale;
	ProjectileMovementComponent->bShouldBounce = LaunchConfig.bShouldBounce;
	ProjectileMovementComponent->Bounciness = LaunchConfig.Bounciness;
	ProjectileMovementComponent->Friction = LaunchConfig.Friction;
	ProjectileMovementComponent->Velocity = InitialVelocity;
	ProjectileMovementComponent->Activate(true);

	bInitialized = true;
	SetLifeSpan(ExplosionConfig.MaximumLifetime);
	if (ExplosionConfig.UsesFuse())
	{
		GetWorldTimerManager().SetTimer(
			FuseTimerHandle,
			this,
			&AKCActionProjectile::HandleFuseExpired,
			ExplosionConfig.FuseDuration,
			false);
	}

	ForceNetUpdate();
	return true;
}

void AKCActionProjectile::HandleFuseExpired()
{
	Detonate();
}

bool AKCActionProjectile::Detonate()
{
	if (!HasAuthority() || !bInitialized || bHasDetonated)
	{
		return false;
	}

	bHasDetonated = true;
	GetWorldTimerManager().ClearTimer(FuseTimerHandle);
	ProjectileMovementComponent->StopMovementImmediately();
	ProjectileMovementComponent->Deactivate();
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	TArray<AActor*> Targets;
	GatherExplosionTargets(Targets);
	for (AActor* Target : Targets)
	{
		ApplyExplosionToTarget(Target);
	}

	MulticastPlayExplosionEffects(
		GetActorLocation(),
		GetActorRotation(),
		ActiveExplosionConfig.ExplosionSound,
		ActiveExplosionConfig.ExplosionVFX);
	Destroy();
	return true;
}

USphereComponent* AKCActionProjectile::GetCollisionComponent() const
{
	return CollisionComponent;
}

UProjectileMovementComponent* AKCActionProjectile::GetProjectileMovement() const
{
	return ProjectileMovementComponent;
}

AActor* AKCActionProjectile::GetIgnoredSourceActor() const
{
	return IgnoredSourceActor.Get();
}

void AKCActionProjectile::HandleBlockingHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	if (!HasAuthority() || bHasDetonated ||
		OtherActor == IgnoredSourceActor.Get())
	{
		return;
	}

	if (ActiveExplosionConfig.ExplodesOnImpact())
	{
		Detonate();
	}
}

void AKCActionProjectile::OnRep_Presentation()
{
	ApplyPresentation();
}

void AKCActionProjectile::MulticastPlayExplosionEffects_Implementation(
	FVector_NetQuantize ExplosionLocation,
	FRotator ExplosionRotation,
	USoundBase* ExplosionSound,
	UNiagaraSystem* ExplosionVFX)
{
	if (ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			ExplosionSound,
			ExplosionLocation);
	}

	if (ExplosionVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this,
			ExplosionVFX,
			ExplosionLocation,
			ExplosionRotation);
	}
}

void AKCActionProjectile::ApplyPresentation()
{
	ProjectileMeshComponent->SetStaticMesh(ReplicatedProjectileMesh);
	ProjectileMeshComponent->SetRelativeScale3D(ReplicatedProjectileMeshScale);
	CollisionComponent->SetSphereRadius(ReplicatedCollisionRadius, true);
}

void AKCActionProjectile::GatherExplosionTargets(
	TArray<AActor*>& OutTargets) const
{
	OutTargets.Reset();
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float RadiusSquared = FMath::Square(
		ActiveExplosionConfig.ExplosionRadius);
	for (TActorIterator<APawn> It(World); It; ++It)
	{
		APawn* Target = *It;
		if (!IsValid(Target) ||
			FVector::DistSquared(GetActorLocation(), Target->GetActorLocation()) >
				RadiusSquared ||
			(!ActiveExplosionConfig.bAffectInstigator && Target == GetInstigator()) ||
			!UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target) ||
			!HasLineOfSightTo(Target))
		{
			continue;
		}

		OutTargets.Add(Target);
		if (ActiveExplosionConfig.MaximumTargets > 0 &&
			OutTargets.Num() >= ActiveExplosionConfig.MaximumTargets)
		{
			break;
		}
	}
}

bool AKCActionProjectile::HasLineOfSightTo(const AActor* TargetActor) const
{
	if (!ActiveExplosionConfig.bRequireLineOfSight || !TargetActor)
	{
		return true;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(KCProjectileLineOfSight));
	QueryParams.AddIgnoredActor(this);
	if (const AActor* SourceActor = IgnoredSourceActor.Get())
	{
		QueryParams.AddIgnoredActor(SourceActor);
	}

	FHitResult Hit;
	const bool bBlocked = GetWorld()->LineTraceSingleByChannel(
		Hit,
		GetActorLocation(),
		TargetActor->GetActorLocation(),
		ECC_Visibility,
		QueryParams);
	return !bBlocked || Hit.GetActor() == TargetActor;
}

void AKCActionProjectile::ApplyExplosionToTarget(AActor* TargetActor) const
{
	UAbilitySystemComponent* TargetAbilitySystem =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	if (!TargetAbilitySystem || !ActiveSourceAbilitySystem ||
		!ExplosionEffectSpec.IsValid())
	{
		return;
	}

	// Spec는 발사 순간 만들어 두었으므로 원본 일회용 아이템이 사라져도 적용할 수 있다.
	ActiveSourceAbilitySystem->ApplyGameplayEffectSpecToTarget(
		*ExplosionEffectSpec.Data.Get(),
		TargetAbilitySystem);

	if (!ActiveExplosionConfig.Knockback.bEnabled)
	{
		return;
	}

	UKCKnockbackComponent* KnockbackComponent =
		TargetActor->FindComponentByClass<UKCKnockbackComponent>();
	if (!KnockbackComponent)
	{
		return;
	}

	FKCKnockbackRequest Request;
	Request.Direction = TargetActor->GetActorLocation() - GetActorLocation();
	if (Request.Direction.IsNearlyZero())
	{
		Request.Direction = GetActorForwardVector();
	}
	Request.HorizontalSpeed = ActiveExplosionConfig.Knockback.HorizontalSpeed;
	Request.VerticalSpeed = ActiveExplosionConfig.Knockback.VerticalSpeed;
	Request.bOverrideHorizontalVelocity =
		ActiveExplosionConfig.Knockback.bOverrideHorizontalVelocity;
	Request.bOverrideVerticalVelocity =
		ActiveExplosionConfig.Knockback.bOverrideVerticalVelocity;
	KnockbackComponent->ApplyKnockback(Request);
}

void AKCActionProjectile::ClearSourceMovementIgnore()
{
	if (AActor* SourceActor = IgnoredSourceActor.Get())
	{
		CollisionComponent->IgnoreActorWhenMoving(SourceActor, false);
	}
	if (APawn* SourcePawn = IgnoredSourcePawn.Get())
	{
		SourcePawn->MoveIgnoreActorRemove(this);
	}
	IgnoredSourcePawn.Reset();
	IgnoredSourceActor.Reset();
}

void AKCActionProjectile::RefreshSourceMovementIgnore(
	AActor* SourceActor,
	APawn* SourcePawn)
{
	if (IgnoredSourceActor.Get() == SourceActor &&
		IgnoredSourcePawn.Get() == SourcePawn)
	{
		return;
	}

	ClearSourceMovementIgnore();
	if (!IsValid(SourceActor))
	{
		return;
	}

	IgnoredSourceActor = SourceActor;
	IgnoredSourcePawn = SourcePawn;
	CollisionComponent->IgnoreActorWhenMoving(SourceActor, true);
	if (SourcePawn)
	{
		// 투사체 Sweep와 투척자 이동 Sweep 양쪽에서 서로를 무시한다.
		SourcePawn->MoveIgnoreActorAdd(this);
	}
}

void AKCActionProjectile::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AKCActionProjectile, ReplicatedProjectileMesh);
	DOREPLIFETIME(AKCActionProjectile, ReplicatedProjectileMeshScale);
	DOREPLIFETIME(AKCActionProjectile, ReplicatedCollisionRadius);
}
