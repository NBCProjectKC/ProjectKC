#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "ProjectKC/AbilitySystem/Ability/KCGA_PlayerDash.h"
#include "ProjectKC/AbilitySystem/Attribute/KCCharacterAttributeSet.h"
#include "ProjectKC/AbilitySystem/Effect/KCGE_Dash.h"
#include "ProjectKC/AbilitySystem/Tag/KCAbilityGameplayTags.h"
#include "ProjectKC/Player/KCPlayerCharacter.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FKCDashSystemContractTest,
	"ProjectKC.Player.Dash.Contract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKCDashSystemContractTest::RunTest(const FString& Parameters)
{
	const AKCPlayerCharacter* CharacterCDO = GetDefault<AKCPlayerCharacter>();
	TestNotNull(TEXT("플레이어 CDO가 Character AttributeSet을 소유한다."),
		CharacterCDO->GetCharacterAttributes());
	TestEqual(
		TEXT("초기 MaxWalkSpeed는 MoveSpeed Attribute 기본값과 같다."),
		CharacterCDO->GetCharacterMovement()->MaxWalkSpeed,
		CharacterCDO->GetCharacterAttributes()->GetMoveSpeed());

	const UKCGE_DashCost* CostEffect = GetDefault<UKCGE_DashCost>();
	TestTrue(
		TEXT("대시 비용은 Instant GE다."),
		CostEffect->DurationPolicy == EGameplayEffectDurationType::Instant);
	TestEqual(TEXT("대시 비용 GE는 Modifier 하나를 가진다."),
		CostEffect->Modifiers.Num(), 1);
	if (CostEffect->Modifiers.Num() == 1)
	{
		const FGameplayModifierInfo& Modifier = CostEffect->Modifiers[0];
		float CostMagnitude = 0.0f;
		TestTrue(
			TEXT("대시 비용은 Stamina를 수정한다."),
			Modifier.Attribute ==
				UKCCharacterAttributeSet::GetStaminaAttribute());
		TestTrue(
			TEXT("대시 비용은 정적 수치로 계산된다."),
			Modifier.ModifierMagnitude.GetStaticMagnitudeIfPossible(
				1.0f,
				CostMagnitude));
		TestEqual(TEXT("대시는 Stamina 20을 소모한다."), CostMagnitude, -20.0f);
	}

	const UKCGE_DashCooldown* CooldownEffect =
		GetDefault<UKCGE_DashCooldown>();
	float CooldownDuration = 0.0f;
	TestTrue(
		TEXT("대시 쿨다운은 지속시간 GE다."),
		CooldownEffect->DurationPolicy ==
			EGameplayEffectDurationType::HasDuration);
	TestTrue(
		TEXT("대시 쿨다운은 정적 지속시간을 가진다."),
		CooldownEffect->DurationMagnitude.GetStaticMagnitudeIfPossible(
			1.0f,
			CooldownDuration));
	TestEqual(TEXT("대시 쿨다운은 0.8초다."), CooldownDuration, 0.8f);
	TestTrue(
		TEXT("대시 쿨다운 GE가 대시 Cooldown 태그를 부여한다."),
		CooldownEffect->GetGrantedTags().HasTagExact(
			TAG_KC_Cooldown_Ability_Dash));
	const UKCGA_PlayerDash* DashAbility = GetDefault<UKCGA_PlayerDash>();
	TestTrue(
		TEXT("대시 Ability가 Cooldown 태그를 검사한다."),
		DashAbility->GetCooldownTags()->HasTagExact(
			TAG_KC_Cooldown_Ability_Dash));
	TestTrue(
		TEXT("대시는 LocalPredicted Ability다."),
		DashAbility->GetNetExecutionPolicy() ==
			EGameplayAbilityNetExecutionPolicy::LocalPredicted);
	TestTrue(
		TEXT("대시는 캐릭터마다 인스턴스를 유지한다."),
		DashAbility->GetInstancingPolicy() ==
			EGameplayAbilityInstancingPolicy::InstancedPerActor);
	TestTrue(
		TEXT("대시 Ability가 Stamina 비용 GE를 사용한다."),
		DashAbility->GetCostGameplayEffect()->IsA<UKCGE_DashCost>());
	TestTrue(
		TEXT("대시 Ability가 Cooldown GE를 사용한다."),
		DashAbility->GetCooldownGameplayEffect()->IsA<UKCGE_DashCooldown>());
	TestEqual(
		TEXT("대시 몽타주의 기본 재생 속도는 1이다."),
		DashAbility->GetDashMontagePlayRate(),
		1.0f);

	return true;
}

#endif
