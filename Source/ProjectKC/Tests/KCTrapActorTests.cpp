#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AbilitySystemInterface.h"
#include "ProjectKC/Trap/KCAbilityTrapActor.h"
#include "ProjectKC/Trap/KCTrapActorBase.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FKCTrapActorHierarchyTest,
	"ProjectKC.Trap.Hierarchy.Contracts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKCTrapActorHierarchyTest::RunTest(const FString& Parameters)
{
	TestTrue(
		TEXT("GAS 함정은 공통 함정 Actor를 상속한다."),
		AKCAbilityTrapActor::StaticClass()->IsChildOf(
			AKCTrapActorBase::StaticClass()));
	TestFalse(
		TEXT("공통 함정 Actor는 GAS 인터페이스에 의존하지 않는다."),
		AKCTrapActorBase::StaticClass()->ImplementsInterface(
			UAbilitySystemInterface::StaticClass()));
	TestTrue(
		TEXT("GAS 함정만 AbilitySystemInterface를 구현한다."),
		AKCAbilityTrapActor::StaticClass()->ImplementsInterface(
			UAbilitySystemInterface::StaticClass()));

	const FProperty* TriggerModeProperty = FindFProperty<FProperty>(
		AKCTrapActorBase::StaticClass(),
		TEXT("TriggerMode"));
	TestTrue(
		TEXT("TriggerMode은 공통 부모가 소유한다."),
		TriggerModeProperty &&
			TriggerModeProperty->GetOwnerClass() ==
				AKCTrapActorBase::StaticClass());
	TestNotNull(
		TEXT("공통 부모는 Blueprint 실행 이벤트를 제공한다."),
		AKCTrapActorBase::StaticClass()->FindFunctionByName(
			TEXT("ExecuteTrap")));

	return true;
}

#endif
