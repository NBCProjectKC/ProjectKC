#include "ProjectKC/AbilitySystem/Ability/KCGameplayAbility.h"

#include "AbilitySystemComponent.h"
#include "GameplayAbilitySpec.h"
#include "GameplayEffect.h"
#include "ProjectKC/AbilitySystem/Component/KCAbilitySystemComponent.h"
#include "ProjectKC/AbilitySystem/Config/KCActionConfig.h"
#include "ProjectKC/AbilitySystem/Definition/KCAbilityDefinition.h"
#include "ProjectKC/AbilitySystem/Fragment/KCActionExecutionContext.h"
#include "ProjectKC/AbilitySystem/Fragment/KCActionFragment.h"
#include "ProjectKC/AbilitySystem/Struct/KCGameplayEffectRecipeStruct.h"

UKCGameplayAbility::UKCGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

bool UKCGameplayAbility::ValidateDefinitionContract(
	const UKCAbilityDefinition& Definition,
	FString& OutError) const
{
	OutError.Reset();
	if (Definition.ActionMontage.HasMontage() && !bSupportsActionMontage)
	{
		OutError = FString::Printf(
			TEXT("ActionClass '%s'가 Action Montage를 지원하지 않습니다."),
			*GetClass()->GetName());
		return false;
	}

	if (Definition.ActionConfig)
	{
		if (!SupportedActionConfigClass ||
			!Definition.ActionConfig->IsA(SupportedActionConfigClass))
		{
			OutError = FString::Printf(
				TEXT("ActionClass '%s'가 ActionConfig '%s'를 지원하지 않습니다."),
				*GetClass()->GetName(),
				*GetNameSafe(Definition.ActionConfig));
			return false;
		}
	}
	else if (bActionConfigRequired)
	{
		OutError = FString::Printf(
			TEXT("ActionClass '%s'에 필요한 ActionConfig가 없습니다."),
			*GetClass()->GetName());
		return false;
	}

	for (const FKCActionHookStruct& Hook : Definition.ActionHooks)
	{
		if (!SupportedActionHooks.HasTagExact(Hook.HookTag))
		{
			OutError = FString::Printf(
				TEXT("ActionClass '%s'가 Action Hook '%s'를 지원하지 않습니다."),
				*GetClass()->GetName(),
				*Hook.HookTag.ToString());
			return false;
		}
	}

	for (const FGameplayTag& RequiredHook : RequiredActionHooks)
	{
		if (!Definition.FindActionHook(RequiredHook))
		{
			OutError = FString::Printf(
				TEXT("ActionClass '%s'에 필요한 Action Hook '%s'가 없습니다."),
				*GetClass()->GetName(),
				*RequiredHook.ToString());
			return false;
		}
	}

	return true;
}

bool UKCGameplayAbility::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	const UKCAbilityDefinition* Definition = nullptr;
	if (!ResolveDefinitionForSpec(Handle, ActorInfo, Definition))
	{
		return false;
	}

	return Super::CanActivateAbility(
		Handle,
		ActorInfo,
		SourceTags,
		TargetTags,
		OptionalRelevantTags);
}

void UKCGameplayAbility::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	const UKCAbilityDefinition* Definition = nullptr;
	if (!ResolveDefinitionForSpec(Handle, ActorInfo, Definition))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ActiveDefinition = const_cast<UKCAbilityDefinition*>(Definition);
	RuntimeMagnitudes.Reset();
	TrackedActiveEffects.Reset();

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UKCGameplayAbility::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid() &&
		ActorInfo->AbilitySystemComponent->IsOwnerActorAuthoritative())
	{
		for (const FKCTrackedActiveEffect& TrackedEffect : TrackedActiveEffects)
		{
			if (TrackedEffect.TargetAbilitySystem.IsValid() &&
				TrackedEffect.EffectHandle.IsValid())
			{
				TrackedEffect.TargetAbilitySystem->RemoveActiveGameplayEffect(
					TrackedEffect.EffectHandle);
			}
		}
	}

	TrackedActiveEffects.Reset();
	RuntimeMagnitudes.Reset();
	ActiveDefinition = nullptr;

	Super::EndAbility(
		Handle,
		ActorInfo,
		ActivationInfo,
		bReplicateEndAbility,
		bWasCancelled);
}

void UKCGameplayAbility::AddSupportedActionHook(FGameplayTag HookTag)
{
	if (HookTag.IsValid())
	{
		SupportedActionHooks.AddTag(HookTag);
	}
}

void UKCGameplayAbility::AddRequiredActionHook(FGameplayTag HookTag)
{
	if (HookTag.IsValid())
	{
		SupportedActionHooks.AddTag(HookTag);
		RequiredActionHooks.AddTag(HookTag);
	}
}

void UKCGameplayAbility::SetSupportedActionConfigClass(
	TSubclassOf<UKCActionConfig> ConfigClass,
	bool bRequired)
{
	SupportedActionConfigClass = ConfigClass;
	bActionConfigRequired = bRequired;
}

void UKCGameplayAbility::SetSupportsActionMontage(bool bSupported)
{
	bSupportsActionMontage = bSupported;
}

bool UKCGameplayAbility::SetRuntimeMagnitude(
	FGameplayTag DataTag,
	float Magnitude)
{
	if (!DataTag.IsValid() || !FMath::IsFinite(Magnitude) ||
		!ActiveDefinition ||
		!ActiveDefinition->DeclaresSetByCallerTag(DataTag))
	{
		return false;
	}

	RuntimeMagnitudes.Add(DataTag, Magnitude);
	return true;
}

bool UKCGameplayAbility::ExecuteActionHook(
	FGameplayTag HookTag,
	UAbilitySystemComponent* TargetAbilitySystem,
	AActor* TargetActor,
	const FHitResult* HitResult)
{
	UAbilitySystemComponent* SourceAbilitySystem =
		GetAbilitySystemComponentFromActorInfo();
	const FKCActionHookStruct* Hook = ActiveDefinition
		? ActiveDefinition->FindActionHook(HookTag)
		: nullptr;
	if (!SourceAbilitySystem || !Hook)
	{
		return false;
	}

	FKCActionExecutionContext Context;
	Context.Ability = this;
	Context.SourceAbilitySystem = SourceAbilitySystem;
	Context.TargetAbilitySystem = TargetAbilitySystem;
	Context.SourceActor = GetAvatarActorFromActorInfo();
	Context.TargetActor = TargetActor;
	if (HitResult)
	{
		Context.HitResult = *HitResult;
		Context.bHasHitResult = true;
	}

	TArray<const UKCActionFragment*, TInlineAllocator<8>> ExecutableFragments;
	for (const UKCActionFragment* Fragment : Hook->Fragments)
	{
		if (!IsValid(Fragment))
		{
			return false;
		}

		FString ExecutionError;
		if (Fragment->CanExecute(Context, ExecutionError))
		{
			ExecutableFragments.Add(Fragment);
		}
		else if (Fragment->bRequired)
		{
			return false;
		}
	}

	for (const UKCActionFragment* Fragment : ExecutableFragments)
	{
		if (!Fragment->Execute(Context) && Fragment->bRequired)
		{
			return false;
		}
	}

	return true;
}

const UKCAbilityDefinition* UKCGameplayAbility::GetActiveDefinition() const
{
	return ActiveDefinition;
}

const UKCActionConfig* UKCGameplayAbility::GetActiveActionConfig() const
{
	return ActiveDefinition ? ActiveDefinition->ActionConfig : nullptr;
}

bool UKCGameplayAbility::ApplyGameplayEffectRecipe(
	const FKCGameplayEffectRecipeStruct& Recipe,
	const FKCActionExecutionContext& Context,
	bool bTrackUntilAbilityEnds)
{
	UAbilitySystemComponent* SourceAbilitySystem =
		GetAbilitySystemComponentFromActorInfo();
	if (Context.Ability != this ||
		Context.SourceAbilitySystem != SourceAbilitySystem ||
		!Context.TargetAbilitySystem ||
		!Context.IsAuthoritative())
	{
		return false;
	}

	const FGameplayEffectSpecHandle SpecHandle = MakeEffectSpec(Recipe);
	if (!SpecHandle.IsValid())
	{
		return false;
	}

	FActiveGameplayEffectHandle ActiveHandle;
	if (Context.TargetAbilitySystem == SourceAbilitySystem)
	{
		ActiveHandle = SourceAbilitySystem->ApplyGameplayEffectSpecToSelf(
			*SpecHandle.Data.Get());
	}
	else
	{
		ActiveHandle = SourceAbilitySystem->ApplyGameplayEffectSpecToTarget(
			*SpecHandle.Data.Get(),
			Context.TargetAbilitySystem);
	}

	if (bTrackUntilAbilityEnds && ActiveHandle.IsValid())
	{
		FKCTrackedActiveEffect& TrackedEffect =
			TrackedActiveEffects.AddDefaulted_GetRef();
		TrackedEffect.TargetAbilitySystem = Context.TargetAbilitySystem;
		TrackedEffect.EffectHandle = ActiveHandle;
	}

	return true;
}

bool UKCGameplayAbility::ResolveDefinitionForSpec(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const UKCAbilityDefinition*& OutDefinition,
	FString* OutError) const
{
	OutDefinition = nullptr;
	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid())
	{
		if (OutError)
		{
			*OutError = TEXT("AbilityActorInfo 또는 AbilitySystemComponent가 없습니다.");
		}
		return false;
	}

	const FGameplayAbilitySpec* Spec =
		ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle(Handle);
	if (!Spec || !Spec->SourceObject.IsValid())
	{
		if (OutError)
		{
			*OutError = TEXT("정확한 AbilitySpec 또는 SourceObject를 찾지 못했습니다.");
		}
		return false;
	}

	if (!UKCAbilitySystemComponent::ResolveDefinitionFromSource(
		Spec->SourceObject.Get(),
		OutDefinition,
		OutError))
	{
		return false;
	}

	if (OutDefinition->ActionClass != GetClass())
	{
		if (OutError)
		{
			*OutError = TEXT("Definition의 ActionClass와 실행 중인 Ability 클래스가 다릅니다.");
		}
		return false;
	}

	return true;
}

FGameplayEffectSpecHandle UKCGameplayAbility::MakeEffectSpec(
	const FKCGameplayEffectRecipeStruct& Recipe) const
{
	UAbilitySystemComponent* AbilitySystem =
		GetAbilitySystemComponentFromActorInfo();
	if (!AbilitySystem || !Recipe.EffectClass)
	{
		return FGameplayEffectSpecHandle();
	}

	FGameplayEffectContextHandle Context = AbilitySystem->MakeEffectContext();
	Context.AddSourceObject(GetCurrentSourceObject());

	FGameplayEffectSpecHandle SpecHandle = AbilitySystem->MakeOutgoingSpec(
		Recipe.EffectClass,
		Recipe.EffectLevel,
		Context);
	if (!SpecHandle.IsValid())
	{
		return FGameplayEffectSpecHandle();
	}

	SpecHandle.Data->DynamicGrantedTags.AppendTags(Recipe.DynamicGrantedTags);
	for (const FKCSetByCallerValueStruct& Value : Recipe.SetByCallers)
	{
		const float* RuntimeMagnitude = RuntimeMagnitudes.Find(Value.DataTag);
		SpecHandle.Data->SetSetByCallerMagnitude(
			Value.DataTag,
			RuntimeMagnitude ? *RuntimeMagnitude : Value.Magnitude);
	}

	return SpecHandle;
}
