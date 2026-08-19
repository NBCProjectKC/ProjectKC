#include "ProjectKC/Trap/KCAbilityTrapActor.h"

#include "ProjectKC/AbilitySystem/Component/KCAbilitySourceComponent.h"
#include "ProjectKC/AbilitySystem/Component/KCAbilitySystemComponent.h"
#include "ProjectKC/AbilitySystem/Definition/KCAbilityDefinition.h"
#include "Components/BoxComponent.h"
#include "Misc/DataValidation.h"

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

void AKCAbilityTrapActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	AbilitySourceComponent->ConfigureAbilityDefinition(ActionDefinition);
}

#if WITH_EDITOR
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

UAbilitySystemComponent* AKCAbilityTrapActor::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

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

void AKCAbilityTrapActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority())
	{
		AbilitySourceComponent->Revoke(true);
	}

	Super::EndPlay(EndPlayReason);
}

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
