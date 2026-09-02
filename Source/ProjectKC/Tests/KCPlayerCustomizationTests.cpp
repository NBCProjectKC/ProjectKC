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

	const FTransform ExpectedApronTransform(
		FRotator::ZeroRotator,
		FVector(0.187529f, 0.087705f, -52.033743f),
		FVector::OneVector);
	const FTransform ExpectedChefHatTransform(
		FRotator(19.998790f, 0.036939f, -0.194630f),
		FVector(27.539447f, -0.048058f, -21.737259f),
		FVector::OneVector);

	TestTrue(TEXT("앞치마 기본 배치값이 유지된다."),
		Defaults->ApronTransform.Equals(ExpectedApronTransform));
	TestTrue(TEXT("셰프 모자 기본 배치값이 유지된다."),
		Defaults->ChefHatTransform.Equals(ExpectedChefHatTransform));

	return true;
}

#endif
