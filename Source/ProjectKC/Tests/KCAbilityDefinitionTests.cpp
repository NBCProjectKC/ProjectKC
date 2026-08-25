#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Animation/AnimMontage.h"
#include "GameplayEffect.h"
#include "ProjectKC/AbilitySystem/Ability/KCGA_Action.h"
#include "ProjectKC/AbilitySystem/Ability/KCGA_ChannelAction.h"
#include "ProjectKC/AbilitySystem/Component/KCAbilitySourceComponent.h"
#include "ProjectKC/AbilitySystem/Component/KCAbilitySystemComponent.h"
#include "ProjectKC/AbilitySystem/Definition/KCChannelActionDefinition.h"
#include "ProjectKC/AbilitySystem/Definition/KCSingleActionDefinition.h"
#include "ProjectKC/AbilitySystem/Fragment/KCApplyGameplayEffectFragment.h"
#include "ProjectKC/AbilitySystem/Fragment/KCKnockbackFragment.h"
#include "ProjectKC/AbilitySystem/Tag/KCAbilityGameplayTags.h"
#include "ProjectKC/AbilitySystem/Tag/KCGameplayTagBlueprintLibrary.h"
#include "ProjectKC/AbilitySystem/Targeting/KCEventTargeting.h"
#include "ProjectKC/AbilitySystem/Targeting/KCItemSocketTrailTargeting.h"
#include "ProjectKC/AbilitySystem/Targeting/KCOverlapTargeting.h"
#include "ProjectKC/AbilitySystem/Targeting/KCSelfTargeting.h"
#include "ProjectKC/AbilitySystem/Targeting/KCSweepTargeting.h"

#include <limits>

namespace KCAbilityDefinitionTests
{
	UKCApplyGameplayEffectFragment* AddEffectFragment(
		UKCAbilityDefinition* Definition,
		FKCActionHookStruct& Hook,
		EKCActionScope Scope = EKCActionScope::Target)
	{
		UKCApplyGameplayEffectFragment* Fragment =
			NewObject<UKCApplyGameplayEffectFragment>(Definition);
		Fragment->EffectRecipe.EffectClass = UGameplayEffect::StaticClass();
		Fragment->ApplicationScope = Scope;
		Hook.Fragments.Add(Fragment);
		return Fragment;
	}

	/** 대상 수집 방식만 다르고 나머지는 같은 최소 Definition이다. */
	UKCSingleActionDefinition* MakeDefinition(
		TSubclassOf<UKCActionTargeting> TargetingClass)
	{
		UKCSingleActionDefinition* Definition =
			NewObject<UKCSingleActionDefinition>();
		Definition->ActionTargeting =
			NewObject<UKCActionTargeting>(Definition, TargetingClass);

		FKCActionHookStruct Hook;
		Hook.HookTag = TAG_KC_ActionHook_OnExecute;
		AddEffectFragment(Definition, Hook);
		Definition->ActionHooks.Add(MoveTemp(Hook));
		return Definition;
	}

	UKCChannelActionDefinition* MakeChannelDefinition(
		TSubclassOf<UKCActionTargeting> TargetingClass)
	{
		UKCChannelActionDefinition* Definition =
			NewObject<UKCChannelActionDefinition>();
		Definition->ActionTargeting =
			NewObject<UKCActionTargeting>(Definition, TargetingClass);

		FKCActionHookStruct Hook;
		Hook.HookTag = TAG_KC_ActionHook_OnExecute;
		AddEffectFragment(Definition, Hook);
		Definition->ActionHooks.Add(MoveTemp(Hook));
		return Definition;
	}

	UAnimMontage* AddMontage(UKCAbilityDefinition* Definition)
	{
		Definition->ActionMontage.Montage = NewObject<UAnimMontage>(Definition);
		return Definition->ActionMontage.Montage;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FKCAbilityDefinitionValidationTest,
	"ProjectKC.GAS.Definition.Validation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKCAbilityDefinitionValidationTest::RunTest(const FString& Parameters)
{
	using namespace KCAbilityDefinitionTests;
	FString Error;

	TestTrue(
		TEXT("등록된 네이티브 Gameplay Tag를 이름으로 안전하게 조회한다."),
		UKCGameplayTagBlueprintLibrary::RequestRegisteredGameplayTag(
			TEXT("ActionHook.OnExecute")) == TAG_KC_ActionHook_OnExecute);

	// ── 대상 수집 축 ───────────────────────────────────────
	TestTrue(
		TEXT("Self 대상 Definition은 유효하다."),
		MakeDefinition(UKCSelfTargeting::StaticClass())
			->ValidateWithActionContract(Error));
	TestTrue(
		TEXT("Event 대상 Definition은 유효하다."),
		MakeDefinition(UKCEventTargeting::StaticClass())
			->ValidateWithActionContract(Error));
	TestTrue(
		TEXT("Sweep 대상 Definition은 유효하다."),
		MakeDefinition(UKCSweepTargeting::StaticClass())
			->ValidateWithActionContract(Error));
	UKCAbilityDefinition* SocketTrailWithoutMontage =
		MakeDefinition(UKCItemSocketTrailTargeting::StaticClass());
	TestFalse(
		TEXT("Socket Trail 방식은 NotifyState를 담을 Montage 없이 사용할 수 없다."),
		SocketTrailWithoutMontage->ValidateWithActionContract(Error));
	AddMontage(SocketTrailWithoutMontage);
	TestTrue(
		TEXT("Montage를 가진 Socket Trail Definition은 유효하다."),
		SocketTrailWithoutMontage->ValidateWithActionContract(Error));
	TestTrue(
		TEXT("Socket Trail 방식은 TraceWindow Targeting 계약을 구현한다."),
		SocketTrailWithoutMontage->ActionTargeting->IsA<
			UKCTraceWindowTargeting>());
	TestTrue(
		TEXT("Overlap 대상 Definition은 유효하다."),
		MakeDefinition(UKCOverlapTargeting::StaticClass())
			->ValidateWithActionContract(Error));
	TestFalse(
		TEXT("Overlap 방식은 활성화 Target을 요구하지 않는다."),
		GetDefault<UKCOverlapTargeting>()->RequiresActivationTarget());

	UKCAbilityDefinition* NoTargeting =
		MakeDefinition(UKCSelfTargeting::StaticClass());
	NoTargeting->ActionTargeting = nullptr;
	TestFalse(
		TEXT("대상 수집 방식이 없는 Definition은 거부한다."),
		NoTargeting->Validate(Error));

	// ── 활성화 계약이 타게팅 방식에서 나온다 ───────────────
	TestTrue(
		TEXT("Event 방식만 활성화 Target을 요구한다."),
		GetDefault<UKCEventTargeting>()->RequiresActivationTarget());
	TestFalse(
		TEXT("Self 방식은 활성화 Target을 요구하지 않는다."),
		GetDefault<UKCSelfTargeting>()->RequiresActivationTarget());
	TestFalse(
		TEXT("Sweep 방식은 활성화 Target을 요구하지 않는다."),
		GetDefault<UKCSweepTargeting>()->RequiresActivationTarget());
	TestTrue(
		TEXT("Sweep 방식은 Instant Targeting 계약을 구현한다."),
		GetDefault<UKCSweepTargeting>()->IsA<UKCInstantActionTargeting>());

	// ── 수명주기 Definition과 GA는 코드로 고정된다 ─────────
	UKCSingleActionDefinition* ImmediateSingle =
		MakeDefinition(UKCEventTargeting::StaticClass());
	TestNull(
		TEXT("몽타주가 없는 Single Action은 별도 Timing 오브젝트를 만들지 않는다."),
		ImmediateSingle->ActionMontage.Montage.Get());
	TestTrue(
		TEXT("Single Action은 KCGA_Action에 고정된다."),
		ImmediateSingle->GetAbilityClass() == UKCGA_Action::StaticClass());
	TestTrue(
		TEXT("몽타주 없는 Single Action은 즉시 실행 계약으로 유효하다."),
		ImmediateSingle->ValidateWithActionContract(Error));

	UKCAbilityDefinition* MontageDefinition =
		MakeDefinition(UKCSweepTargeting::StaticClass());
	AddMontage(MontageDefinition);
	TestTrue(
		TEXT("몽타주를 가진 Single Action은 유효하다."),
		MontageDefinition->ValidateWithActionContract(Error));

	UKCChannelActionDefinition* ChannelDefinition =
		MakeChannelDefinition(UKCOverlapTargeting::StaticClass());
	TestFalse(
		TEXT("Channel Action은 반복 실행 시점을 제공할 Montage가 필수다."),
		ChannelDefinition->Validate(Error));
	AddMontage(ChannelDefinition);
	TestTrue(
		TEXT("Montage를 가진 Channel Action은 유효하다."),
		ChannelDefinition->ValidateWithActionContract(Error));
	TestTrue(
		TEXT("Channel Action은 KCGA_ChannelAction에 고정된다."),
		ChannelDefinition->GetAbilityClass() ==
			UKCGA_ChannelAction::StaticClass());

	UKCAbilityDefinition* ZeroPlayRate =
		MakeDefinition(UKCSweepTargeting::StaticClass());
	AddMontage(ZeroPlayRate);
	ZeroPlayRate->ActionMontage.PlayRate = 0.0f;
	TestFalse(
		TEXT("PlayRate가 0 이하면 거부한다."),
		ZeroPlayRate->Validate(Error));

	UKCAbilityDefinition* NaNPlayRate =
		MakeDefinition(UKCSweepTargeting::StaticClass());
	AddMontage(NaNPlayRate);
	NaNPlayRate->ActionMontage.PlayRate =
		std::numeric_limits<float>::quiet_NaN();
	TestFalse(
		TEXT("유한하지 않은 PlayRate는 거부한다."),
		NaNPlayRate->Validate(Error));

	UKCAbilityDefinition* MissingSection =
		MakeDefinition(UKCSweepTargeting::StaticClass());
	AddMontage(MissingSection);
	MissingSection->ActionMontage.StartSection =
		TEXT("KCTestMissingSection");
	TestFalse(
		TEXT("Montage에 없는 StartSection은 거부한다."),
		MissingSection->Validate(Error));

	// ── 적용 범위 축: 흡혈 무기가 데이터만으로 조립된다 ────
	UKCAbilityDefinition* Lifesteal =
		MakeDefinition(UKCSweepTargeting::StaticClass());
	UKCApplyGameplayEffectFragment* DamageFragment =
		CastChecked<UKCApplyGameplayEffectFragment>(
			Lifesteal->ActionHooks[0].Fragments[0]);
	FKCSetByCallerValueStruct DamageValue;
	DamageValue.DataTag = TAG_KC_Data_Damage_Flat;
	DamageValue.Magnitude = 10.0f;
	DamageFragment->EffectRecipe.SetByCallers.Add(DamageValue);

	UKCApplyGameplayEffectFragment* HealFragment = AddEffectFragment(
		Lifesteal,
		Lifesteal->ActionHooks[0],
		EKCActionScope::Source);
	FKCSetByCallerValueStruct HealValue;
	HealValue.DataTag = TAG_KC_Data_Damage_Flat;
	HealValue.Magnitude = 5.0f;
	HealFragment->EffectRecipe.SetByCallers.Add(HealValue);

	TestTrue(
		TEXT("같은 태그로 대상 10 / 소스 5를 주는 조합은 유효하다."),
		Lifesteal->ValidateWithActionContract(Error));
	TestTrue(
		TEXT("한 Hook에 서로 다른 Scope의 Fragment를 함께 조립한다."),
		Lifesteal->ActionHooks[0].Fragments.Num() == 2 &&
		DamageFragment->ApplicationScope == EKCActionScope::Target &&
		HealFragment->ApplicationScope == EKCActionScope::Source);

	// ── 시점을 여러 개 조립한다 ────────────────────────────
	UKCAbilityDefinition* MultiHook =
		MakeDefinition(UKCSweepTargeting::StaticClass());
	FKCActionHookStruct StartHook;
	StartHook.HookTag = TAG_KC_ActionHook_OnStart;
	AddEffectFragment(MultiHook, StartHook, EKCActionScope::Source);
	MultiHook->ActionHooks.Add(MoveTemp(StartHook));
	TestTrue(
		TEXT("OnStart와 OnExecute를 한 Definition에 함께 조립한다."),
		MultiHook->ValidateWithActionContract(Error));

	UKCAbilityDefinition* DuplicateHook =
		MakeDefinition(UKCSelfTargeting::StaticClass());
	// 같은 배열의 원소를 그대로 Add하면 UE가 어서션에 걸리므로 복사본을 만든다.
	const FKCActionHookStruct HookCopy = DuplicateHook->ActionHooks[0];
	DuplicateHook->ActionHooks.Add(HookCopy);
	TestFalse(
		TEXT("같은 시점 Hook이 중복되면 거부한다."),
		DuplicateHook->Validate(Error));

	// ── 기존 계약 회귀 ─────────────────────────────────────
	UKCAbilityDefinition* EmptyDefinition =
		NewObject<UKCSingleActionDefinition>();
	TestFalse(
		TEXT("Targeting이 없는 구체 Definition은 거부한다."),
		EmptyDefinition->Validate(Error));

	UKCAbilityDefinition* KnockbackOnly =
		MakeDefinition(UKCEventTargeting::StaticClass());
	KnockbackOnly->ActionHooks[0].Fragments.Empty();
	KnockbackOnly->ActionHooks[0].Fragments.Add(
		NewObject<UKCKnockbackFragment>(KnockbackOnly));
	TestTrue(
		TEXT("GE 없이 Knockback Fragment만 조립한 Definition도 유효하다."),
		KnockbackOnly->ValidateWithActionContract(Error));

	UKCAbilityDefinition* DuplicateInRecipe =
		MakeDefinition(UKCSelfTargeting::StaticClass());
	UKCApplyGameplayEffectFragment* Fragment =
		CastChecked<UKCApplyGameplayEffectFragment>(
			DuplicateInRecipe->ActionHooks[0].Fragments[0]);
	FKCSetByCallerValueStruct Duplicate;
	Duplicate.DataTag = TAG_KC_Data_Damage_Flat;
	Fragment->EffectRecipe.SetByCallers = {Duplicate, Duplicate};
	TestFalse(
		TEXT("같은 GE Recipe 안의 중복 SetByCaller 태그는 여전히 거부한다."),
		DuplicateInRecipe->Validate(Error));

	UKCAbilitySourceComponent* Source = NewObject<UKCAbilitySourceComponent>();
	UKCAbilityDefinition* SourceDefinition =
		MakeDefinition(UKCSelfTargeting::StaticClass());
	Source->ConfigureAbilityDefinition(SourceDefinition);
	TestTrue(
		TEXT("런타임에 주입한 Definition을 공용 Source가 보유한 것으로 판정한다."),
		Source->HasAbilityDefinition());
	const UKCAbilityDefinition* Resolved = nullptr;
	TestTrue(
		TEXT("공용 Source 컴포넌트에서 Definition을 복원할 수 있다."),
		UKCAbilitySystemComponent::ResolveDefinitionFromSource(
			Source, Resolved, &Error));
	TestTrue(
		TEXT("복원한 Definition은 Source가 참조한 정확한 자산이다."),
		Resolved == SourceDefinition);

	UKCAbilitySourceComponent* AuthoredSource =
		NewObject<UKCAbilitySourceComponent>();
	UKCAbilityDefinition* AuthoredDefinition =
		MakeDefinition(UKCSweepTargeting::StaticClass());
	AuthoredSource->ActionDefinition = AuthoredDefinition;
	TestTrue(
		TEXT("컴포넌트에 직접 저작한 ActionDefinition도 Source가 보유한 것으로 판정한다."),
		AuthoredSource->HasAbilityDefinition());

	Resolved = nullptr;
	TestTrue(
		TEXT("공용 Source에서 직접 저작한 ActionDefinition을 복원할 수 있다."),
		AuthoredSource->ResolveAbilityDefinition(Resolved));
	TestTrue(
		TEXT("복원한 Definition은 Source에 직접 저작한 정확한 오브젝트다."),
		Resolved == AuthoredDefinition);

	UKCAbilityDefinition* InstanceDefinition =
		MakeDefinition(UKCEventTargeting::StaticClass());
	AuthoredSource->InstanceActionDefinition = InstanceDefinition;
	Resolved = nullptr;
	TestTrue(
		TEXT("배치 인스턴스 Definition Override를 복원할 수 있다."),
		AuthoredSource->ResolveAbilityDefinition(Resolved));
	TestTrue(
		TEXT("인스턴스 Override는 컴포넌트 기본 Definition보다 우선한다."),
		Resolved == InstanceDefinition);

	UKCAbilityDefinition* RuntimeDefinition =
		MakeDefinition(UKCSelfTargeting::StaticClass());
	TestTrue(
		TEXT("Grant 전에는 런타임 Definition을 주입할 수 있다."),
		AuthoredSource->ConfigureAbilityDefinition(RuntimeDefinition));
	Resolved = nullptr;
	TestTrue(
		TEXT("런타임 주입 Definition을 복원할 수 있다."),
		AuthoredSource->ResolveAbilityDefinition(Resolved));
	TestTrue(
		TEXT("런타임 주입값은 인스턴스 Definition Override보다 우선한다."),
		Resolved == RuntimeDefinition);

	return true;
}

#endif
