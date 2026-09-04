#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"
#include "Engine/World.h"
#include "ProjectKC/Item/Component/KCHeldItemComponent.h"
#include "ProjectKC/Item/Definition/KCItemDefinition.h"
#include "ProjectKC/Item/KCWorldItemActor.h"
#include "ProjectKC/Player/KCPlayerCharacter.h"

namespace KCHeldItemSocketTests
{
	const FName HandSocket(TEXT("HandItem"));
	const FName BackSocket(TEXT("BackSlot"));

	UKCItemDefinition* MakeDefinition(UObject* Outer, FName SocketOverride)
	{
		UKCItemDefinition* Definition = NewObject<UKCItemDefinition>(Outer);
		Definition->ItemId =
			FGameplayTag::RequestGameplayTag(TEXT("Item.Id.SeaUrchin"));
		Definition->DisplayName = FText::FromString(TEXT("Socket Test Item"));
		Definition->Presentation.StaticMesh = NewObject<UStaticMesh>(Definition);
		Definition->Presentation.bSimulatePhysicsInWorld = false;
		Definition->Presentation.HolderSocketNameOverride = SocketOverride;
		return Definition;
	}

	/** HandItem과 BackSlot 소켓을 모두 제공하는 Holder 부착용 Mesh다. */
	UStaticMeshComponent* MakeSocketMesh(AActor* Holder)
	{
		UStaticMesh* SocketStaticMesh = NewObject<UStaticMesh>(Holder);
		for (const FName SocketName : {HandSocket, BackSocket})
		{
			UStaticMeshSocket* Socket =
				NewObject<UStaticMeshSocket>(SocketStaticMesh);
			Socket->SocketName = SocketName;
			SocketStaticMesh->Sockets.Add(Socket);
		}

		UStaticMeshComponent* SocketMesh = NewObject<UStaticMeshComponent>(Holder);
		SocketMesh->SetupAttachment(Holder->GetRootComponent());
		SocketMesh->SetStaticMesh(SocketStaticMesh);
		SocketMesh->RegisterComponent();
		return SocketMesh;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FKCHeldItemSocketOverrideTest,
	"ProjectKC.Item.HeldItem.SocketOverride",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKCHeldItemSocketOverrideTest::RunTest(const FString& Parameters)
{
	using namespace KCHeldItemSocketTests;

	const FName TestWorldName = MakeUniqueObjectName(
		nullptr,
		UWorld::StaticClass(),
		TEXT("KCHeldItemSocketTestWorld"),
		EUniqueObjectNameOptions::GloballyUnique);
	UWorld* TestWorld = UWorld::CreateWorld(
		EWorldType::Game,
		false,
		TestWorldName,
		GetTransientPackage());
	if (!TestNotNull(TEXT("소켓 검증용 World를 만든다."), TestWorld))
	{
		return false;
	}

	FWorldContext& WorldContext =
		GEngine->CreateNewWorldContext(EWorldType::Game);
	WorldContext.SetCurrentWorld(TestWorld);

	AKCPlayerCharacter* Holder = TestWorld->SpawnActor<AKCPlayerCharacter>();
	if (TestNotNull(TEXT("소켓 검증용 Holder를 스폰한다."), Holder))
	{
		UKCHeldItemComponent* HeldItemComponent =
			Holder->GetHeldItemComponent();
		TestTrue(
			TEXT("Holder의 기본 손 소켓을 설정한다."),
			HeldItemComponent->ConfigureAttachment(
				MakeSocketMesh(Holder),
				HandSocket));

		TestEqual(
			TEXT("Definition이 없으면 Holder의 기본 손 소켓을 쓴다."),
			HeldItemComponent->ResolveHolderSocketName(nullptr),
			HandSocket);

		UKCItemDefinition* PlainDefinition = MakeDefinition(Holder, NAME_None);
		TestEqual(
			TEXT("Override가 비어 있으면 Holder의 기본 손 소켓을 쓴다."),
			HeldItemComponent->ResolveHolderSocketName(PlainDefinition),
			HandSocket);

		UKCItemDefinition* OverrideDefinition =
			MakeDefinition(Holder, BackSocket);
		TestEqual(
			TEXT("아이템이 지정한 소켓이 Holder 기본값을 덮어쓴다."),
			HeldItemComponent->ResolveHolderSocketName(OverrideDefinition),
			BackSocket);

		AKCWorldItemActor* OverrideItem =
			TestWorld->SpawnActor<AKCWorldItemActor>();
		if (TestNotNull(TEXT("소켓을 덮어쓰는 아이템을 스폰한다."), OverrideItem))
		{
			TestTrue(
				TEXT("소켓을 덮어쓰는 아이템을 초기화한다."),
				OverrideItem->InitializeItem(OverrideDefinition));
			TestTrue(
				TEXT("Holder가 소켓을 덮어쓰는 아이템을 든다."),
				HeldItemComponent->TryPickUp(OverrideItem));
			TestEqual(
				TEXT("아이템이 실제로 지정한 소켓에 부착된다."),
				OverrideItem->GetAttachParentSocketName(),
				BackSocket);
			TestEqual(
				TEXT("Holder의 기본 손 소켓 설정은 그대로 남는다."),
				HeldItemComponent->GetHandSocketName(),
				HandSocket);
		}
	}

	TestWorld->DestroyWorld(false);
	GEngine->DestroyWorldContext(TestWorld);
	return true;
}

#endif
