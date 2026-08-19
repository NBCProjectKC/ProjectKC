#include "ProjectKC/AbilitySystem/Ability/KCGAMeleeSwing.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "CollisionQueryParams.h"
#include "Components/ActorComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "ProjectKC/AbilitySystem/Config/KCMeleeActionConfig.h"
#include "ProjectKC/AbilitySystem/Tag/KCGameplayTags.h"

DEFINE_LOG_CATEGORY_STATIC(LogKCMeleeSwing, Log, All);

namespace KCMeleeSwing
{
	void AddIgnoredSourceActors(
		FCollisionQueryParams& QueryParams,
		const AActor& SourceActor,
		const UObject* SourceObject)
	{
		QueryParams.AddIgnoredActor(&SourceActor);

		const AActor* SourceObjectActor = Cast<AActor>(SourceObject);
		if (!SourceObjectActor)
		{
			const UActorComponent* SourceComponent =
				Cast<UActorComponent>(SourceObject);
			SourceObjectActor = SourceComponent
				? SourceComponent->GetOwner()
				: nullptr;
		}

		if (SourceObjectActor && SourceObjectActor != &SourceActor)
		{
			QueryParams.AddIgnoredActor(SourceObjectActor);
		}
	}

	bool IsOwnedOrAttachedTo(
		const AActor& Candidate,
		const AActor& PossibleParent)
	{
		return Candidate.IsOwnedBy(&PossibleParent) ||
			Candidate.IsAttachedTo(&PossibleParent);
	}
}

UKCGAMeleeSwing::UKCGAMeleeSwing()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	SetSupportedActionConfigClass(UKCMeleeActionConfig::StaticClass(), true);
	AddRequiredActionHook(TAG_KC_ActionHook_Target_OnHit);
}

void UKCGAMeleeSwing::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	if (!IsActive())
	{
		return;
	}

	AActor* SourceActor = GetAvatarActorFromActorInfo();
	const UKCMeleeActionConfig* Config =
		Cast<UKCMeleeActionConfig>(GetActiveActionConfig());
	if (!SourceActor || !SourceActor->HasAuthority() || !Config)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 공격 시도 자체에 Cost/Cooldown이 소모되어야 하므로 명중 판정보다 먼저 확정한다.
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	TArray<FHitResult> HitResults;
	GatherHitResults(*Config, *SourceActor, HitResults);

	int32 ExecutedTargetCount = 0;
	for (const FHitResult& HitResult : HitResults)
	{
		AActor* TargetActor = HitResult.GetActor();
		if (!IsValid(TargetActor) ||
			!IsPathUnobstructed(
				*Config,
				*SourceActor,
				*TargetActor))
		{
			continue;
		}

		UAbilitySystemComponent* TargetAbilitySystem =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(
				TargetActor);
		if (!ExecuteActionHook(
			TAG_KC_ActionHook_Target_OnHit,
			TargetAbilitySystem,
			TargetActor,
			&HitResult))
		{
			UE_LOG(
				LogKCMeleeSwing,
				Verbose,
				TEXT("Melee Hook을 실행할 수 없는 대상을 건너뜁니다. Source='%s', Target='%s'"),
				*GetNameSafe(SourceActor),
				*GetNameSafe(TargetActor));
			continue;
		}

		++ExecutedTargetCount;
		if (ExecutedTargetCount >= Config->MaxTargets)
		{
			break;
		}
	}

	// 빗나간 공격도 정상적으로 실행된 한 번의 공격이다.
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UKCGAMeleeSwing::GatherHitResults(
	const UKCMeleeActionConfig& Config,
	AActor& SourceActor,
	TArray<FHitResult>& OutHits) const
{
	OutHits.Reset();
	UWorld* World = SourceActor.GetWorld();
	if (!World)
	{
		return;
	}

	FVector Forward = SourceActor.GetActorForwardVector();
	Forward.Z = 0.0f;
	if (!Forward.Normalize())
	{
		return;
	}

	const FVector Start = SourceActor.GetActorLocation() +
		Forward * Config.StartForwardOffset +
		FVector::UpVector * Config.HeightOffset;
	const FVector End = Start + Forward * Config.SweepDistance;

	FCollisionQueryParams QueryParams(
		SCENE_QUERY_STAT(KCMeleeSwing),
		false);
	KCMeleeSwing::AddIgnoredSourceActors(
		QueryParams,
		SourceActor,
		GetCurrentSourceObject());

	TArray<FHitResult> RawHits;
	const FCollisionObjectQueryParams ObjectQuery(Config.TargetObjectTypes);
	World->SweepMultiByObjectType(
		RawHits,
		Start,
		End,
		FQuat::Identity,
		ObjectQuery,
		FCollisionShape::MakeSphere(Config.SweepRadius),
		QueryParams);

	RawHits.Sort(
		[&SourceActor](const FHitResult& Left, const FHitResult& Right)
		{
			const float LeftDistance = FMath::IsFinite(Left.Distance)
				? Left.Distance
				: FVector::DistSquared(
					SourceActor.GetActorLocation(),
					Left.ImpactPoint);
			const float RightDistance = FMath::IsFinite(Right.Distance)
				? Right.Distance
				: FVector::DistSquared(
					SourceActor.GetActorLocation(),
					Right.ImpactPoint);
			if (!FMath::IsNearlyEqual(LeftDistance, RightDistance))
			{
				return LeftDistance < RightDistance;
			}

			const AActor* LeftActor = Left.GetActor();
			const AActor* RightActor = Right.GetActor();
			const uint32 LeftId = LeftActor
				? LeftActor->GetUniqueID()
				: MAX_uint32;
			const uint32 RightId = RightActor
				? RightActor->GetUniqueID()
				: MAX_uint32;
			return LeftId < RightId;
		});

	TSet<AActor*> SeenTargets;
	for (const FHitResult& HitResult : RawHits)
	{
		AActor* TargetActor = HitResult.GetActor();
		if (!IsValid(TargetActor) || TargetActor == &SourceActor ||
			SeenTargets.Contains(TargetActor) ||
			KCMeleeSwing::IsOwnedOrAttachedTo(*TargetActor, SourceActor))
		{
			continue;
		}

		SeenTargets.Add(TargetActor);
		OutHits.Add(HitResult);
	}
}

bool UKCGAMeleeSwing::IsPathUnobstructed(
	const UKCMeleeActionConfig& Config,
	const AActor& SourceActor,
	const AActor& TargetActor) const
{
	if (!Config.bRequireUnobstructedPath)
	{
		return true;
	}

	UWorld* World = SourceActor.GetWorld();
	if (!World)
	{
		return false;
	}

	const FVector Start = SourceActor.GetActorLocation() +
		FVector::UpVector * Config.HeightOffset;
	const FVector End = TargetActor.GetActorLocation();

	FCollisionQueryParams QueryParams(
		SCENE_QUERY_STAT(KCMeleeObstruction),
		false);
	KCMeleeSwing::AddIgnoredSourceActors(
		QueryParams,
		SourceActor,
		GetCurrentSourceObject());

	FHitResult ObstructionHit;
	const ECollisionChannel Channel =
		UEngineTypes::ConvertToCollisionChannel(
			Config.ObstructionTraceChannel.GetValue());
	if (!World->LineTraceSingleByChannel(
		ObstructionHit,
		Start,
		End,
		Channel,
		QueryParams))
	{
		return true;
	}

	const AActor* ObstructionActor = ObstructionHit.GetActor();
	return ObstructionActor == &TargetActor ||
		(ObstructionActor &&
			KCMeleeSwing::IsOwnedOrAttachedTo(
				*ObstructionActor,
				TargetActor));
}
