#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"
#include "Engine/World.h"
#include "ProjectKC/Item/Component/KCHeldItemComponent.h"
#include "ProjectKC/Item/Definition/KCItemDefinition.h"
#include "ProjectKC/Item/KCWorldItemActor.h"
#include "ProjectKC/Item/Tag/KCItemTags.h"

/**
 * 든 아이템이 Holder를 밀 수 있는지 확인한다.
 * 핵심은 물리 시뮬레이션이 아니라 충돌이다.
 * 물리를 끄더라도 충돌이 남아 있으면 Kinematic Body가 캡슐을 밀어낸다.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FKCHeldItemPhysicsTest,
	"ProjectKC.Item.HeldItem.PhysicsAndCollision",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKCHeldItemPhysicsTest::RunTest(const FString& Parameters)
{
	const FName TestWorldName = MakeUniqueObjectName(
		nullptr,
		UWorld::StaticClass(),
		TEXT("KCHeldItemPhysicsTestWorld"),
		EUniqueObjectNameOptions::GloballyUnique);
	UWorld* TestWorld = UWorld::CreateWorld(
		EWorldType::Game,
		false,
		TestWorldName,
		GetTransientPackage());
	if (!TestNotNull(TEXT("장착 검증용 테스트 World를 만든다."), TestWorld))
	{
		return false;
	}

	FWorldContext& WorldContext =
		GEngine->CreateNewWorldContext(EWorldType::Game);
	WorldContext.SetCurrentWorld(TestWorld);

	// Holder는 손 소켓을 제공하는 컴포넌트와 HeldItem 컴포넌트만 갖춘 최소 Actor다.
	AActor* Holder = TestWorld->SpawnActor<AActor>();
	USceneComponent* HolderRoot = NewObject<USceneComponent>(Holder);
	Holder->SetRootComponent(HolderRoot);
	HolderRoot->RegisterComponent();

	UStaticMesh* HandStaticMesh = NewObject<UStaticMesh>(Holder);
	UStaticMeshSocket* HandSocket = NewObject<UStaticMeshSocket>(HandStaticMesh);
	HandSocket->SocketName = TEXT("HandItem");
	HandStaticMesh->Sockets.Add(HandSocket);

	UStaticMeshComponent* HandMesh = NewObject<UStaticMeshComponent>(Holder);
	HandMesh->SetupAttachment(HolderRoot);
	HandMesh->SetStaticMesh(HandStaticMesh);
	HandMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HandMesh->RegisterComponent();

	UKCHeldItemComponent* HeldItemComponent =
		NewObject<UKCHeldItemComponent>(Holder);
	HeldItemComponent->RegisterComponent();

	UKCItemDefinition* Definition = NewObject<UKCItemDefinition>(Holder);
	Definition->ItemId = TAG_Item_Id_Tomato;
	Definition->DisplayName = FText::FromString(TEXT("Held Item"));
	Definition->Presentation.StaticMesh = NewObject<UStaticMesh>(Definition);
	// 테스트 메시에는 충돌 지오메트리가 없어 시뮬레이션을 켤 수 없다.
	// 이 테스트가 확인하는 것은 상태 전이마다 충돌이 어떻게 바뀌는가다.
	Definition->Presentation.bSimulatePhysicsInWorld = false;

	AKCWorldItemActor* Item = TestWorld->SpawnActor<AKCWorldItemActor>();
	UStaticMeshComponent* ItemMesh = Item ? Item->GetItemMesh() : nullptr;
	if (TestNotNull(TEXT("테스트 아이템을 스폰한다."), Item) &&
		TestNotNull(TEXT("아이템 Mesh가 존재한다."), ItemMesh) &&
		TestTrue(
			TEXT("아이템에 Definition을 넣는다."),
			Item->InitializeItem(Definition)))
	{
		// 1) 월드 상태 — 캐릭터를 막는 충돌이 켜져 있다.
		TestTrue(
			TEXT("월드에 놓인 아이템은 충돌이 켜져 있다."),
			ItemMesh->GetCollisionEnabled() ==
				ECollisionEnabled::QueryAndPhysics);
		TestTrue(
			TEXT("월드에 놓인 아이템은 Pawn 채널을 Block한다."),
			ItemMesh->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block);

		// 2) 장착 상태 — 충돌과 물리가 모두 꺼져야 Holder를 밀 수 없다.
		TestTrue(
			TEXT("Holder가 아이템을 든다."),
			HeldItemComponent->TryPickUp(Item));
		TestTrue(
			TEXT("아이템 상태가 Held로 바뀐다."),
			Item->GetItemState() == EKCWorldItemState::Held);
		TestTrue(
			TEXT("든 아이템은 손 소켓에 부착된다."),
			ItemMesh->GetAttachParent() == HandMesh);
		TestTrue(
			TEXT("든 아이템은 충돌이 완전히 꺼진다."),
			ItemMesh->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
		TestFalse(
			TEXT("든 아이템은 물리를 시뮬레이션하지 않는다."),
			ItemMesh->IsSimulatingPhysics());

		// 3) 드롭 상태 — 충돌이 다시 켜지고 Holder에서 분리된다.
		const FTransform DropTransform(
			Holder->GetActorLocation() + FVector(120.0f, 0.0f, 30.0f));
		TestTrue(
			TEXT("Holder가 아이템을 내려놓는다."),
			HeldItemComponent->DropHeldItem(DropTransform));
		TestNull(
			TEXT("내려놓은 아이템은 Holder에서 분리된다."),
			ItemMesh->GetAttachParent());
		TestTrue(
			TEXT("내려놓은 아이템은 충돌이 다시 켜진다."),
			ItemMesh->GetCollisionEnabled() ==
				ECollisionEnabled::QueryAndPhysics);
	}

	TestWorld->DestroyWorld(false);
	GEngine->DestroyWorldContext(TestWorld);

	return true;
}

#endif
