#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AbilitySystemComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshSocket.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "ProjectKC/AbilitySystem/Definition/KCSingleActionDefinition.h"
#include "ProjectKC/AbilitySystem/Fragment/KCApplyGameplayEffectFragment.h"
#include "ProjectKC/AbilitySystem/Tag/KCAbilityGameplayTags.h"
#include "ProjectKC/AbilitySystem/Targeting/KCEventTargeting.h"
#include "ProjectKC/AbilitySystem/Targeting/KCSelfTargeting.h"
#include "ProjectKC/Item/Definition/KCItemDefinition.h"
#include "ProjectKC/Item/Component/KCHeldItemComponent.h"
#include "ProjectKC/Item/KCWorldItemActor.h"
#include "ProjectKC/Player/KCPlayerCharacter.h"

namespace KCItemUseLifecycleTests
{
	UKCItemDefinition* MakeDefinition(
		UObject* Outer,
		EKCItemUseLifecycle UseLifecycle)
	{
		UKCItemDefinition* Definition = NewObject<UKCItemDefinition>(Outer);
		Definition->ItemId = FGameplayTag::RequestGameplayTag(
			TEXT("Item.Id.SeaUrchin"));
		Definition->DisplayName = FText::FromString(TEXT("Consumable Item"));
		Definition->Presentation.StaticMesh = NewObject<UStaticMesh>(Definition);
		Definition->Presentation.bSimulatePhysicsInWorld = false;
		Definition->UseLifecycle = UseLifecycle;

		UKCSingleActionDefinition* Action =
			NewObject<UKCSingleActionDefinition>(Definition);
		Action->ActionTargeting = NewObject<UKCSelfTargeting>(Action);
		FKCActionHookStruct Hook;
		Hook.HookTag = TAG_KC_ActionHook_OnExecute;
		UKCApplyGameplayEffectFragment* Fragment =
			NewObject<UKCApplyGameplayEffectFragment>(Action);
		Fragment->EffectRecipe.EffectClass = UGameplayEffect::StaticClass();
		Fragment->bRequired = true;
		Hook.Fragments.Add(Fragment);
		Action->ActionHooks.Add(MoveTemp(Hook));
		Definition->UseAction = Action;
		return Definition;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FKCItemUseLifecycleRuntimeTest,
	"ProjectKC.Item.UseLifecycle.Runtime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKCItemUseLifecycleRuntimeTest::RunTest(const FString& Parameters)
{
	const FName TestWorldName = MakeUniqueObjectName(
		nullptr,
		UWorld::StaticClass(),
		TEXT("KCItemUseLifecycleTestWorld"),
		EUniqueObjectNameOptions::GloballyUnique);
	UWorld* TestWorld = UWorld::CreateWorld(
		EWorldType::Game,
		false,
		TestWorldName,
		GetTransientPackage());
	if (!TestNotNull(TEXT("사용 수명주기 검증용 World를 만든다."), TestWorld))
	{
		return false;
	}

	FWorldContext& WorldContext =
		GEngine->CreateNewWorldContext(EWorldType::Game);
	WorldContext.SetCurrentWorld(TestWorld);

	AKCWorldItemActor* PersistentItem =
		TestWorld->SpawnActor<AKCWorldItemActor>();
	if (TestNotNull(TEXT("Persistent 검증용 아이템을 스폰한다."), PersistentItem))
	{
		UKCItemDefinition* PersistentDefinition =
			KCItemUseLifecycleTests::MakeDefinition(
				PersistentItem,
				EKCItemUseLifecycle::Persistent);
		TestTrue(
			TEXT("Persistent Definition으로 아이템을 초기화한다."),
			PersistentItem->InitializeItem(PersistentDefinition));
		TestFalse(
			TEXT("Persistent 아이템은 성공 사용 소비를 예약하지 않는다."),
			PersistentItem->TryBeginUseConsumption());
		TestFalse(
			TEXT("Persistent 아이템은 소비 대기 상태가 아니다."),
			PersistentItem->IsUseConsumptionPending());
	}

	AKCWorldItemActor* ConsumableItem =
		TestWorld->SpawnActor<AKCWorldItemActor>();
	if (TestNotNull(TEXT("소비 검증용 아이템을 스폰한다."), ConsumableItem))
	{
		UKCItemDefinition* ConsumableDefinition =
			KCItemUseLifecycleTests::MakeDefinition(
				ConsumableItem,
				EKCItemUseLifecycle::ConsumeOnSuccessfulExecute);
		TestTrue(
			TEXT("ConsumeOnSuccessfulExecute Definition으로 초기화한다."),
			ConsumableItem->InitializeItem(ConsumableDefinition));
		TestTrue(
			TEXT("첫 성공 실행은 아이템 소비를 예약한다."),
			ConsumableItem->TryBeginUseConsumption());
		TestTrue(
			TEXT("소비가 예약된 아이템은 대기 상태다."),
			ConsumableItem->IsUseConsumptionPending());
		TestFalse(
			TEXT("소비가 예약된 아이템은 다시 사용할 수 없다."),
			ConsumableItem->IsUsable());
		TestFalse(
			TEXT("소비가 예약된 아이템은 다시 주울 수 없다."),
			ConsumableItem->CanBePickedUp());
		TestFalse(
			TEXT("소비가 예약되면 원본 표현을 즉시 숨긴다."),
			ConsumableItem->GetItemMesh()->IsVisible());
		TestTrue(
			TEXT("소비 정산 전에는 Source Actor를 유지한다."),
			IsValid(ConsumableItem));
		TestFalse(
			TEXT("같은 아이템의 소비를 두 번 예약할 수 없다."),
			ConsumableItem->TryBeginUseConsumption());

		TestTrue(
			TEXT("Action 정리 뒤 소비 파괴를 확정한다."),
			ConsumableItem->FinalizePendingUseConsumption());
		TestFalse(
			TEXT("소비 파괴는 중복 확정할 수 없다."),
			ConsumableItem->FinalizePendingUseConsumption());
		TestTrue(
			TEXT("확정 호출 스택에서는 Source Actor를 즉시 제거하지 않는다."),
			IsValid(ConsumableItem));

	}

	AKCPlayerCharacter* Holder =
		TestWorld->SpawnActor<AKCPlayerCharacter>();
	AKCWorldItemActor* RuntimeConsumableItem =
		TestWorld->SpawnActor<AKCWorldItemActor>();
	if (TestNotNull(TEXT("실행 경로 검증용 Holder를 스폰한다."), Holder) &&
		TestNotNull(
			TEXT("실행 경로 검증용 소비 아이템을 스폰한다."),
			RuntimeConsumableItem))
	{
		Holder->GetAbilitySystemComponent()->InitAbilityActorInfo(Holder, Holder);

		UStaticMesh* HandStaticMesh = NewObject<UStaticMesh>(Holder);
		UStaticMeshSocket* HandSocket =
			NewObject<UStaticMeshSocket>(HandStaticMesh);
		HandSocket->SocketName = TEXT("HandItem");
		HandStaticMesh->Sockets.Add(HandSocket);
		UStaticMeshComponent* HandMesh =
			NewObject<UStaticMeshComponent>(Holder);
		HandMesh->SetupAttachment(Holder->GetRootComponent());
		HandMesh->SetStaticMesh(HandStaticMesh);
		HandMesh->RegisterComponent();

		UKCHeldItemComponent* HeldItemComponent =
			Holder->GetHeldItemComponent();
		TestTrue(
			TEXT("테스트 Holder의 손 소켓을 설정한다."),
			HeldItemComponent->ConfigureAttachment(
				HandMesh,
				TEXT("HandItem")));

		UKCItemDefinition* RuntimeDefinition =
			KCItemUseLifecycleTests::MakeDefinition(
				RuntimeConsumableItem,
				EKCItemUseLifecycle::ConsumeOnSuccessfulExecute);
		TestTrue(
			TEXT("실행 경로용 소비 아이템을 초기화한다."),
			RuntimeConsumableItem->InitializeItem(RuntimeDefinition));
		TestTrue(
			TEXT("Holder가 실행 경로용 소비 아이템을 든다."),
			HeldItemComponent->TryPickUp(RuntimeConsumableItem));
		TestTrue(
			TEXT("Self Action의 필수 Fragment를 성공적으로 실행한다."),
			HeldItemComponent->PressHeldItemUse());
		TestTrue(
			TEXT("성공한 실제 Action 실행은 원본 소비를 예약한다."),
			RuntimeConsumableItem->IsUseConsumptionPending());
		TestTrue(
			TEXT("Action 호출 스택에서는 원본 아이템을 유지한다."),
			IsValid(RuntimeConsumableItem));
		TestFalse(
			TEXT("Action 종료가 소비 파괴를 자동으로 확정한다."),
			RuntimeConsumableItem->FinalizePendingUseConsumption());

		// 동기식 Automation 안에서는 수동 World Tick이 엔진의 전역 프레임을
		// 증가시키지 않는다. 모든 NextTick 예약을 첫 World Tick 전에 만든다.
		TestWorld->Tick(LEVELTICK_All, 1.0f / 60.0f);
		TestFalse(
			TEXT("직접 확정한 소비 아이템은 다음 틱에 제거된다."),
			IsValid(ConsumableItem));
		TestFalse(
			TEXT("Action 정리 다음 틱에는 원본 소비 아이템이 제거된다."),
			IsValid(RuntimeConsumableItem));
		TestFalse(
			TEXT("소비 파괴 시 Holder 참조와 눌린 입력도 함께 정리된다."),
			HeldItemComponent->HasHeldItem());

		AKCWorldItemActor* FailedConsumableItem =
			TestWorld->SpawnActor<AKCWorldItemActor>();
		AActor* TargetWithoutAbilitySystem = TestWorld->SpawnActor<AActor>();
		if (TestNotNull(
				TEXT("실패 실행 검증용 소비 아이템을 스폰한다."),
				FailedConsumableItem) &&
			TestNotNull(
				TEXT("ASC가 없는 실패 대상을 스폰한다."),
				TargetWithoutAbilitySystem))
		{
			UKCItemDefinition* FailedDefinition =
				KCItemUseLifecycleTests::MakeDefinition(
					FailedConsumableItem,
					EKCItemUseLifecycle::ConsumeOnSuccessfulExecute);
			FailedDefinition->UseAction->ActionTargeting =
				NewObject<UKCEventTargeting>(FailedDefinition->UseAction);
			TestTrue(
				TEXT("필수 Fragment 실패 검증용 아이템을 초기화한다."),
				FailedConsumableItem->InitializeItem(FailedDefinition));
			TestTrue(
				TEXT("Holder가 실패 실행 검증용 아이템을 든다."),
				HeldItemComponent->TryPickUp(FailedConsumableItem));
			TestTrue(
				TEXT("대상을 지정한 Action 활성화 요청 자체는 처리된다."),
				HeldItemComponent->UseHeldItemWithTarget(
					TargetWithoutAbilitySystem));
			TestFalse(
				TEXT("필수 Fragment가 실패하면 원본 소비를 예약하지 않는다."),
				FailedConsumableItem->IsUseConsumptionPending());

			TestWorld->Tick(LEVELTICK_All, 1.0f / 60.0f);
			TestTrue(
				TEXT("실패한 실행 뒤에도 소비 아이템은 손에 남는다."),
				IsValid(FailedConsumableItem) &&
					HeldItemComponent->GetHeldItem() == FailedConsumableItem);
		}
	}

	TestWorld->DestroyWorld(false);
	GEngine->DestroyWorldContext(TestWorld);
	return true;
}

#endif
