#include "ProjectKC/AbilitySystem/Fragment/KCExecuteGameplayCueFragment.h"

#include "AbilitySystemComponent.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "ProjectKC/AbilitySystem/Ability/KCGA_Base.h"
#include "ProjectKC/AbilitySystem/Fragment/KCActionExecutionContext.h"

bool UKCExecuteGameplayCueFragment::Validate(FString& OutError) const
{
	OutError.Reset();
	if (!CueTag.IsValid())
	{
		OutError = TEXT("CueTag가 비어 있습니다.");
		return false;
	}

	const FGameplayTag GameplayCueRoot =
		FGameplayTag::RequestGameplayTag(TEXT("GameplayCue"), false);
	if (!GameplayCueRoot.IsValid() || !CueTag.MatchesTag(GameplayCueRoot))
	{
		OutError = TEXT("CueTag는 GameplayCue 하위 태그여야 합니다.");
		return false;
	}

	return true;
}

bool UKCExecuteGameplayCueFragment::CanExecute(
	const FKCActionExecutionContext& Context,
	FString& OutError) const
{
	OutError.Reset();
	if (!Context.IsAuthoritative() ||
		!Context.ResolveScopedAbilitySystem(ApplicationScope) ||
		!IsValid(Context.ResolveScopedActor(ApplicationScope)))
	{
		OutError = TEXT("GameplayCue를 실행할 권한, ASC 또는 대상 Actor가 없습니다.");
		return false;
	}

	return true;
}

bool UKCExecuteGameplayCueFragment::Execute(
	const FKCActionExecutionContext& Context) const
{
	UAbilitySystemComponent* ScopedAbilitySystem =
		Context.ResolveScopedAbilitySystem(ApplicationScope);
	AActor* ScopedActor = Context.ResolveScopedActor(ApplicationScope);
	if (!Context.IsAuthoritative() || !ScopedAbilitySystem ||
		!IsValid(ScopedActor) || !CueTag.IsValid())
	{
		return false;
	}

	UObject* SourceObject = Context.Ability
		? Context.Ability->GetCurrentSourceObject()
		: nullptr;
	AActor* EffectCauser = Cast<AActor>(SourceObject);
	if (!EffectCauser)
	{
		if (const UActorComponent* SourceComponent =
			Cast<UActorComponent>(SourceObject))
		{
			EffectCauser = SourceComponent->GetOwner();
		}
	}
	if (!EffectCauser)
	{
		EffectCauser = Context.SourceActor;
	}

	FGameplayEffectContextHandle EffectContext =
		Context.SourceAbilitySystem->MakeEffectContext();
	EffectContext.AddInstigator(Context.SourceActor, EffectCauser);
	if (SourceObject)
	{
		EffectContext.AddSourceObject(SourceObject);
	}
	if (Context.bHasHitResult)
	{
		EffectContext.AddHitResult(Context.HitResult, true);
	}

	FGameplayCueParameters CueParameters(EffectContext);
	CueParameters.Instigator = Context.SourceActor;
	CueParameters.EffectCauser = EffectCauser;
	CueParameters.SourceObject = SourceObject;
	CueParameters.AbilityLevel = Context.Ability
		? Context.Ability->GetAbilityLevel()
		: 1;
	CueParameters.bReplicateLocationWhenUsingMinimalRepProxy = true;

	if (Context.bHasHitResult)
	{
		CueParameters.Location = Context.HitResult.ImpactPoint;
		CueParameters.Normal = Context.HitResult.ImpactNormal;
		CueParameters.PhysicalMaterial = Context.HitResult.PhysMaterial.Get();
	}
	else
	{
		CueParameters.Location = ScopedActor->GetActorLocation();
	}

	ScopedAbilitySystem->ExecuteGameplayCue(CueTag, CueParameters);
	return true;
}
