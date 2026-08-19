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

/** 아이템이 아닌 월드 소스도 동일한 GA+GE 파이프라인을 쓰는 최소 함정 예제다. */
UCLASS(Blueprintable)
class PROJECTKC_API AKCAbilityTrapActor
	: public AActor
	, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AKCAbilityTrapActor();
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(
		FDataValidationContext& Context) const override;
#endif

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void HandleTriggerBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|Trap")
	TObjectPtr<UBoxComponent> Trigger;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|Ability")
	TObjectPtr<UKCAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|Ability")
	TObjectPtr<UKCAbilitySourceComponent> AbilitySourceComponent;

	UPROPERTY(
		EditDefaultsOnly,
		Instanced,
		BlueprintReadOnly,
		Category = "KC|Ability")
	TObjectPtr<UKCAbilityDefinition> ActionDefinition;
};
