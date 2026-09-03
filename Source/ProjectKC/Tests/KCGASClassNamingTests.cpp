#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Abilities/GameplayAbility.h"
#include "GameplayEffect.h"
#include "UObject/UObjectIterator.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FKCGASClassNamingConventionTest,
	"ProjectKC.GAS.Naming.NativeClassPrefixes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKCGASClassNamingConventionTest::RunTest(const FString& Parameters)
{
	int32 AbilityClassCount = 0;
	int32 EffectClassCount = 0;

	for (TObjectIterator<UClass> ClassIterator; ClassIterator; ++ClassIterator)
	{
		const UClass* Class = *ClassIterator;
		if (!Class || Class->GetOutermost()->GetName() != TEXT("/Script/ProjectKC"))
		{
			continue;
		}

		if (Class->IsChildOf(UGameplayAbility::StaticClass()))
		{
			++AbilityClassCount;
			TestTrue(
				*FString::Printf(
					TEXT("GameplayAbility 클래스 '%s'는 KCGA_ 접두사를 사용한다."),
					*Class->GetName()),
				Class->GetName().StartsWith(TEXT("KCGA_")));
		}
		else if (Class->IsChildOf(UGameplayEffect::StaticClass()))
		{
			++EffectClassCount;
			TestTrue(
				*FString::Printf(
					TEXT("GameplayEffect 클래스 '%s'는 KCGE_ 접두사를 사용한다."),
					*Class->GetName()),
				Class->GetName().StartsWith(TEXT("KCGE_")));
		}
	}

	// 개수를 고정해 두면 새 GA/GE를 추가할 때 이 테스트가 반드시 한 번 눈에 띈다.
	// GA: Base, ActionRuntimeBase, Action, ChannelAction, PlayerDash
	// GE: Damage, CookingProgressIncrease, CookingProgressDecrease, StaminaRegen,
	//     DashCost, DashCooldown, ActionCooldown
	TestEqual(TEXT("ProjectKC 네이티브 GA 클래스 수"), AbilityClassCount, 5);
	TestEqual(TEXT("ProjectKC 네이티브 GE 클래스 수"), EffectClassCount, 7);
	return true;
}

#endif
