#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"
#include "Engine/World.h"
#include "ProjectKC/Item/Component/KCHeldItemComponent.h"
#include "ProjectKC/Item/Definition/KCItemDefinition.h"
#include "ProjectKC/Item/KCWorldItemActor.h"
#include "ProjectKC/Item/Tag/KCItemTags.h"
#include "ProjectKC/Player/KCPlayerCharacter.h"

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
	// 캐릭터가 아이템을 밀 수 있는 컴포넌트를 갖고 있는지 본다.
	// 눈에 보이는 몸은 스켈레탈 메시 본에 바인딩한 스태틱 메시들이고,
	// 스켈레탈 메시 자체는 숨겨진 애니메이션 드라이버다.
	// 최종 Blueprint를 우선 보는 이유는 BP에서 충돌을 되살렸을 수 있어서다.
	UClass* PlayerBlueprintClass = LoadClass<AKCPlayerCharacter>(
		nullptr,
		TEXT("/Game/KC/Player/Blueprints/BP_KCPlayerCharacter."
			"BP_KCPlayerCharacter_C"));
	const AKCPlayerCharacter* PlayerDefaults = PlayerBlueprintClass
		? PlayerBlueprintClass->GetDefaultObject<AKCPlayerCharacter>()
		: GetDefault<AKCPlayerCharacter>();

	TInlineComponentArray<UStaticMeshComponent*> PlayerMeshes(PlayerDefaults);
	int32 CheckedAvatarParts = 0;
	for (const UStaticMeshComponent* AvatarMesh : PlayerMeshes)
	{
		if (!AvatarMesh || !AvatarMesh->GetName().StartsWith(TEXT("Avatar")))
		{
			continue;
		}

		++CheckedAvatarParts;
		TestTrue(
			*FString::Printf(
				TEXT("보이는 아바타 파츠 '%s'는 충돌이 없다."),
				*AvatarMesh->GetName()),
			AvatarMesh->GetCollisionEnabled() ==
				ECollisionEnabled::NoCollision);
	}
	TestTrue(
		TEXT("검사한 아바타 파츠가 하나 이상이다."),
		CheckedAvatarParts > 0);

	const USkeletalMeshComponent* DriverMesh = PlayerDefaults->GetMesh();
	if (TestNotNull(TEXT("캐릭터에 애니메이션 드라이버 메시가 있다."), DriverMesh))
	{
		TestTrue(
			TEXT("드라이버 메시는 Pawn 오브젝트 타입이라 아이템이 무시한다."),
			DriverMesh->GetCollisionObjectType() == ECC_Pawn);
		TestTrue(
			TEXT("드라이버 메시는 QueryOnly라 물리 접촉을 만들 수 없다."),
			DriverMesh->GetCollisionEnabled() == ECollisionEnabled::QueryOnly);
	}

	const UCapsuleComponent* MovementCapsule =
		PlayerDefaults->GetCapsuleComponent();
	if (TestNotNull(TEXT("캐릭터에 이동용 캡슐이 있다."), MovementCapsule))
	{
		TestTrue(
			TEXT("이동 캡슐은 Pawn 오브젝트 타입이라 아이템이 무시한다."),
			MovementCapsule->GetCollisionObjectType() == ECC_Pawn);
	}

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
		// KCWorldItem 프리셋의 계약이다. Pawn만 빼고 전부 막는다.
		TestTrue(
			TEXT("월드에 놓인 아이템은 Pawn을 무시해 캐릭터 이동을 막지 않는다."),
			ItemMesh->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Ignore);
		TestTrue(
			TEXT("월드에 놓인 아이템은 바닥과 벽은 그대로 막는다."),
			ItemMesh->GetCollisionResponseToChannel(ECC_WorldStatic) == ECR_Block);
		TestTrue(
			TEXT("월드에 놓인 아이템끼리는 그대로 충돌한다."),
			ItemMesh->GetCollisionResponseToChannel(ECC_PhysicsBody) == ECR_Block);
		TestTrue(
			TEXT("월드에 놓인 아이템은 상호작용 시야 트레이스에 잡힌다."),
			ItemMesh->GetCollisionResponseToChannel(ECC_Visibility) == ECR_Block);

		// 캐릭터를 실제로 스폰한다. Blueprint가 SCS로 붙인 컴포넌트는
		// CDO에 없어서 스폰해야만 보인다.
		//
		// 계약은 '이동 캡슐이 아이템에 막히지 않는다' 하나다.
		// 장식 컴포넌트가 아이템을 밀어내는 것은 의도된 동작이므로 막지 않는다.
		// 이동 판정과 무관해 서버 보정을 유발하지 않기 때문이다.
		// 어떤 컴포넌트가 아이템과 상호작용하는지는 아래에 정보로만 남긴다.
		AKCPlayerCharacter* PlayerInstance = PlayerBlueprintClass
			? TestWorld->SpawnActor<AKCPlayerCharacter>(PlayerBlueprintClass)
			: nullptr;
		if (TestNotNull(TEXT("최종 Player Blueprint를 스폰한다."), PlayerInstance))
		{
			const UCapsuleComponent* MovementCollision =
				PlayerInstance->GetCapsuleComponent();
			if (TestNotNull(
				TEXT("스폰된 캐릭터에 이동 캡슐이 있다."),
				MovementCollision))
			{
				TestTrue(
					TEXT("이동 캡슐은 아이템에 막히지 않는다."),
					ItemMesh->GetCollisionResponseToChannel(
						MovementCollision->GetCollisionObjectType()) ==
						ECR_Ignore);
			}

			TInlineComponentArray<UPrimitiveComponent*> PlayerPrimitives(
				PlayerInstance);
			for (const UPrimitiveComponent* Primitive : PlayerPrimitives)
			{
				if (!Primitive || Primitive == MovementCollision ||
					Primitive->GetCollisionEnabled() ==
						ECollisionEnabled::NoCollision)
				{
					continue;
				}

				// 막히려면 양쪽이 모두 Block이어야 한다.
				const bool bPushesWorldItem =
					Primitive->GetCollisionResponseToChannel(ECC_PhysicsBody) ==
						ECR_Block &&
					ItemMesh->GetCollisionResponseToChannel(
						Primitive->GetCollisionObjectType()) == ECR_Block;
				if (bPushesWorldItem)
				{
					AddInfo(FString::Printf(
						TEXT("장식 컴포넌트 '%s'(프리셋 %s)가 월드 아이템을 ")
						TEXT("밀어낸다. 의도된 동작이다."),
						*Primitive->GetName(),
						*Primitive->GetCollisionProfileName().ToString()));
				}
			}
		}
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
