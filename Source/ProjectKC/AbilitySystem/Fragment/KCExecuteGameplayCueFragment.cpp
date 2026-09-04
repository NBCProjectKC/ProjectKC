#include "ProjectKC/AbilitySystem/Fragment/KCExecuteGameplayCueFragment.h"

#include "AbilitySystemComponent.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameplayEffectTypes.h"
#include "ProjectKC/AbilitySystem/Ability/KCGA_Base.h"
#include "ProjectKC/AbilitySystem/Fragment/KCActionExecutionContext.h"

bool UKCExecuteGameplayCueFragment::Validate(FString& OutError) const
{
	OutError.Reset();
	if (!CueTag.IsValid())
	{
		OutError = TEXT("CueTag가 비어 있습니다.");
		return false;
	}

	const FGameplayTag GameplayCueRoot =
		FGameplayTag::RequestGameplayTag(TEXT("GameplayCue"), false);
	if (!GameplayCueRoot.IsValid() || !CueTag.MatchesTag(GameplayCueRoot))
	{
		OutError = TEXT("CueTag는 GameplayCue 하위 태그여야 합니다.");
		return false;
	}

	return true;
}

bool UKCExecuteGameplayCueFragment::SupportsDeferredExecution() const
{
	return true;
}

bool UKCExecuteGameplayCueFragment::CanExecute(
	const FKCActionExecutionContext& Context,
	FString& OutError) const
{
	OutError.Reset();
	if (!Context.IsAuthoritative() ||
		!Context.ResolveScopedAbilitySystem(ApplicationScope) ||
		!IsValid(Context.ResolveScopedActor(ApplicationScope)))
	{
		OutError = TEXT("GameplayCue를 실행할 권한, ASC 또는 대상 Actor가 없습니다.");
		return false;
	}

	return true;
}

bool UKCExecuteGameplayCueFragment::Execute(
	const FKCActionExecutionContext& Context) const
{
	UAbilitySystemComponent* ScopedAbilitySystem =
		Context.ResolveScopedAbilitySystem(ApplicationScope);
	AActor* ScopedActor = Context.ResolveScopedActor(ApplicationScope);
	if (!Context.IsAuthoritative() || !ScopedAbilitySystem ||
		!IsValid(ScopedActor) || !CueTag.IsValid())
	{
		return false;
	}

	UObject* SourceObject = Context.Ability
		? Context.Ability->GetCurrentSourceObject()
		: Context.EffectSourceObject;
	AActor* EffectCauser = Cast<AActor>(SourceObject);
	if (!EffectCauser)
	{
		if (const UActorComponent* SourceComponent =
			Cast<UActorComponent>(SourceObject))
		{
			EffectCauser = SourceComponent->GetOwner();
		}
	}
	if (!EffectCauser)
	{
		EffectCauser = Context.SourceActor;
	}

	/**
	 * Cue Notify는 HitResult가 있으면 그 ImpactNormal을, 없으면 CueParameters.Normal을
	 * 회전으로 쓴다. 방향을 지정했다면 두 경로가 어긋나지 않도록 둘 다 덮어쓴다.
	 */
	const FVector CueDirection = ResolveDirection(Context);
	const bool bHasCueDirection = !CueDirection.IsNearlyZero();

	FHitResult CueHitResult = Context.HitResult;
	if (bHasCueDirection)
	{
		CueHitResult.Normal = CueDirection;
		CueHitResult.ImpactNormal = CueDirection;
	}

	FGameplayEffectContextHandle EffectContext =
		Context.SourceAbilitySystem->MakeEffectContext();
	EffectContext.AddInstigator(Context.SourceActor, EffectCauser);
	if (SourceObject)
	{
		EffectContext.AddSourceObject(SourceObject);
	}
	if (Context.bHasHitResult)
	{
		EffectContext.AddHitResult(CueHitResult, true);
	}

	FGameplayCueParameters CueParameters(EffectContext);
	CueParameters.Instigator = Context.SourceActor;
	CueParameters.EffectCauser = EffectCauser;
	CueParameters.SourceObject = SourceObject;
	CueParameters.AbilityLevel = Context.Ability
		? Context.Ability->GetAbilityLevel()
		: 1;
	CueParameters.bReplicateLocationWhenUsingMinimalRepProxy = true;

	if (Context.bHasHitResult)
	{
		CueParameters.Location = CueHitResult.ImpactPoint;
		CueParameters.Normal = CueHitResult.ImpactNormal;
		CueParameters.PhysicalMaterial = CueHitResult.PhysMaterial.Get();
	}
	else
	{
		CueParameters.Location = ScopedActor->GetActorLocation();
		if (bHasCueDirection)
		{
			CueParameters.Normal = CueDirection;
		}
	}

	ScopedAbilitySystem->ExecuteGameplayCue(CueTag, CueParameters);
	return true;
}

FVector UKCExecuteGameplayCueFragment::ResolveDirection(
	const FKCActionExecutionContext& Context) const
{
	FVector Direction = FVector::ZeroVector;
	switch (DirectionMode)
	{
	case EKCGameplayCueDirectionMode::SourceForward:
		if (Context.SourceActor)
		{
			Direction = Context.SourceActor->GetActorForwardVector();
		}
		break;

	case EKCGameplayCueDirectionMode::SourceAim:
		if (const APawn* SourcePawn = Cast<APawn>(Context.SourceActor))
		{
			Direction = SourcePawn->GetBaseAimRotation().Vector();
		}
		else if (Context.SourceActor)
		{
			Direction = Context.SourceActor->GetActorForwardVector();
		}
		break;

	case EKCGameplayCueDirectionMode::SourceToTarget:
		if (Context.SourceActor && Context.TargetActor)
		{
			Direction = Context.TargetActor->GetActorLocation() -
				Context.SourceActor->GetActorLocation();
		}
		break;

	case EKCGameplayCueDirectionMode::InverseHitNormal:
		if (Context.bHasHitResult)
		{
			Direction = -Context.HitResult.ImpactNormal;
		}
		break;

	case EKCGameplayCueDirectionMode::FromContext:
	default:
		return FVector::ZeroVector;
	}

	if (bFlattenDirection)
	{
		Direction.Z = 0.0f;
	}

	if (!Direction.Normalize())
	{
		// 방향을 못 구했으면 소스 정면으로 물러선다.
		if (Context.SourceActor)
		{
			Direction = Context.SourceActor->GetActorForwardVector();
			if (bFlattenDirection)
			{
				Direction.Z = 0.0f;
			}
			if (Direction.Normalize())
			{
				return Direction;
			}
		}
		return FVector::ZeroVector;
	}

	return Direction;
}
