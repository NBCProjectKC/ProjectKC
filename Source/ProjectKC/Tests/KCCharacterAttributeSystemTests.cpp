#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "ProjectKC/AbilitySystem/Attribute/KCCharacterAttributeSet.h"
#include "ProjectKC/AbilitySystem/Effect/KCGE_Damage.h"
#include "ProjectKC/AbilitySystem/Fragment/KCApplyGameplayEffectFragment.h"
#include "ProjectKC/AbilitySystem/Tag/KCAbilityGameplayTags.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FKCCharacterAttributeSystemContractTest,
	"ProjectKC.GAS.Attributes.CharacterContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKCCharacterAttributeSystemContractTest::RunTest(const FString& Parameters)
{
	const UKCCharacterAttributeSet* CharacterAttributes =
		GetDefault<UKCCharacterAttributeSet>();
	TestEqual(
		TEXT("기본 MaxHealth는 100이다."),
		CharacterAttributes->GetMaxHealth(),
		100.0f);
	TestEqual(
		TEXT("기본 Health는 가득 찬다."),
		CharacterAttributes->GetHealth(),
		100.0f);
	TestEqual(
		TEXT("기본 MaxStamina는 100이다."),
		CharacterAttributes->GetMaxStamina(),
		100.0f);
	TestEqual(
		TEXT("기본 Stamina는 가득 찬다."),
		CharacterAttributes->GetStamina(),
		100.0f);
	TestEqual(
		TEXT("기본 MoveSpeed는 600이다."),
		CharacterAttributes->GetMoveSpeed(),
		600.0f);

	const UKCGE_Damage* DamageEffect =
		GetDefault<UKCGE_Damage>();
	TestTrue(
		TEXT("Damage GE는 Instant다."),
		DamageEffect->DurationPolicy == EGameplayEffectDurationType::Instant);
	TestEqual(
		TEXT("Damage GE는 실행 계산을 사용하지 않는다."),
		DamageEffect->Executions.Num(),
		0);
	TestEqual(
		TEXT("Damage GE는 Modifier 하나만 가진다."),
		DamageEffect->Modifiers.Num(),
		1);
	if (DamageEffect->Modifiers.Num() == 1)
	{
		const FGameplayModifierInfo& Modifier = DamageEffect->Modifiers[0];
		TestTrue(
			TEXT("Damage GE는 Health를 직접 수정한다."),
			Modifier.Attribute ==
				UKCCharacterAttributeSet::GetHealthAttribute());
		TestTrue(
			TEXT("Damage GE는 Additive 연산을 사용한다."),
			Modifier.ModifierOp == EGameplayModOp::Additive);
	}

	UKCApplyGameplayEffectFragment* DamageFragment =
		NewObject<UKCApplyGameplayEffectFragment>();
	DamageFragment->EffectRecipe.EffectClass =
		UKCGE_Damage::StaticClass();
	FKCSetByCallerValueStruct DamageValue;
	DamageValue.DataTag = TAG_KC_Data_Damage_Flat;
	DamageValue.Magnitude = -25.0f;
	DamageFragment->EffectRecipe.SetByCallers.Add(DamageValue);

	FString Error;
	TestTrue(
		TEXT("음수 데미지 Delta를 가진 Fragment Recipe는 유효하다."),
		DamageFragment->Validate(Error));

	return true;
}

#endif
