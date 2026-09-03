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
#include "ProjectKC/AbilitySystem/Fragment/KCActionExecutionContext.h"
#include "ProjectKC/AbilitySystem/Fragment/KCActionFragment.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogKCActionProjectile, Log, All);

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
	// 서버 업데이트 사이를 메꾸기 위한 기본값이다. BP 파생 클래스에서 조정할 수 있고,
	// 실제 보간 대상 연결은 클라이언트 BeginPlay에서 한다.
	ProjectileMovementComponent->bInterpMovement = true;
	ProjectileMovementComponent->bInterpRotation = true;
}

void AKCActionProjectile::BeginPlay()
{
	Super::BeginPlay();

	// 서버 위치 복제를 단일 진실로 사용하고 Simulated Proxy는 충돌 판정을 하지 않는다.
	if (!HasAuthority())
	{
		// 시뮬레이션만 멈춘다. 컴포넌트 틱까지 끄면 보간이 돌지 못해
		// 서버 업데이트가 도착할 때마다 위치가 그대로 튄다.
		ProjectileMovementComponent->bSimulationEnabled = false;
		ProjectileMovementComponent->SetInterpolatedComponent(ProjectileMeshComponent);
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

void AKCActionProjectile::PostNetReceiveLocationAndRotation()
{
	// 기본 구현은 SetActorLocationAndRotation()으로 즉시 대입하므로 업데이트마다 튄다.
	// 보간 대상이 연결된 클라이언트에서는 목표만 갱신하고 메시가 따라오게 한다.
	if (!ProjectileMovementComponent ||
		!ProjectileMovementComponent->bInterpMovement ||
		!ProjectileMovementComponent->GetInterpolatedComponent())
	{
		Super::PostNetReceiveLocationAndRotation();
		return;
	}

	const FRepMovement& LocalRepMovement = GetReplicatedMovement();
	ProjectileMovementComponent->MoveInterpolationTarget(
		FRepMovement::RebaseOntoLocalOrigin(LocalRepMovement.Location, this),
		LocalRepMovement.Rotation);
}

bool AKCActionProjectile::InitializeProjectile(
	const FKCProjectileLaunchConfigStruct& LaunchConfig,
	const FKCProjectileExplosionConfigStruct& ExplosionConfig,
	const TArray<TObjectPtr<UKCActionFragment>>& ExplosionTargetFragments,
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

	FString TargetFragmentsError;
	if (!InitializeExplosionTargetFragments(
		ExplosionTargetFragments,
		SourceAbilitySystem,
		EffectSourceObject,
		SourceActor,
		TargetFragmentsError))
	{
		UE_LOG(
			LogKCActionProjectile,
			Warning,
			TEXT("투사체 Target Fragment 초기화에 실패했습니다: %s"),
			*TargetFragmentsError);
		return false;
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
	ExecuteExplosionTargetFragments(TargetActor, TargetAbilitySystem);
}

bool AKCActionProjectile::InitializeExplosionTargetFragments(
	const TArray<TObjectPtr<UKCActionFragment>>& SourceFragments,
	UAbilitySystemComponent* SourceAbilitySystem,
	UObject* EffectSourceObject,
	AActor* SourceActor,
	FString& OutError)
{
	OutError.Reset();
	ActiveExplosionTargetFragments.Reset();
	ActiveExplosionTargetFragments.Reserve(SourceFragments.Num());

	FKCActionExecutionContext PrepareContext;
	PrepareContext.SourceAbilitySystem = SourceAbilitySystem;
	PrepareContext.SourceActor = SourceActor;
	PrepareContext.EffectSourceObject = EffectSourceObject;

	for (int32 Index = 0; Index < SourceFragments.Num(); ++Index)
	{
		const UKCActionFragment* SourceFragment = SourceFragments[Index];
		if (!IsValid(SourceFragment))
		{
			OutError = FString::Printf(
				TEXT("ExplosionTargetFragments[%d]가 비어 있습니다."),
				Index);
			return false;
		}

		if (SourceFragment->ApplicationScope != EKCActionScope::Target ||
			!SourceFragment->SupportsDeferredExecution())
		{
			OutError = FString::Printf(
				TEXT("ExplosionTargetFragments[%d] '%s'는 Target Scope와 지연 실행을 지원해야 합니다."),
				Index,
				*GetNameSafe(SourceFragment));
			return false;
		}

		FString FragmentError;
		if (!SourceFragment->Validate(FragmentError))
		{
			OutError = FString::Printf(
				TEXT("ExplosionTargetFragments[%d] '%s'가 유효하지 않습니다: %s"),
				Index,
				*GetNameSafe(SourceFragment),
				*FragmentError);
			return false;
		}

		UKCActionFragment* RuntimeFragment = DuplicateObject<UKCActionFragment>(
			SourceFragment,
			this);
		if (!RuntimeFragment)
		{
			OutError = FString::Printf(
				TEXT("ExplosionTargetFragments[%d] '%s'의 Runtime 복제에 실패했습니다."),
				Index,
				*GetNameSafe(SourceFragment));
			return false;
		}

		if (!RuntimeFragment->PrepareDeferredExecution(
			PrepareContext,
			FragmentError))
		{
			OutError = FString::Printf(
				TEXT("ExplosionTargetFragments[%d] '%s'의 지연 실행 준비에 실패했습니다: %s"),
				Index,
				*GetNameSafe(SourceFragment),
				*FragmentError);
			return false;
		}

		ActiveExplosionTargetFragments.Add(RuntimeFragment);
	}

	return true;
}

bool AKCActionProjectile::ExecuteExplosionTargetFragments(
	AActor* TargetActor,
	UAbilitySystemComponent* TargetAbilitySystem) const
{
	if (ActiveExplosionTargetFragments.IsEmpty())
	{
		return true;
	}

	FKCActionExecutionContext Context;
	Context.SourceAbilitySystem = ActiveSourceAbilitySystem;
	Context.TargetAbilitySystem = TargetAbilitySystem;
	// 지연 Fragment의 Source는 원본 Ability가 아니라 실제 효과 원점인 투사체다.
	Context.SourceActor = const_cast<AKCActionProjectile*>(this);
	Context.TargetActor = TargetActor;
	Context.EffectSourceObject = ActiveEffectSourceObject;

	TArray<const UKCActionFragment*, TInlineAllocator<8>> ExecutableFragments;
	for (const UKCActionFragment* Fragment : ActiveExplosionTargetFragments)
	{
		if (!IsValid(Fragment))
		{
			return false;
		}

		FString ExecutionError;
		if (Fragment->CanExecute(Context, ExecutionError))
		{
			ExecutableFragments.Add(Fragment);
		}
		else if (Fragment->bRequired)
		{
			UE_LOG(
				LogKCActionProjectile,
				Warning,
				TEXT("필수 폭발 Target Fragment '%s'가 대상 '%s'에서 실행 조건을 만족하지 못했습니다: %s"),
				*GetNameSafe(Fragment),
				*GetNameSafe(TargetActor),
				*ExecutionError);
			return false;
		}
	}

	for (const UKCActionFragment* Fragment : ExecutableFragments)
	{
		if (!Fragment->Execute(Context) && Fragment->bRequired)
		{
			UE_LOG(
				LogKCActionProjectile,
				Warning,
				TEXT("필수 폭발 Target Fragment '%s'가 대상 '%s'에서 실행에 실패했습니다."),
				*GetNameSafe(Fragment),
				*GetNameSafe(TargetActor));
			return false;
		}
	}

	return true;
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
