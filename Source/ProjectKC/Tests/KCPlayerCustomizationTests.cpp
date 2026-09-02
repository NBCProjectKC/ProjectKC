#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Player/Component/KCPlayerCustomizationComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FKCPlayerCustomizationDefaultsTest,
	"ProjectKC.Customization.Player.Defaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKCPlayerCustomizationDefaultsTest::RunTest(const FString& Parameters)
{
	const UKCPlayerCustomizationComponent* Defaults =
		GetDefault<UKCPlayerCustomizationComponent>();

	TestNotNull(TEXT("눈 메시 기본 에셋이 설정된다."), Defaults->EyeMesh.Get());
	TestNotNull(TEXT("앞치마 메시 기본 에셋이 설정된다."), Defaults->ApronMesh.Get());
	TestNotNull(TEXT("셰프 모자 메시 기본 에셋이 설정된다."), Defaults->ChefHatMesh.Get());
	TestNotNull(TEXT("페인트 머티리얼 기본 에셋이 설정된다."), Defaults->PaintMaterial.Get());
	TestTrue(TEXT("앞치마는 몸체와 같은 원점을 사용한다."),
		Defaults->ApronTransform.Equals(FTransform::Identity));
	TestTrue(TEXT("셰프 모자는 몸체와 같은 원점을 사용한다."),
		Defaults->ChefHatTransform.Equals(FTransform::Identity));

	return true;
}

#endif
