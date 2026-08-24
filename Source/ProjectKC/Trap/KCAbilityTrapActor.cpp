#include "ProjectKC/Trap/KCAbilityTrapActor.h"

#include "ProjectKC/AbilitySystem/Component/KCAbilitySourceComponent.h"
#include "ProjectKC/AbilitySystem/Component/KCAbilitySystemComponent.h"
#include "ProjectKC/AbilitySystem/Definition/KCAbilityDefinition.h"
#include "Misc/DataValidation.h"

AKCAbilityTrapActor::AKCAbilityTrapActor()
{
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
}

void AKCAbilityTrapActor::ExecuteTrap_Implementation(
	const FKCTrapTriggerContext& Context)
{
	if (Context.TargetActor)
	{
		if (Context.bHasHitResult)
		{
			AbilitySourceComponent->TryActivateWithHitResult(
				Context.TargetActor.Get(),
				Context.HitResult);
		}
		else
		{
			AbilitySourceComponent->TryActivateWithTarget(
				Context.TargetActor.Get());
		}
		return;
	}

	// TargetActor가 없는 전역 Periodic은 Definition의 Targeting이 대상을 수집한다.
	AbilitySourceComponent->TryActivate();
}
