#include "ProjectKC/Trap/KCAbilityTrapActor.h"

#include "ProjectKC/AbilitySystem/Component/KCAbilitySourceComponent.h"
#include "ProjectKC/AbilitySystem/Component/KCAbilitySystemComponent.h"
#include "ProjectKC/AbilitySystem/Definition/KCAbilityDefinition.h"
#include "Components/BoxComponent.h"
#include "Misc/DataValidation.h"

/**
 * @brief Creates and configures the trap actor's trigger, ability system, and ability source components.
 */
AKCAbilityTrapActor::AKCAbilityTrapActor()
{
	bReplicates = true;

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	SetRootComponent(Trigger);
	Trigger->SetCollisionProfileName(TEXT("Trigger"));
	Trigger->SetGenerateOverlapEvents(true);

	AbilitySystemComponent =
		CreateDefaultSubobject<UKCAbilitySystemComponent>(TEXT("AbilitySystem"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(
		EGameplayEffectReplicationMode::Minimal);

	AbilitySourceComponent =
		CreateDefaultSubobject<UKCAbilitySourceComponent>(TEXT("AbilitySource"));
}

/**
 * @brief Configures the ability source with the assigned action definition.
 *
 * @param Transform The actor's construction transform.
 */
void AKCAbilityTrapActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	AbilitySourceComponent->ConfigureAbilityDefinition(ActionDefinition);
}

#if WITH_EDITOR
/**
 * @brief Validates the configured action definition and inherited actor data.
 *
 * @return EDataValidationResult Invalid when the action definition is missing or
 *         fails its action contract; otherwise the superclass result, or Valid
 *         when the superclass does not perform validation.
 */
EDataValidationResult AKCAbilityTrapActor::IsDataValid(
	FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	FString Error;
	if (!ActionDefinition ||
		!ActionDefinition->ValidateWithActionContract(Error))
	{
		Context.AddError(ActionDefinition
			? FText::FromString(Error)
			: FText::FromString(TEXT("ActionDefinition이 비어 있습니다.")));
		return EDataValidationResult::Invalid;
	}

	return Result == EDataValidationResult::NotValidated
		? EDataValidationResult::Valid
		: Result;
}
#endif

/**
 * @brief Retrieves the trap actor's ability system component.
 *
 * @return UAbilitySystemComponent* The actor's ability system component.
 */
UAbilitySystemComponent* AKCAbilityTrapActor::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

/**
 * @brief Initializes the ability source, ability system actor information, and trigger overlap handling when play begins.
 */
void AKCAbilityTrapActor::BeginPlay()
{
	Super::BeginPlay();

	AbilitySourceComponent->ConfigureAbilityDefinition(ActionDefinition);
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	Trigger->OnComponentBeginOverlap.AddDynamic(
		this,
		&AKCAbilityTrapActor::HandleTriggerBeginOverlap);

	if (HasAuthority())
	{
		AbilitySourceComponent->GrantToAbilitySystem(AbilitySystemComponent);
	}
}

/**
 * @brief Revokes the trap's granted abilities during actor teardown.
 */
void AKCAbilityTrapActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority())
	{
		AbilitySourceComponent->Revoke(true);
	}

	Super::EndPlay(EndPlayReason);
}

/**
 * @brief Activates the trap ability source when another actor enters the trigger.
 *
 * @param OtherActor Actor that entered the trigger.
 * @param bFromSweep Whether the overlap includes a sweep hit result.
 * @param SweepResult Hit result associated with a sweep overlap.
 */
void AKCAbilityTrapActor::HandleTriggerBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (HasAuthority() && OtherActor && OtherActor != this)
	{
		if (bFromSweep)
		{
			AbilitySourceComponent->TryActivateWithHitResult(
				OtherActor,
				SweepResult);
		}
		else
		{
			AbilitySourceComponent->TryActivateWithTarget(OtherActor);
		}
	}
}
