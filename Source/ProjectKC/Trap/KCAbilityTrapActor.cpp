#include "ProjectKC/Trap/KCAbilityTrapActor.h"

#include "ProjectKC/AbilitySystem/Component/KCAbilitySourceComponent.h"
#include "ProjectKC/AbilitySystem/Component/KCAbilitySystemComponent.h"
#include "ProjectKC/AbilitySystem/Definition/KCAbilityDefinition.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
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

#if WITH_EDITOR
EDataValidationResult AKCAbilityTrapActor::IsDataValid(
	FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	FString Error;
	const UKCAbilityDefinition* Definition =
		AbilitySourceComponent->GetActionDefinition();
	if (!Definition || !Definition->ValidateWithActionContract(Error))
	{
		Context.AddError(Definition
			? FText::FromString(Error)
			: FText::FromString(
				TEXT("AbilitySource 컴포넌트의 ActionDefinition이 비어 있습니다.")));
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

	AbilitySystemComponent->InitAbilityActorInfo(this, this);

	// 회수는 AbilitySourceComponent가 자기 EndPlay에서 처리한다.
	AbilitySourceComponent->GrantToOwner();

	// 들어온 순간의 반응은 두 모드 모두 필요하다.
	Trigger->OnComponentBeginOverlap.AddDynamic(
		this,
		&AKCAbilityTrapActor::HandleTriggerBeginOverlap);

	if (TriggerMode == EKCTrapTriggerMode::Periodic && HasAuthority())
	{
		GetWorldTimerManager().SetTimer(
			PeriodicTimerHandle,
			this,
			&AKCAbilityTrapActor::HandlePeriodicTrigger,
			PeriodicInterval,
			true);
	}
}

void AKCAbilityTrapActor::HandlePeriodicTrigger()
{
	if (!HasAuthority())
	{
		return;
	}

	// 대상 수집은 Targeting이 한다. 겹친 대상이 없으면 아무 일도 일어나지 않는다.
	AbilitySourceComponent->TryActivate();
}

void AKCAbilityTrapActor::HandleTriggerBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!HasAuthority() || !OtherActor || OtherActor == this)
	{
		return;
	}

	// Periodic 모드는 대상을 지목하지 않는다. 간격을 기다리지 않고 즉시 한 번 돌린다.
	if (TriggerMode == EKCTrapTriggerMode::Periodic)
	{
		AbilitySourceComponent->TryActivate();
		return;
	}

	if (bFromSweep)
	{
		AbilitySourceComponent->TryActivateWithHitResult(OtherActor, SweepResult);
	}
	else
	{
		AbilitySourceComponent->TryActivateWithTarget(OtherActor);
	}
}
