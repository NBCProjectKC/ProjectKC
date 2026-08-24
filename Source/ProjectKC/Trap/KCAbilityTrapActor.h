#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "ProjectKC/Trap/KCTrapActorBase.h"
#include "KCAbilityTrapActor.generated.h"

class UKCAbilityDefinition;
class UKCAbilitySourceComponent;
class UKCAbilitySystemComponent;

/** 아이템이 아닌 월드 소스도 동일한 GA+GE 파이프라인을 쓰는 최소 함정 예제다. */
UCLASS(Blueprintable)
class PROJECTKC_API AKCAbilityTrapActor
	: public AKCTrapActorBase
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

	virtual void ExecuteTrap_Implementation(
		const FKCTrapTriggerContext& Context) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|Ability")
	TObjectPtr<UKCAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|Ability")
	TObjectPtr<UKCAbilitySourceComponent> AbilitySourceComponent;
};
