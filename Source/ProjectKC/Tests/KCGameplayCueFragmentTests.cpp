#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "ProjectKC/AbilitySystem/Fragment/KCExecuteGameplayCueFragment.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FKCGameplayCueDirectionTest,
	"ProjectKC.GAS.Cue.Direction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKCGameplayCueDirectionTest::RunTest(const FString& Parameters)
{
	const FName TestWorldName = MakeUniqueObjectName(
		nullptr,
		UWorld::StaticClass(),
		TEXT("KCGameplayCueDirectionTestWorld"),
		EUniqueObjectNameOptions::GloballyUnique);
	UWorld* TestWorld = UWorld::CreateWorld(
		EWorldType::Game,
		false,
		TestWorldName,
		GetTransientPackage());
	if (!TestNotNull(TEXT("Cue 방향 검증용 World를 만든다."), TestWorld))
	{
		return false;
	}

	FWorldContext& WorldContext =
		GEngine->CreateNewWorldContext(EWorldType::Game);
	WorldContext.SetCurrentWorld(TestWorld);

	AActor* SourceActor = TestWorld->SpawnActor<AActor>();
	if (TestNotNull(TEXT("방향 기준이 될 소스 Actor를 스폰한다."), SourceActor))
	{
		USceneComponent* Root = NewObject<USceneComponent>(SourceActor);
		SourceActor->SetRootComponent(Root);
		Root->RegisterComponent();
		SourceActor->SetActorRotation(FRotator::ZeroRotator);

		UKCExecuteGameplayCueFragment* Fragment =
			NewObject<UKCExecuteGameplayCueFragment>(SourceActor);
		Fragment->DirectionMode = EKCGameplayCueDirectionMode::SourceForward;

		FKCActionExecutionContext Context;
		Context.SourceActor = SourceActor;

		TestEqual(
			TEXT("오프셋 기본값은 소스 정면을 그대로 쓴다."),
			Fragment->ResolveDirection(Context),
			FVector::ForwardVector,
			0.001f);

		Fragment->DirectionOffset = FRotator(0.0f, 180.0f, 0.0f);
		TestEqual(
			TEXT("Yaw 180은 소스의 뒤를 향한다."),
			Fragment->ResolveDirection(Context),
			-FVector::ForwardVector,
			0.001f);

		Fragment->DirectionOffset = FRotator(180.0f, 0.0f, 0.0f);
		TestEqual(
			TEXT("Pitch 180도 소스의 뒤를 향한다."),
			Fragment->ResolveDirection(Context),
			-FVector::ForwardVector,
			0.001f);

		Fragment->DirectionOffset = FRotator(90.0f, 0.0f, 0.0f);
		TestEqual(
			TEXT("Pitch 90은 위를 향한다."),
			Fragment->ResolveDirection(Context),
			FVector::UpVector,
			0.001f);

		Fragment->DirectionOffset = FRotator(0.0f, 90.0f, 0.0f);
		TestEqual(
			TEXT("Yaw 90은 소스의 오른쪽을 향한다."),
			Fragment->ResolveDirection(Context),
			FVector::RightVector,
			0.001f);

		// Cue는 방향 벡터만 실어 나르므로 Roll은 결과를 바꾸지 못한다.
		Fragment->DirectionOffset = FRotator(0.0f, 0.0f, 180.0f);
		TestEqual(
			TEXT("Roll은 방향을 바꾸지 못한다."),
			Fragment->ResolveDirection(Context),
			FVector::ForwardVector,
			0.001f);

		// 오프셋은 월드 축이 아니라 구한 방향의 로컬 프레임에서 돌아야 한다.
		SourceActor->SetActorRotation(FRotator(0.0f, 90.0f, 0.0f));
		Fragment->DirectionOffset = FRotator(0.0f, 180.0f, 0.0f);
		TestEqual(
			TEXT("소스가 돌아가 있어도 Yaw 180은 그 소스의 뒤를 향한다."),
			Fragment->ResolveDirection(Context),
			-FVector::RightVector,
			0.001f);

		// FromContext는 방향을 지정하지 않는 모드라 오프셋이 끼어들면 안 된다.
		Fragment->DirectionMode = EKCGameplayCueDirectionMode::FromContext;
		TestEqual(
			TEXT("FromContext는 오프셋이 있어도 방향을 만들지 않는다."),
			Fragment->ResolveDirection(Context),
			FVector::ZeroVector,
			0.001f);
	}

	TestWorld->DestroyWorld(false);
	GEngine->DestroyWorldContext(TestWorld);
	return true;
}

#endif
