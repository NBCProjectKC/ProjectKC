#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "ProjectKC/AbilitySystem/Definition/KCSingleActionDefinition.h"
#include "ProjectKC/AbilitySystem/Fragment/KCApplyGameplayEffectFragment.h"
#include "ProjectKC/AbilitySystem/Tag/KCAbilityGameplayTags.h"
#include "ProjectKC/AbilitySystem/Targeting/KCSelfTargeting.h"
#include "ProjectKC/Item/Definition/KCItemDefinition.h"
#include "ProjectKC/Item/KCWorldItemActor.h"

namespace KCItemDurabilityTests
{
	UKCItemDefinition* MakeOnUseDefinition(UObject* Outer)
	{
		UKCItemDefinition* Definition = NewObject<UKCItemDefinition>(Outer);
		Definition->ItemId = FGameplayTag::RequestGameplayTag(
			TEXT("Item.Id.FryingPan"));
		Definition->DisplayName = FText::FromString(TEXT("Durable Item"));
		Definition->Presentation.StaticMesh = NewObject<UStaticMesh>(Definition);
		Definition->Presentation.bSimulatePhysicsInWorld = false;

		UKCSingleActionDefinition* Action =
			NewObject<UKCSingleActionDefinition>(Definition);
		Action->ActionTargeting = NewObject<UKCSelfTargeting>(Action);
		FKCActionHookStruct Hook;
		Hook.HookTag = TAG_KC_ActionHook_OnExecute;
		UKCApplyGameplayEffectFragment* Fragment =
			NewObject<UKCApplyGameplayEffectFragment>(Action);
		Fragment->EffectRecipe.EffectClass = UGameplayEffect::StaticClass();
		Hook.Fragments.Add(Fragment);
		Action->ActionHooks.Add(MoveTemp(Hook));
		Definition->UseAction = Action;
		Definition->Durability.ConsumeMode =
			EKCItemDurabilityConsumeMode::OnUse;
		Definition->Durability.ConsumeAmount = 25.0f;
		return Definition;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FKCItemDurabilityRuntimeTest,
	"ProjectKC.Item.Durability.Runtime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKCItemDurabilityRuntimeTest::RunTest(const FString& Parameters)
{
	const FName TestWorldName = MakeUniqueObjectName(
		nullptr,
		UWorld::StaticClass(),
		TEXT("KCItemDurabilityTestWorld"),
		EUniqueObjectNameOptions::GloballyUnique);
	UWorld* TestWorld = UWorld::CreateWorld(
		EWorldType::Game,
		false,
		TestWorldName,
		GetTransientPackage());
	if (!TestNotNull(TEXT("내구도 검증용 테스트 World를 만든다."), TestWorld))
	{
		return false;
	}

	FWorldContext& WorldContext =
		GEngine->CreateNewWorldContext(EWorldType::Game);
	WorldContext.SetCurrentWorld(TestWorld);

	AKCWorldItemActor* Item = TestWorld->SpawnActor<AKCWorldItemActor>();
	if (TestNotNull(TEXT("내구도 검증용 아이템을 스폰한다."), Item))
	{
		UKCItemDefinition* Definition =
			KCItemDurabilityTests::MakeOnUseDefinition(Item);
		TestTrue(
			TEXT("유효한 내구도 Definition으로 아이템을 초기화한다."),
			Item->InitializeItem(Definition));
		TestEqual(
			TEXT("새 아이템은 최대 내구도 100으로 시작한다."),
			Item->GetCurrentDurability(),
			100.0f);
		TestFalse(
			TEXT("설정과 다른 소모 시점은 내구도를 변경하지 않는다."),
			Item->TryConsumeDurability(
				EKCItemDurabilityConsumeMode::OnFirstHit));
		TestEqual(
			TEXT("잘못된 소모 요청 뒤에도 내구도를 유지한다."),
			Item->GetCurrentDurability(),
			100.0f);

		for (int32 UseIndex = 0; UseIndex < 4; ++UseIndex)
		{
			TestTrue(
				*FString::Printf(
					TEXT("%d번째 사용은 내구도를 소모한다."),
					UseIndex + 1),
				Item->TryConsumeDurability(
					EKCItemDurabilityConsumeMode::OnUse));
		}

		TestEqual(
			TEXT("25씩 네 번 소모하면 내구도가 0이 된다."),
			Item->GetCurrentDurability(),
			0.0f);
		TestTrue(TEXT("내구도 0인 아이템은 파손 상태다."), Item->IsBroken());
		TestFalse(TEXT("파손된 아이템은 사용할 수 없다."), Item->IsUsable());
		TestFalse(
			TEXT("파손 뒤의 추가 소모 요청은 거부한다."),
			Item->TryConsumeDurability(
				EKCItemDurabilityConsumeMode::OnUse));
		TestEqual(
			TEXT("파손된 아이템의 정규화 내구도는 0이다."),
			Item->GetDurabilityNormalized(),
			0.0f);
	}

	AKCWorldItemActor* DestroyItem =
		TestWorld->SpawnActor<AKCWorldItemActor>();
	if (TestNotNull(TEXT("파괴 정책 검증용 아이템을 스폰한다."), DestroyItem))
	{
		UKCItemDefinition* DestroyDefinition =
			KCItemDurabilityTests::MakeOnUseDefinition(DestroyItem);
		DestroyDefinition->Durability.ConsumeAmount = 100.0f;
		DestroyDefinition->Durability.BreakBehavior =
			EKCItemBreakBehavior::Destroy;
		TestTrue(
			TEXT("Destroy 정책 Definition으로 아이템을 초기화한다."),
			DestroyItem->InitializeItem(DestroyDefinition));
		TestTrue(
			TEXT("내구도를 0으로 만드는 사용은 성공한다."),
			DestroyItem->TryConsumeDurability(
				EKCItemDurabilityConsumeMode::OnUse));
		TestTrue(
			TEXT("소모 처리 중에는 Source Actor를 즉시 파괴하지 않는다."),
			IsValid(DestroyItem));

		TestWorld->Tick(LEVELTICK_All, 1.0f / 60.0f);
		TestFalse(
			TEXT("다음 틱에는 Destroy 정책 아이템이 파괴된다."),
			IsValid(DestroyItem));
	}

	TestWorld->DestroyWorld(false);
	GEngine->DestroyWorldContext(TestWorld);
	return true;
}

#endif
