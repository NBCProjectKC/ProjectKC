#include "ProjectKC/Trap/KCTrapActorBase.h"

#include "Components/BoxComponent.h"
#include "TimerManager.h"

AKCTrapActorBase::AKCTrapActorBase()
{
	bReplicates = true;

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	SetRootComponent(Trigger);
	Trigger->SetCollisionProfileName(TEXT("Trigger"));
	Trigger->SetGenerateOverlapEvents(true);
}

void AKCTrapActorBase::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		return;
	}

	if (TriggerMode == EKCTrapTriggerMode::OnEnter ||
		TriggerMode == EKCTrapTriggerMode::OnEnterThenPeriodic)
	{
		Trigger->OnComponentBeginOverlap.AddDynamic(
			this,
			&AKCTrapActorBase::HandleTriggerBeginOverlap);
	}

	if (TriggerMode == EKCTrapTriggerMode::OnEnterThenPeriodic)
	{
		Trigger->OnComponentEndOverlap.AddDynamic(
			this,
			&AKCTrapActorBase::HandleTriggerEndOverlap);
	}
	else if (TriggerMode == EKCTrapTriggerMode::Periodic)
	{
		GetWorldTimerManager().SetTimer(
			PeriodicTimerHandle,
			this,
			&AKCTrapActorBase::HandlePeriodicTrigger,
			PeriodicInterval,
			true,
			PeriodicInterval);
	}
}

void AKCTrapActorBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(PeriodicTimerHandle);
	for (TPair<TWeakObjectPtr<AActor>, FTimerHandle>& Entry :
		OccupantPeriodicTimerHandles)
	{
		GetWorldTimerManager().ClearTimer(Entry.Value);
	}
	OccupantPeriodicTimerHandles.Reset();

	Super::EndPlay(EndPlayReason);
}

void AKCTrapActorBase::ExecuteTrap_Implementation(
	const FKCTrapTriggerContext& Context)
{
}

void AKCTrapActorBase::HandlePeriodicTrigger()
{
	if (!HasAuthority())
	{
		return;
	}

	FKCTrapTriggerContext Context;
	Context.Cause = EKCTrapTriggerCause::Periodic;
	ExecuteTrap(Context);
}

void AKCTrapActorBase::HandleTriggerBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!HasAuthority() ||
		(TriggerMode != EKCTrapTriggerMode::OnEnter &&
			TriggerMode != EKCTrapTriggerMode::OnEnterThenPeriodic) ||
		!OtherActor || OtherActor == this)
	{
		return;
	}

	// 상호작용 Sphere 같은 보조 컴포넌트만 먼저 닿은 것은 Actor 진입이 아니다.
	if (!IsActorInsideTrigger(*OtherActor))
	{
		return;
	}

	if (TriggerMode == EKCTrapTriggerMode::OnEnterThenPeriodic)
	{
		StartOccupantPeriodicTrigger(
			*OtherActor,
			bFromSweep ? &SweepResult : nullptr);
		return;
	}

	FKCTrapTriggerContext Context;
	Context.Cause = EKCTrapTriggerCause::OnEnter;
	Context.TargetActor = OtherActor;
	Context.bHasHitResult = bFromSweep;
	if (bFromSweep)
	{
		Context.HitResult = SweepResult;
	}
	ExecuteTrap(Context);
}

void AKCTrapActorBase::HandleTriggerEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex)
{
	if (!HasAuthority() ||
		TriggerMode != EKCTrapTriggerMode::OnEnterThenPeriodic ||
		!OtherActor || OtherActor == this)
	{
		return;
	}

	// 다른 보조 컴포넌트가 먼저 빠져나가도 Root 충돌이 남아 있으면 영역 내부다.
	if (IsActorInsideTrigger(*OtherActor))
	{
		return;
	}

	StopOccupantPeriodicTrigger(OtherActor);
}

bool AKCTrapActorBase::IsActorInsideTrigger(const AActor& Actor) const
{
	if (!Trigger)
	{
		return false;
	}

	if (const UPrimitiveComponent* RootPrimitive =
		Cast<UPrimitiveComponent>(Actor.GetRootComponent()))
	{
		return Trigger->IsOverlappingComponent(RootPrimitive);
	}

	return Trigger->IsOverlappingActor(&Actor);
}

void AKCTrapActorBase::StartOccupantPeriodicTrigger(
	AActor& TargetActor,
	const FHitResult* HitResult)
{
	const TWeakObjectPtr<AActor> TargetKey(&TargetActor);
	if (OccupantPeriodicTimerHandles.Contains(TargetKey))
	{
		return;
	}

	FTimerHandle& TimerHandle =
		OccupantPeriodicTimerHandles.Add(TargetKey);
	FTimerDelegate TimerDelegate = FTimerDelegate::CreateUObject(
		this,
		&AKCTrapActorBase::HandleOccupantPeriodicTrigger,
		TargetKey);
	GetWorldTimerManager().SetTimer(
		TimerHandle,
		TimerDelegate,
		PeriodicInterval,
		true,
		PeriodicInterval);

	FKCTrapTriggerContext Context;
	Context.Cause = EKCTrapTriggerCause::OnEnter;
	Context.TargetActor = &TargetActor;
	Context.bHasHitResult = HitResult != nullptr;
	if (HitResult)
	{
		Context.HitResult = *HitResult;
	}
	ExecuteTrap(Context);
}

void AKCTrapActorBase::HandleOccupantPeriodicTrigger(
	TWeakObjectPtr<AActor> TargetActor)
{
	AActor* ResolvedTarget = TargetActor.Get();
	if (!HasAuthority() || !IsValid(ResolvedTarget) ||
		!IsActorInsideTrigger(*ResolvedTarget))
	{
		StopOccupantPeriodicTrigger(TargetActor);
		return;
	}

	FKCTrapTriggerContext Context;
	Context.Cause = EKCTrapTriggerCause::OccupantPeriodic;
	Context.TargetActor = ResolvedTarget;
	ExecuteTrap(Context);
}

void AKCTrapActorBase::StopOccupantPeriodicTrigger(
	TWeakObjectPtr<AActor> TargetActor)
{
	if (FTimerHandle* TimerHandle =
		OccupantPeriodicTimerHandles.Find(TargetActor))
	{
		GetWorldTimerManager().ClearTimer(*TimerHandle);
		OccupantPeriodicTimerHandles.Remove(TargetActor);
	}
}
