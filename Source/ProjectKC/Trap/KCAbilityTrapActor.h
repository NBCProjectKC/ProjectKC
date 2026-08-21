#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Actor.h"
#include "KCAbilityTrapActor.generated.h"

class UBoxComponent;
class UPrimitiveComponent;
class UKCAbilityDefinition;
class UKCAbilitySourceComponent;
class UKCAbilitySystemComponent;

/** 함정이 언제 발동을 거는가. 무엇이 일어나는지는 Definition이 정한다. */
UENUM(BlueprintType)
enum class EKCTrapTriggerMode : uint8
{
	/** 트리거에 들어온 대상을 지목해 한 번 발동한다. */
	OnEnter,

	/** 일정 간격으로 발동한다. 대상 수집은 Targeting이 맡는다. */
	Periodic
};

/** 아이템이 아닌 월드 소스도 동일한 GA+GE 파이프라인을 쓰는 최소 함정 예제다. */
UCLASS(Blueprintable)
class PROJECTKC_API AKCAbilityTrapActor
	: public AActor
	, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AKCAbilityTrapActor();

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(
		FDataValidationContext& Context) const override;
#endif

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

protected:
	virtual void BeginPlay() override;

	/** Periodic 모드에서 간격마다 호출한다. */
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Trap")
	EKCTrapTriggerMode TriggerMode = EKCTrapTriggerMode::OnEnter;

	/** Periodic 모드의 발동 간격이다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "KC|Trap",
		meta = (
			EditCondition = "TriggerMode == EKCTrapTriggerMode::Periodic",
			ClampMin = "0.05",
			Units = "s"))
	float PeriodicInterval = 0.6f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|Trap")
	TObjectPtr<UBoxComponent> Trigger;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|Ability")
	TObjectPtr<UKCAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|Ability")
	TObjectPtr<UKCAbilitySourceComponent> AbilitySourceComponent;

private:
	FTimerHandle PeriodicTimerHandle;
};
