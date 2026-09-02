#include "ProjectKC/AbilitySystem/Fragment/KCThrowProjectileFragment.h"

#include "Components/ActorComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectKC/AbilitySystem/Ability/KCGA_Base.h"
#include "ProjectKC/AbilitySystem/Fragment/KCActionExecutionContext.h"
#include "ProjectKC/AbilitySystem/Projectile/KCActionProjectile.h"
#include "ProjectKC/Item/Definition/KCItemDefinition.h"
#include "ProjectKC/Item/KCWorldItemActor.h"

UKCThrowProjectileFragment::UKCThrowProjectileFragment()
{
	ApplicationScope = EKCActionScope::Source;
	bRequired = true;
	LaunchConfig.ProjectileClass = AKCActionProjectile::StaticClass();
}

bool UKCThrowProjectileFragment::Validate(FString& OutError) const
{
	FString LaunchError;
	if (!LaunchConfig.Validate(LaunchError))
	{
		OutError = FString::Printf(
			TEXT("LaunchConfig가 유효하지 않습니다: %s"),
			*LaunchError);
		return false;
	}

	FString ExplosionError;
	if (!ExplosionConfig.Validate(ExplosionError))
	{
		OutError = FString::Printf(
			TEXT("ExplosionConfig가 유효하지 않습니다: %s"),
			*ExplosionError);
		return false;
	}

	for (int32 Index = 0; Index < ExplosionTargetFragments.Num(); ++Index)
	{
		const UKCActionFragment* Fragment = ExplosionTargetFragments[Index];
		if (!IsValid(Fragment))
		{
			OutError = FString::Printf(
				TEXT("ExplosionTargetFragments[%d]가 비어 있습니다."),
				Index);
			return false;
		}

		if (Fragment->ApplicationScope != EKCActionScope::Target)
		{
			OutError = FString::Printf(
				TEXT("ExplosionTargetFragments[%d] '%s'는 Target Scope여야 합니다."),
				Index,
				*GetNameSafe(Fragment));
			return false;
		}

		if (!Fragment->SupportsDeferredExecution())
		{
			OutError = FString::Printf(
				TEXT("ExplosionTargetFragments[%d] '%s'는 지연 실행을 지원하지 않습니다."),
				Index,
				*GetNameSafe(Fragment));
			return false;
		}

		FString FragmentError;
		if (!Fragment->Validate(FragmentError))
		{
			OutError = FString::Printf(
				TEXT("ExplosionTargetFragments[%d] '%s'가 유효하지 않습니다: %s"),
				Index,
				*GetNameSafe(Fragment),
				*FragmentError);
			return false;
		}
	}

	return true;
}

bool UKCThrowProjectileFragment::DeclaresSetByCallerTag(
	FGameplayTag DataTag) const
{
	return ExplosionTargetFragments.ContainsByPredicate(
		[DataTag](const UKCActionFragment* Fragment)
		{
			return IsValid(Fragment) &&
				Fragment->DeclaresSetByCallerTag(DataTag);
		});
}

void UKCThrowProjectileFragment::AppendDeclaredSetByCallerTags(
	FGameplayTagContainer& OutTags) const
{
	for (const UKCActionFragment* Fragment : ExplosionTargetFragments)
	{
		if (IsValid(Fragment))
		{
			Fragment->AppendDeclaredSetByCallerTags(OutTags);
		}
	}
}

bool UKCThrowProjectileFragment::CanExecute(
	const FKCActionExecutionContext& Context,
	FString& OutError) const
{
	OutError.Reset();
	AActor* LaunchOrigin = ResolveLaunchOrigin(Context);
	if (!Context.IsAuthoritative() || !Context.SourceAbilitySystem ||
		!IsValid(Context.SourceActor) || !IsValid(LaunchOrigin) ||
		!LaunchOrigin->GetWorld())
	{
		OutError = TEXT("투사체를 생성할 서버 권한, Source ASC 또는 생성 원점이 없습니다.");
		return false;
	}

	return true;
}

bool UKCThrowProjectileFragment::Execute(
	const FKCActionExecutionContext& Context) const
{
	FString ExecutionError;
	if (!CanExecute(Context, ExecutionError))
	{
		return false;
	}

	AActor* LaunchOrigin = ResolveLaunchOrigin(Context);
	FTransform SpawnTransform;
	FVector InitialVelocity;
	if (!BuildLaunchSolution(
		Context.SourceActor,
		LaunchOrigin,
		Context.InputChargeAlpha,
		SpawnTransform,
		InitialVelocity))
	{
		return false;
	}

	UWorld* World = LaunchOrigin->GetWorld();
	AKCActionProjectile* Projectile =
		World->SpawnActorDeferred<AKCActionProjectile>(
			LaunchConfig.ProjectileClass,
			SpawnTransform,
			Context.SourceActor,
			Cast<APawn>(Context.SourceActor),
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Projectile)
	{
		return false;
	}

	UGameplayStatics::FinishSpawningActor(Projectile, SpawnTransform);
	if (!Projectile->InitializeProjectile(
		LaunchConfig,
		ExplosionConfig,
		ExplosionTargetFragments,
		Context.SourceAbilitySystem,
		ResolveEffectSourceObject(Context, LaunchOrigin),
		Context.SourceActor,
		Cast<APawn>(Context.SourceActor),
		InitialVelocity))
	{
		Projectile->Destroy();
		return false;
	}

	return true;
}

bool UKCThrowProjectileFragment::BuildLaunchSolution(
	const AActor* SourceActor,
	const AActor* LaunchOrigin,
	float ChargeAlpha,
	FTransform& OutSpawnTransform,
	FVector& OutInitialVelocity) const
{
	OutSpawnTransform = FTransform::Identity;
	OutInitialVelocity = FVector::ZeroVector;
	if (!IsValid(SourceActor) || !IsValid(LaunchOrigin))
	{
		return false;
	}

	FVector Forward = SourceActor->GetActorForwardVector();
	if (!Forward.Normalize())
	{
		return false;
	}

	const FVector SpawnLocation = LaunchOrigin->GetActorLocation() +
		Forward * LaunchConfig.SpawnForwardOffset +
		FVector::UpVector * LaunchConfig.SpawnUpOffset;
	OutSpawnTransform = FTransform(Forward.Rotation(), SpawnLocation);
	OutInitialVelocity =
		Forward * LaunchConfig.ResolveForwardSpeed(ChargeAlpha) +
		FVector::UpVector * LaunchConfig.UpwardSpeed;
	return !OutInitialVelocity.ContainsNaN();
}

AActor* UKCThrowProjectileFragment::ResolveLaunchOrigin(
	const FKCActionExecutionContext& Context) const
{
	UObject* SourceObject = Context.Ability
		? Context.Ability->GetCurrentSourceObject()
		: nullptr;
	if (const UActorComponent* SourceComponent =
		Cast<UActorComponent>(SourceObject))
	{
		if (AActor* ComponentOwner = SourceComponent->GetOwner())
		{
			return ComponentOwner;
		}
	}
	if (AActor* SourceObjectActor = Cast<AActor>(SourceObject))
	{
		return SourceObjectActor;
	}

	return Context.SourceActor;
}

UObject* UKCThrowProjectileFragment::ResolveEffectSourceObject(
	const FKCActionExecutionContext& Context,
	AActor* LaunchOrigin) const
{
	if (const AKCWorldItemActor* SourceItem =
		Cast<AKCWorldItemActor>(LaunchOrigin))
	{
		if (UObject* ItemDefinition = SourceItem->GetItemDefinition())
		{
			return ItemDefinition;
		}
	}

	if (Context.Ability && Context.Ability->GetCurrentSourceObject())
	{
		return Context.Ability->GetCurrentSourceObject();
	}
	return LaunchOrigin;
}
