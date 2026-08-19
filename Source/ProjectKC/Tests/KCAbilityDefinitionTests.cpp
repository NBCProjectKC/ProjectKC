#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Animation/AnimMontage.h"
#include "GameplayEffect.h"
#include "ProjectKC/AbilitySystem/Ability/KCGameplayAbility.h"
#include "ProjectKC/AbilitySystem/Ability/KCGAMeleeSwing.h"
#include "ProjectKC/AbilitySystem/Ability/KCGAInstantSelfAction.h"
#include "ProjectKC/AbilitySystem/Ability/KCGAInstantTargetAction.h"
#include "ProjectKC/AbilitySystem/Component/KCAbilitySourceComponent.h"
#include "ProjectKC/AbilitySystem/Component/KCAbilitySystemComponent.h"
#include "ProjectKC/AbilitySystem/Config/KCMeleeActionConfig.h"
#include "ProjectKC/AbilitySystem/Definition/KCAbilityDefinition.h"
#include "ProjectKC/AbilitySystem/Fragment/KCApplyGameplayEffectFragment.h"
#include "ProjectKC/AbilitySystem/Fragment/KCKnockbackFragment.h"
#include "ProjectKC/AbilitySystem/Tag/KCGameplayTagBlueprintLibrary.h"
#include "ProjectKC/AbilitySystem/Tag/KCGameplayTags.h"

#include <limits>

namespace KCAbilityDefinitionTests
{
	UKCApplyGameplayEffectFragment* AddEffectFragment(
		UKCAbilityDefinition* Definition,
		FKCActionHookStruct& Hook)
	{
		UKCApplyGameplayEffectFragment* Fragment =
			NewObject<UKCApplyGameplayEffectFragment>(Definition);
		Fragment->EffectRecipe.EffectClass = UGameplayEffect::StaticClass();
		Hook.Fragments.Add(Fragment);
		return Fragment;
	}

	UKCAbilityDefinition* MakeValidSelfDefinition()
	{
		UKCAbilityDefinition* Definition = NewObject<UKCAbilityDefinition>();
		Definition->ActionClass = UKCGAInstantSelfAction::StaticClass();

		FKCActionHookStruct Hook;
		Hook.HookTag = TAG_KC_ActionHook_Self_OnActivate;
		AddEffectFragment(Definition, Hook);
		Definition->ActionHooks.Add(MoveTemp(Hook));
		return Definition;
	}

	UKCAbilityDefinition* MakeKnockbackOnlyDefinition()
	{
		UKCAbilityDefinition* Definition = NewObject<UKCAbilityDefinition>();
		Definition->ActionClass = UKCGAInstantTargetAction::StaticClass();

		FKCActionHookStruct Hook;
		Hook.HookTag = TAG_KC_ActionHook_Target_OnTrigger;
		Hook.Fragments.Add(NewObject<UKCKnockbackFragment>(Definition));
		Definition->ActionHooks.Add(MoveTemp(Hook));
		return Definition;
	}

	UKCAbilityDefinition* MakeValidMeleeDefinition()
	{
		UKCAbilityDefinition* Definition = NewObject<UKCAbilityDefinition>();
		Definition->ActionClass = UKCGAMeleeSwing::StaticClass();
		Definition->ActionConfig =
			NewObject<UKCMeleeActionConfig>(Definition);

		FKCActionHookStruct Hook;
		Hook.HookTag = TAG_KC_ActionHook_Target_OnHit;
		Hook.Fragments.Add(NewObject<UKCKnockbackFragment>(Definition));
		Definition->ActionHooks.Add(MoveTemp(Hook));
		return Definition;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FKCAbilityDefinitionValidationTest,
	"ProjectKC.GAS.Definition.Validation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKCAbilityDefinitionValidationTest::RunTest(const FString& Parameters)
{
	FString Error;
	TestTrue(
		TEXT("등록된 네이티브 Gameplay Tag를 이름으로 안전하게 조회한다."),
		UKCGameplayTagBlueprintLibrary::RequestRegisteredGameplayTag(
			TEXT("ActionHook.Target.OnHit")) ==
			TAG_KC_ActionHook_Target_OnHit);
	TestFalse(
		TEXT("등록되지 않은 Gameplay Tag 이름은 Invalid Tag를 반환한다."),
		UKCGameplayTagBlueprintLibrary::RequestRegisteredGameplayTag(
			TEXT("KC.Test.NotRegistered"))
			.IsValid());

	UKCAbilityDefinition* EmptyDefinition = NewObject<UKCAbilityDefinition>();
	TestFalse(
		TEXT("ActionClass가 없는 Definition은 거부한다."),
		EmptyDefinition->Validate(Error));

	UKCAbilityDefinition* ValidDefinition =
		KCAbilityDefinitionTests::MakeValidSelfDefinition();
	TestTrue(
		TEXT("지원 Hook과 GE Fragment를 갖춘 Definition은 유효하다."),
		ValidDefinition->ValidateWithActionContract(Error));

	UKCAbilityDefinition* UnsupportedDefinition =
		KCAbilityDefinitionTests::MakeValidSelfDefinition();
	UnsupportedDefinition->ActionHooks[0].HookTag =
		TAG_KC_ActionHook_Target_OnTrigger;
	TestFalse(
		TEXT("GA가 지원하지 않는 Hook은 계약 검증에서 거부한다."),
		UnsupportedDefinition->ValidateWithActionContract(Error));

	UKCAbilityDefinition* DuplicateHookDefinition =
		KCAbilityDefinitionTests::MakeValidSelfDefinition();
	const FKCActionHookStruct DuplicateHook =
		DuplicateHookDefinition->ActionHooks[0];
	DuplicateHookDefinition->ActionHooks.Add(DuplicateHook);
	TestFalse(
		TEXT("중복 Action Hook은 거부한다."),
		DuplicateHookDefinition->Validate(Error));

	UKCAbilityDefinition* DuplicateValueDefinition =
		KCAbilityDefinitionTests::MakeValidSelfDefinition();
	UKCApplyGameplayEffectFragment* EffectFragment =
		CastChecked<UKCApplyGameplayEffectFragment>(
			DuplicateValueDefinition->ActionHooks[0].Fragments[0]);
	FKCSetByCallerValueStruct DuplicateValue;
	DuplicateValue.DataTag = TAG_KC_Data_Damage_Flat;
	EffectFragment->EffectRecipe.SetByCallers =
		{DuplicateValue, DuplicateValue};
	TestFalse(
		TEXT("같은 GE Recipe 안의 중복 SetByCaller 태그는 거부한다."),
		DuplicateValueDefinition->Validate(Error));

	UKCAbilityDefinition* CrossFragmentDuplicateDefinition =
		KCAbilityDefinitionTests::MakeValidSelfDefinition();
	UKCApplyGameplayEffectFragment* FirstEffectFragment =
		CastChecked<UKCApplyGameplayEffectFragment>(
			CrossFragmentDuplicateDefinition->ActionHooks[0].Fragments[0]);
	FKCSetByCallerValueStruct SharedDamageValue;
	SharedDamageValue.DataTag = TAG_KC_Data_Damage_Flat;
	FirstEffectFragment->EffectRecipe.SetByCallers.Add(SharedDamageValue);
	UKCApplyGameplayEffectFragment* SecondEffectFragment =
		KCAbilityDefinitionTests::AddEffectFragment(
			CrossFragmentDuplicateDefinition,
			CrossFragmentDuplicateDefinition->ActionHooks[0]);
	SecondEffectFragment->EffectRecipe.SetByCallers.Add(SharedDamageValue);
	TestFalse(
		TEXT("Definition 전체에서 모호한 SetByCaller 태그 중복을 거부한다."),
		CrossFragmentDuplicateDefinition->Validate(Error));

	UKCAbilityDefinition* KnockbackOnlyDefinition =
		KCAbilityDefinitionTests::MakeKnockbackOnlyDefinition();
	TestTrue(
		TEXT("GE 없이 Knockback Fragment만 조립한 Definition도 유효하다."),
		KnockbackOnlyDefinition->ValidateWithActionContract(Error));

	UKCAbilityDefinition* InvalidKnockbackDefinition =
		KCAbilityDefinitionTests::MakeKnockbackOnlyDefinition();
	UKCKnockbackFragment* KnockbackFragment =
		CastChecked<UKCKnockbackFragment>(
			InvalidKnockbackDefinition->ActionHooks[0].Fragments[0]);
	KnockbackFragment->HorizontalSpeed = 0.0f;
	KnockbackFragment->VerticalSpeed = 0.0f;
	TestFalse(
		TEXT("실질적인 속도가 없는 Knockback Fragment는 거부한다."),
		InvalidKnockbackDefinition->Validate(Error));

	UKCAbilityDefinition* ValidMeleeDefinition =
		KCAbilityDefinitionTests::MakeValidMeleeDefinition();
	TestTrue(
		TEXT("Melee Config와 OnHit Hook을 갖춘 근접 공격 Definition은 유효하다."),
		ValidMeleeDefinition->ValidateWithActionContract(Error));

	UKCAbilityDefinition* MissingMeleeConfigDefinition =
		KCAbilityDefinitionTests::MakeValidMeleeDefinition();
	MissingMeleeConfigDefinition->ActionConfig = nullptr;
	TestFalse(
		TEXT("Melee GA는 Melee Config가 없으면 계약 검증에서 거부한다."),
		MissingMeleeConfigDefinition->ValidateWithActionContract(Error));

	UKCAbilityDefinition* InvalidMeleeConfigDefinition =
		KCAbilityDefinitionTests::MakeValidMeleeDefinition();
	UKCMeleeActionConfig* InvalidMeleeConfig =
		CastChecked<UKCMeleeActionConfig>(
			InvalidMeleeConfigDefinition->ActionConfig);
	InvalidMeleeConfig->TargetObjectTypes.Reset();
	TestFalse(
		TEXT("검색 Object Type이 없는 Melee Config는 거부한다."),
		InvalidMeleeConfigDefinition->Validate(Error));

	UKCAbilityDefinition* MissingOnHitDefinition =
		KCAbilityDefinitionTests::MakeValidMeleeDefinition();
	MissingOnHitDefinition->ActionHooks[0].HookTag =
		TAG_KC_ActionHook_Target_OnTrigger;
	TestFalse(
		TEXT("Melee GA는 Target.OnHit Hook이 없으면 계약 검증에서 거부한다."),
		MissingOnHitDefinition->ValidateWithActionContract(Error));

	TestTrue(
		TEXT("함정처럼 Montage가 없는 Definition도 계약 검증을 통과한다."),
		KCAbilityDefinitionTests::MakeKnockbackOnlyDefinition()
			->ValidateWithActionContract(Error));

	UKCAbilityDefinition* MontageDefinition =
		KCAbilityDefinitionTests::MakeValidMeleeDefinition();
	MontageDefinition->ActionMontage.Montage =
		NewObject<UAnimMontage>(MontageDefinition);
	TestTrue(
		TEXT("사용 Montage를 지정한 근접 공격 Definition은 유효하다."),
		MontageDefinition->ValidateWithActionContract(Error));

	UKCAbilityDefinition* ZeroPlayRateDefinition =
		KCAbilityDefinitionTests::MakeValidMeleeDefinition();
	ZeroPlayRateDefinition->ActionMontage.Montage =
		NewObject<UAnimMontage>(ZeroPlayRateDefinition);
	ZeroPlayRateDefinition->ActionMontage.PlayRate = 0.0f;
	TestFalse(
		TEXT("PlayRate가 0 이하인 Action Montage는 거부한다."),
		ZeroPlayRateDefinition->Validate(Error));

	UKCAbilityDefinition* NaNPlayRateDefinition =
		KCAbilityDefinitionTests::MakeValidMeleeDefinition();
	NaNPlayRateDefinition->ActionMontage.Montage =
		NewObject<UAnimMontage>(NaNPlayRateDefinition);
	NaNPlayRateDefinition->ActionMontage.PlayRate =
		std::numeric_limits<float>::quiet_NaN();
	TestFalse(
		TEXT("유한하지 않은 PlayRate를 가진 Action Montage는 거부한다."),
		NaNPlayRateDefinition->Validate(Error));

	UKCAbilityDefinition* MissingSectionDefinition =
		KCAbilityDefinitionTests::MakeValidMeleeDefinition();
	MissingSectionDefinition->ActionMontage.Montage =
		NewObject<UAnimMontage>(MissingSectionDefinition);
	MissingSectionDefinition->ActionMontage.StartSection =
		TEXT("KCTestMissingSection");
	TestFalse(
		TEXT("Montage에 없는 StartSection을 지정하면 거부한다."),
		MissingSectionDefinition->Validate(Error));

	UKCAbilityDefinition* UnsupportedMontageDefinition =
		NewObject<UKCAbilityDefinition>();
	UnsupportedMontageDefinition->ActionClass =
		UKCGameplayAbility::StaticClass();
	TestTrue(
		TEXT("Montage를 쓰지 않는 GA의 Hook 없는 Definition은 유효하다."),
		UnsupportedMontageDefinition->ValidateWithActionContract(Error));
	UnsupportedMontageDefinition->ActionMontage.Montage =
		NewObject<UAnimMontage>(UnsupportedMontageDefinition);
	TestFalse(
		TEXT("Action Montage를 지원하지 않는 GA에 Montage를 지정하면 거부한다."),
		UnsupportedMontageDefinition->ValidateWithActionContract(Error));

	UKCAbilityDefinition* WrongConfigDefinition =
		KCAbilityDefinitionTests::MakeValidSelfDefinition();
	WrongConfigDefinition->ActionConfig =
		NewObject<UKCMeleeActionConfig>(WrongConfigDefinition);
	TestFalse(
		TEXT("Self GA에 Melee 전용 Config를 조립하면 계약 검증에서 거부한다."),
		WrongConfigDefinition->ValidateWithActionContract(Error));

	UKCAbilityDefinition* SetByCallerDefinition =
		KCAbilityDefinitionTests::MakeValidSelfDefinition();
	UKCApplyGameplayEffectFragment* SetByCallerFragment =
		CastChecked<UKCApplyGameplayEffectFragment>(
			SetByCallerDefinition->ActionHooks[0].Fragments[0]);
	FKCSetByCallerValueStruct DamageValue;
	DamageValue.DataTag = TAG_KC_Data_Damage_Flat;
	DamageValue.Magnitude = 10.0f;
	SetByCallerFragment->EffectRecipe.SetByCallers.Add(DamageValue);
	TestTrue(
		TEXT("Definition은 Fragment가 선언한 SetByCaller 계약을 찾는다."),
		SetByCallerDefinition->DeclaresSetByCallerTag(
			TAG_KC_Data_Damage_Flat));

	UKCAbilitySourceComponent* Source =
		NewObject<UKCAbilitySourceComponent>();
	Source->ConfigureAbilityDefinition(ValidDefinition);

	const UKCAbilityDefinition* ResolvedDefinition = nullptr;
	TestTrue(
		TEXT("공용 Source 컴포넌트에서 Definition을 복원할 수 있다."),
		UKCAbilitySystemComponent::ResolveDefinitionFromSource(
			Source,
			ResolvedDefinition,
			&Error));
	TestTrue(
		TEXT("복원한 Definition은 Source가 참조한 정확한 자산이다."),
		ResolvedDefinition == ValidDefinition);

	ResolvedDefinition = nullptr;
	TestFalse(
		TEXT("Ability Source 계약이 없는 UObject는 거부한다."),
		UKCAbilitySystemComponent::ResolveDefinitionFromSource(
			NewObject<UKCAbilitySystemComponent>(),
			ResolvedDefinition,
			&Error));

	return true;
}

#endif
