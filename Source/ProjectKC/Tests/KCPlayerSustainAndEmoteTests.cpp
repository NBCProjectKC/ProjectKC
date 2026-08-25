#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "ProjectKC/AbilitySystem/Attribute/KCCharacterAttributeSet.h"
#include "ProjectKC/AbilitySystem/Effect/KCGE_StaminaRegen.h"
#include "ProjectKC/Player/Component/KCEmoteComponent.h"
#include "ProjectKC/Player/KCPlayerCharacter.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FKCPlayerSustainAndEmoteContractTest,
	"ProjectKC.Player.SustainAndEmote.Contract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKCPlayerSustainAndEmoteContractTest::RunTest(const FString& Parameters)
{
	const AKCPlayerCharacter* CharacterCDO = GetDefault<AKCPlayerCharacter>();
	const UKCEmoteComponent* EmoteComponent =
		CharacterCDO->GetEmoteComponent();
	TestNotNull(
		TEXT("플레이어 CDO가 감정표현 컴포넌트를 소유한다."),
		EmoteComponent);
	if (EmoteComponent)
	{
		TestTrue(
			TEXT("감정표현 컴포넌트는 네트워크 복제를 사용한다."),
			EmoteComponent->GetIsReplicated());
	}

	const UKCGE_StaminaRegen* RegenEffect =
		GetDefault<UKCGE_StaminaRegen>();
	TestTrue(
		TEXT("Stamina 자연재생은 무한 지속 GE다."),
		RegenEffect->DurationPolicy == EGameplayEffectDurationType::Infinite);
	TestEqual(
		TEXT("Stamina 자연재생은 0.2초 주기다."),
		RegenEffect->Period.GetValueAtLevel(1.0f),
		0.2f);
	TestFalse(
		TEXT("자연재생은 적용 순간 추가 회복하지 않는다."),
		RegenEffect->bExecutePeriodicEffectOnApplication);
	TestEqual(
		TEXT("Stamina 자연재생 GE는 Modifier 하나를 가진다."),
		RegenEffect->Modifiers.Num(),
		1);
	if (RegenEffect->Modifiers.Num() == 1)
	{
		const FGameplayModifierInfo& Modifier = RegenEffect->Modifiers[0];
		float RegenMagnitude = 0.0f;
		TestTrue(
			TEXT("자연재생은 Stamina Attribute를 수정한다."),
			Modifier.Attribute ==
				UKCCharacterAttributeSet::GetStaminaAttribute());
		TestTrue(
			TEXT("자연재생량은 정적 수치다."),
			Modifier.ModifierMagnitude.GetStaticMagnitudeIfPossible(
				1.0f,
				RegenMagnitude));
		TestEqual(
			TEXT("0.2초마다 Stamina 2, 즉 초당 10을 회복한다."),
			RegenMagnitude,
			2.0f);
	}

	return true;
}

#endif
