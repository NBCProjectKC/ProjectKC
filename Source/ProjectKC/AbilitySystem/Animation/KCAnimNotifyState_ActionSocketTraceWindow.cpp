#include "ProjectKC/AbilitySystem/Animation/KCAnimNotifyState_ActionSocketTraceWindow.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "ProjectKC/AbilitySystem/Ability/KCGA_ActionRuntimeBase.h"

namespace KCActionSocketTraceNotify
{
	UKCGA_ActionRuntimeBase* ResolveAbility(USkeletalMeshComponent* MeshComp)
	{
		AActor* OwnerActor = MeshComp ? MeshComp->GetOwner() : nullptr;
		if (!IsValid(OwnerActor) || !OwnerActor->HasAuthority())
		{
			return nullptr;
		}

		UAbilitySystemComponent* AbilitySystem =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerActor);
		return AbilitySystem
			? Cast<UKCGA_ActionRuntimeBase>(AbilitySystem->GetAnimatingAbility())
			: nullptr;
	}
}

UKCAnimNotifyState_ActionSocketTraceWindow::
UKCAnimNotifyState_ActionSocketTraceWindow()
{
	NotifyStateBehaviorFlags =
		static_cast<uint8>(EAnimNotifyStateBehaviorFlags::NoMergeOnConcurrentPlay);

#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(70, 180, 235);
	bShouldFireInEditor = false;
#endif
}

FString UKCAnimNotifyState_ActionSocketTraceWindow::
GetNotifyName_Implementation() const
{
	return TEXT("KC Action Socket Trace Window");
}

void UKCAnimNotifyState_ActionSocketTraceWindow::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	if (UKCGA_ActionRuntimeBase* Ability =
		KCActionSocketTraceNotify::ResolveAbility(MeshComp))
	{
		Ability->NotifySocketTraceWindowBegin();
	}
}

void UKCAnimNotifyState_ActionSocketTraceWindow::NotifyTick(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float FrameDeltaTime,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);
	if (UKCGA_ActionRuntimeBase* Ability =
		KCActionSocketTraceNotify::ResolveAbility(MeshComp))
	{
		Ability->NotifySocketTraceWindowTick();
	}
}

void UKCAnimNotifyState_ActionSocketTraceWindow::NotifyEnd(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	if (UKCGA_ActionRuntimeBase* Ability =
		KCActionSocketTraceNotify::ResolveAbility(MeshComp))
	{
		Ability->NotifySocketTraceWindowEnd();
	}
}

#if WITH_EDITOR
bool UKCAnimNotifyState_ActionSocketTraceWindow::CanBePlaced(
	UAnimSequenceBase* Animation) const
{
	return Animation && Animation->IsA<UAnimMontage>();
}
#endif
