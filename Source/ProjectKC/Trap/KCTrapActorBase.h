#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KCTrapActorBase.generated.h"

class UBoxComponent;
class UPrimitiveComponent;

/** 함정이 언제 발동하는가. 실제 효과 실행 방식과는 독립적이다. */
UENUM(BlueprintType)
enum class EKCTrapTriggerMode : uint8
{
	/** 트리거에 들어온 대상을 지목해 한 번 발동한다. */
	OnEnter,

	/** 일정 간격으로 발동한다. 대상 수집은 실행 로직이 맡는다. */
	Periodic,

	/** 대상별로 진입 즉시 발동하고, 영역 안에 있는 동안 진입 시점을 기준으로 반복한다. */
	OnEnterThenPeriodic
};

/** ExecuteTrap이 호출된 원인이다. */
UENUM(BlueprintType)
enum class EKCTrapTriggerCause : uint8
{
	/** 대상이 트리거에 진입했다. */
	OnEnter,

	/** 함정 자체의 전역 주기가 도래했다. TargetActor는 비어 있다. */
	Periodic,

	/** OnEnterThenPeriodic 대상의 개별 주기가 도래했다. */
	OccupantPeriodic
};

/** 공통 함정 스케줄러가 실제 실행 로직에 전달하는 정보다. */
USTRUCT(BlueprintType)
struct PROJECTKC_API FKCTrapTriggerContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "KC|Trap")
	EKCTrapTriggerCause Cause = EKCTrapTriggerCause::OnEnter;

	/** OnEnter 및 OccupantPeriodic에서 발동 대상이다. */
	UPROPERTY(BlueprintReadOnly, Category = "KC|Trap")
	TObjectPtr<AActor> TargetActor = nullptr;

	/** Sweep 진입으로 얻은 HitResult가 유효한지 나타낸다. */
	UPROPERTY(BlueprintReadOnly, Category = "KC|Trap")
	bool bHasHitResult = false;

	UPROPERTY(BlueprintReadOnly, Category = "KC|Trap")
	FHitResult HitResult;
};

/**
 * 모든 함정의 공통 부모다.
 * 트리거와 발동 주기만 관리하며, 실제 효과는 ExecuteTrap 구현이 정한다.
 */
UCLASS(Blueprintable)
class PROJECTKC_API AKCTrapActorBase : public AActor
{
	GENERATED_BODY()

public:
	AKCTrapActorBase();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
	 * 발동 시점이 도래했을 때 서버에서 호출된다.
	 * GAS를 쓰지 않는 Blueprint 함정은 이 이벤트만 구현하면 된다.
	 */
	UFUNCTION(
		BlueprintNativeEvent,
		BlueprintAuthorityOnly,
		Category = "KC|Trap",
		meta = (DisplayName = "Execute Trap"))
	void ExecuteTrap(const FKCTrapTriggerContext& Context);
	virtual void ExecuteTrap_Implementation(
		const FKCTrapTriggerContext& Context);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KC|Trap")
	EKCTrapTriggerMode TriggerMode = EKCTrapTriggerMode::OnEnter;

	/** Periodic 및 OnEnterThenPeriodic 모드의 발동 간격이다. */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "KC|Trap",
		meta = (
			EditCondition = "TriggerMode != EKCTrapTriggerMode::OnEnter",
			ClampMin = "0.05",
			Units = "s"))
	float PeriodicInterval = 0.6f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|Trap")
	TObjectPtr<UBoxComponent> Trigger;

private:
	UFUNCTION()
	void HandlePeriodicTrigger();

	UFUNCTION()
	void HandleTriggerBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void HandleTriggerEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex);

	bool IsActorInsideTrigger(const AActor& Actor) const;
	void StartOccupantPeriodicTrigger(
		AActor& TargetActor,
		const FHitResult* HitResult);
	void HandleOccupantPeriodicTrigger(TWeakObjectPtr<AActor> TargetActor);
	void StopOccupantPeriodicTrigger(TWeakObjectPtr<AActor> TargetActor);

	FTimerHandle PeriodicTimerHandle;
	TMap<TWeakObjectPtr<AActor>, FTimerHandle> OccupantPeriodicTimerHandles;
};
