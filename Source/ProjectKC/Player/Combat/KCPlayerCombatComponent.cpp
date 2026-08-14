#include "Player/Combat/KCPlayerCombatComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Player/KCPlayerCharacter.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

UKCPlayerCombatComponent::UKCPlayerCombatComponent()
{
	SetIsReplicatedByDefault(true);

	static ConstructorHelpers::FObjectFinder<UAnimSequence> DefaultAttackAnimation(
		TEXT("/Game/Assets/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01.MM_Attack_01"));
	if (DefaultAttackAnimation.Succeeded())
	{
		AttackAnimation = DefaultAttackAnimation.Object;
	}

	static ConstructorHelpers::FObjectFinder<UAnimSequence> DefaultHitAnimation(
		TEXT("/Game/Assets/Characters/Mannequins/Anims/Death/MM_Death_Front_01.MM_Death_Front_01"));
	if (DefaultHitAnimation.Succeeded())
	{
		HitAnimation = DefaultHitAnimation.Object;
	}
}

void UKCPlayerCombatComponent::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UKCPlayerCombatComponent, bWeaponAttackEnabled);
	DOREPLIFETIME(UKCPlayerCombatComponent, bIsAttacking);
}

void UKCPlayerCombatComponent::TryAttack()
{
	AKCPlayerCharacter* OwnerCharacter = Cast<AKCPlayerCharacter>(GetOwner());
	if (!OwnerCharacter || !OwnerCharacter->IsLocallyControlled())
	{
		return;
	}

	if (OwnerCharacter->HasAuthority())
	{
		PerformAttack();
	}
	else
	{
		ServerTryAttack();
	}
}

void UKCPlayerCombatComponent::HandleAttackHitNotify()
{
	AKCPlayerCharacter* OwnerCharacter = Cast<AKCPlayerCharacter>(GetOwner());
	if (!OwnerCharacter
		|| !OwnerCharacter->HasAuthority()
		|| !bIsAttacking
		|| bAttackHitConsumed)
	{
		return;
	}

	bAttackHitConsumed = true;
	TraceAttack(*OwnerCharacter);
}

void UKCPlayerCombatComponent::SetWeaponAttackEnabled(const bool bEnabled)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		bWeaponAttackEnabled = bEnabled;
	}
}

float UKCPlayerCombatComponent::HandleDamage(
	const float DamageAmount,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	AKCPlayerCharacter* OwnerCharacter = Cast<AKCPlayerCharacter>(GetOwner());
	if (!OwnerCharacter || !OwnerCharacter->HasAuthority() || DamageAmount <= 0.0f)
	{
		return 0.0f;
	}

	APawn* AttackerPawn = EventInstigator ? EventInstigator->GetPawn() : nullptr;
	if (!AttackerPawn)
	{
		AttackerPawn = Cast<APawn>(DamageCauser);
	}

	FVector KnockbackDirection = FVector::ZeroVector;
	if (AttackerPawn)
	{
		KnockbackDirection =
			(OwnerCharacter->GetActorLocation() - AttackerPawn->GetActorLocation()).GetSafeNormal2D();
	}

	if (KnockbackDirection.IsNearlyZero())
	{
		KnockbackDirection = -OwnerCharacter->GetActorForwardVector().GetSafeNormal2D();
	}

	const FVector KnockbackVelocity =
		KnockbackDirection * HorizontalKnockbackStrength
		+ FVector::UpVector * VerticalKnockbackStrength;

	OnRequestDropHeldItem.Broadcast();
	MulticastPlayHitFeedback(AttackerPawn, KnockbackVelocity);
	OwnerCharacter->LaunchCharacter(KnockbackVelocity, true, true);

	return DamageAmount;
}

bool UKCPlayerCombatComponent::IsAttacking() const
{
	return bIsAttacking;
}

bool UKCPlayerCombatComponent::IsWeaponAttackEnabled() const
{
	return bWeaponAttackEnabled;
}

void UKCPlayerCombatComponent::ServerTryAttack_Implementation()
{
	PerformAttack();
}

void UKCPlayerCombatComponent::OnRep_IsAttacking()
{
	if (bIsAttacking)
	{
		PlayAttackAnimation();
		OnAttackStarted.Broadcast();
	}
}

void UKCPlayerCombatComponent::MulticastPlayHitFeedback_Implementation(
	APawn* AttackerPawn,
	const FVector KnockbackVelocity)
{
	PlayHitAnimation();
	OnHitReceived.Broadcast(AttackerPawn, KnockbackVelocity);
}

void UKCPlayerCombatComponent::PerformAttack()
{
	AKCPlayerCharacter* OwnerCharacter = Cast<AKCPlayerCharacter>(GetOwner());
	UWorld* World = GetWorld();
	if (!OwnerCharacter || !OwnerCharacter->HasAuthority() || !World)
	{
		return;
	}

	const float ServerTime = World->GetTimeSeconds();
	if (!bWeaponAttackEnabled
		|| bIsAttacking
		|| ServerTime - LastAttackServerTime < AttackCooldown)
	{
		return;
	}

	LastAttackServerTime = ServerTime;
	bIsAttacking = true;
	bAttackHitConsumed = false;
	PlayAttackAnimation();
	OnAttackStarted.Broadcast();
	OwnerCharacter->ForceNetUpdate();

	const float EffectiveAttackStateDuration = AttackAnimation
		? FMath::Max(AttackStateDuration, AttackAnimation->GetPlayLength())
		: AttackStateDuration;

	World->GetTimerManager().SetTimer(
		AttackStateTimerHandle,
		this,
		&UKCPlayerCombatComponent::FinishAttack,
		EffectiveAttackStateDuration,
		false);
}

void UKCPlayerCombatComponent::FinishAttack()
{
	bIsAttacking = false;
	bAttackHitConsumed = true;

	if (AActor* OwnerActor = GetOwner())
	{
		OwnerActor->ForceNetUpdate();
	}
}

void UKCPlayerCombatComponent::TraceAttack(AKCPlayerCharacter& OwnerCharacter)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector AttackDirection = OwnerCharacter.GetActorForwardVector().GetSafeNormal2D();
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(KCPlayerAttack), false, &OwnerCharacter);
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	TArray<FOverlapResult> OverlapResults;
	World->OverlapMultiByObjectType(
		OverlapResults,
		OwnerCharacter.GetActorLocation(),
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(AttackRange),
		QueryParams);

	const float MinimumFacingDot = FMath::Cos(FMath::DegreesToRadians(AttackHalfAngleDegrees));
	AKCPlayerCharacter* HitCharacter = nullptr;
	float NearestDistanceSquared = TNumericLimits<float>::Max();

	for (const FOverlapResult& OverlapResult : OverlapResults)
	{
		AKCPlayerCharacter* CandidateCharacter =
			Cast<AKCPlayerCharacter>(OverlapResult.GetActor());
		if (!CandidateCharacter || CandidateCharacter == &OwnerCharacter)
		{
			continue;
		}

		const FVector DirectionToCandidate =
			(CandidateCharacter->GetActorLocation() - OwnerCharacter.GetActorLocation()).GetSafeNormal2D();
		const float FacingDot = FVector::DotProduct(AttackDirection, DirectionToCandidate);
		if (FacingDot < MinimumFacingDot)
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared2D(
			OwnerCharacter.GetActorLocation(), CandidateCharacter->GetActorLocation());
		if (DistanceSquared < NearestDistanceSquared)
		{
			HitCharacter = CandidateCharacter;
			NearestDistanceSquared = DistanceSquared;
		}
	}

	if (!HitCharacter)
	{
		return;
	}

	const FHitResult HitResult(
		HitCharacter,
		nullptr,
		HitCharacter->GetActorLocation(),
		-AttackDirection);

	UGameplayStatics::ApplyPointDamage(
		HitCharacter,
		1.0f,
		AttackDirection,
		HitResult,
		OwnerCharacter.GetController(),
		&OwnerCharacter,
		UDamageType::StaticClass());
}

void UKCPlayerCombatComponent::PlayAttackAnimation()
{
	AKCPlayerCharacter* OwnerCharacter = Cast<AKCPlayerCharacter>(GetOwner());
	UAnimInstance* AnimInstance = OwnerCharacter && OwnerCharacter->GetMesh()
		? OwnerCharacter->GetMesh()->GetAnimInstance()
		: nullptr;
	if (AnimInstance && AttackAnimation)
	{
		AnimInstance->PlaySlotAnimationAsDynamicMontage(
			AttackAnimation, AttackAnimationSlotName, 0.1f, 0.1f);
	}
}

void UKCPlayerCombatComponent::PlayHitAnimation()
{
	AKCPlayerCharacter* OwnerCharacter = Cast<AKCPlayerCharacter>(GetOwner());
	UAnimInstance* AnimInstance = OwnerCharacter && OwnerCharacter->GetMesh()
		? OwnerCharacter->GetMesh()->GetAnimInstance()
		: nullptr;
	if (AnimInstance && HitAnimation)
	{
		AnimInstance->PlaySlotAnimationAsDynamicMontage(
			HitAnimation, HitAnimationSlotName, 0.05f, 0.15f);
	}
}
