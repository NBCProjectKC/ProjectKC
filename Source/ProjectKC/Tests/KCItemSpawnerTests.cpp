#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "ProjectKC/Item/Definition/KCItemDefinition.h"
#include "ProjectKC/Item/KCItemSpawnerActor.h"
#include "ProjectKC/Item/KCWorldItemActor.h"
#include "ProjectKC/Item/Tag/KCItemTags.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FKCItemSpawnerTest,
	"ProjectKC.Item.Spawner.Contracts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKCItemSpawnerTest::RunTest(const FString& Parameters)
{
	TestFalse(
		TEXT("스포너는 아이템 Actor를 상속하지 않는 독립 Actor다."),
		AKCItemSpawnerActor::StaticClass()->IsChildOf(
			AKCWorldItemActor::StaticClass()));
	TestNotNull(
		TEXT("Blueprint에서 주기를 시작할 수 있다."),
		AKCItemSpawnerActor::StaticClass()->FindFunctionByName(
			TEXT("StartSpawning")));
	TestNotNull(
		TEXT("Blueprint에서 주기를 멈출 수 있다."),
		AKCItemSpawnerActor::StaticClass()->FindFunctionByName(
			TEXT("StopSpawning")));
	TestNotNull(
		TEXT("Blueprint에서 즉시 한 개를 스폰할 수 있다."),
		AKCItemSpawnerActor::StaticClass()->FindFunctionByName(
			TEXT("SpawnItem")));

	const FClassProperty* ItemActorClassProperty = FindFProperty<FClassProperty>(
		AKCItemSpawnerActor::StaticClass(),
		TEXT("ItemActorClass"));
	if (TestNotNull(
		TEXT("스포너는 ItemActorClass 프로퍼티를 노출한다."),
		ItemActorClassProperty))
	{
		TestTrue(
			TEXT("기본 스폰 클래스는 월드 아이템 Actor다."),
			ItemActorClassProperty->GetObjectPropertyValue_InContainer(
				GetDefault<AKCItemSpawnerActor>()) ==
				AKCWorldItemActor::StaticClass());
	}

	const FName TestWorldName = MakeUniqueObjectName(
		nullptr,
		UWorld::StaticClass(),
		TEXT("KCItemSpawnerTestWorld"),
		EUniqueObjectNameOptions::GloballyUnique);
	UWorld* TestWorld = UWorld::CreateWorld(
		EWorldType::Game,
		false,
		TestWorldName,
		GetTransientPackage());
	if (!TestNotNull(TEXT("스폰 검증용 테스트 World를 만든다."), TestWorld))
	{
		return false;
	}

	FWorldContext& WorldContext =
		GEngine->CreateNewWorldContext(EWorldType::Game);
	WorldContext.SetCurrentWorld(TestWorld);

	AKCItemSpawnerActor* Spawner =
		TestWorld->SpawnActor<AKCItemSpawnerActor>();
	FObjectProperty* DefinitionProperty = FindFProperty<FObjectProperty>(
		AKCItemSpawnerActor::StaticClass(),
		TEXT("ItemDefinition"));
	if (TestNotNull(TEXT("테스트 스포너를 스폰한다."), Spawner) &&
		TestNotNull(
			TEXT("스포너는 ItemDefinition 프로퍼티를 노출한다."),
			DefinitionProperty))
	{
		UKCItemDefinition* Definition = NewObject<UKCItemDefinition>(Spawner);
		Definition->ItemId = TAG_Item_Id_Tomato;
		Definition->DisplayName = FText::FromString(TEXT("Spawner Item"));
		Definition->Presentation.StaticMesh = NewObject<UStaticMesh>(Definition);
		// 테스트 메시에는 충돌 지오메트리가 없어 물리 시뮬레이션을 켤 수 없다.
		Definition->Presentation.bSimulatePhysicsInWorld = false;
		DefinitionProperty->SetObjectPropertyValue_InContainer(
			Spawner,
			Definition);

		AKCWorldItemActor* SpawnedItem = Spawner->SpawnItem();
		if (TestNotNull(
			TEXT("스포너는 즉시 스폰 요청으로 월드 아이템을 만든다."),
			SpawnedItem))
		{
			TestTrue(
				TEXT("스폰된 아이템은 스포너의 Definition을 사용한다."),
				SpawnedItem->GetItemDefinition() == Definition);
			TestTrue(
				TEXT("스폰된 아이템은 BeginPlay 없이도 곧바로 주울 수 있다."),
				SpawnedItem->CanBePickedUp());
			TestTrue(
				TEXT("반경이 0이면 스포너 위치에 그대로 스폰한다."),
				SpawnedItem->GetActorLocation().Equals(
					Spawner->GetActorLocation()));
		}

		DefinitionProperty->SetObjectPropertyValue_InContainer(
			Spawner,
			nullptr);
		Spawner->StartSpawning();
		TestTrue(TEXT("StartSpawning은 주기를 켠다."), Spawner->IsSpawning());

		AddExpectedErrorPlain(
			TEXT("ItemDefinition 또는 ItemActorClass가 없어"));
		TestNull(
			TEXT("Definition이 없으면 아이템을 만들지 않는다."),
			Spawner->SpawnItem());
		TestFalse(
			TEXT("설정이 잘못된 스포너는 주기를 스스로 멈춘다."),
			Spawner->IsSpawning());
	}

	TestWorld->DestroyWorld(false);
	GEngine->DestroyWorldContext(TestWorld);

	return true;
}

#endif
