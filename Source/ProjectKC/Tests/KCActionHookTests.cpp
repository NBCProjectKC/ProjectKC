#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"
#include "Engine/World.h"
#include "ProjectKC/AbilitySystem/Attribute/KCCharacterAttributeSet.h"
#include "ProjectKC/AbilitySystem/Definition/KCSingleActionDefinition.h"
#include "ProjectKC/AbilitySystem/Effect/KCGE_Damage.h"
#include "ProjectKC/AbilitySystem/Fragment/KCApplyGameplayEffectFragment.h"
#include "ProjectKC/AbilitySystem/Struct/KCSetByCallerValueStruct.h"
#include "ProjectKC/AbilitySystem/Tag/KCAbilityGameplayTags.h"
#include "ProjectKC/AbilitySystem/Targeting/KCSweepTargeting.h"
#include "ProjectKC/Item/Component/KCHeldItemComponent.h"
#include "ProjectKC/Item/Definition/KCItemDefinition.h"
#include "ProjectKC/Item/KCWorldItemActor.h"
#include "ProjectKC/Player/KCPlayerCharacter.h"

namespace KCActionHookTests
{
	/** Hook이 돌았는지 체력 변화로 관측하기 위한 소스 대상 자해 Fragment다. */
	UKCApplyGameplayEffectFragment* MakeSelfDamageFragment(
		UObject* Outer,
		float Magnitude)
	{
		UKCApplyGameplayEffectFragment* Fragment =
			NewObject<UKCApplyGameplayEffectFragment>(Outer);
		Fragment->ApplicationScope = EKCActionScope::Source;
		Fragment->EffectRecipe.EffectClass = UKCGE_Damage::StaticClass();
		FKCSetByCallerValueStruct Value;
		Value.DataTag = TAG_KC_Data_Damage_Flat;
		Value.Magnitude = Magnitude;
		Fragment->EffectRecipe.SetByCallers.Add(Value);
		return Fragment;
	}

	/**
	 * 몽타주 없이 즉시 판정하고, 빈 World라 Sweep이 아무도 잡지 못하는 Definition이다.
	 * OnExecuteStart, OnExecute, OnConfirmedHit의 실행 조건 차이를 가른다.
	 */
	UKCItemDefinition* MakeSwingDefinition(UObject* Outer)
	{
		UKCItemDefinition* Definition = NewObject<UKCItemDefinition>(Outer);
		Definition->ItemId = FGameplayTag::RequestGameplayTag(
			TEXT("Item.Id.FryingPan"));
		Definition->DisplayName = FText::FromString(TEXT("Swing Item"));
		Definition->Presentation.StaticMesh = NewObject<UStaticMesh>(Definition);
		Definition->Presentation.bSimulatePhysicsInWorld = false;
		Definition->UseLifecycle = EKCItemUseLifecycle::Persistent;

		UKCSingleActionDefinition* Action =
			NewObject<UKCSingleActionDefinition>(Definition);
		UKCSweepTargeting* Targeting = NewObject<UKCSweepTargeting>(Action);
		Targeting->bRequireUnobstructedPath = false;
		Action->ActionTargeting = Targeting;

		FKCActionHookStruct ExecuteStartHook;
		ExecuteStartHook.HookTag = TAG_KC_ActionHook_OnExecuteStart;
		ExecuteStartHook.Fragments.Add(
			MakeSelfDamageFragment(Action, -5.0f));
		Action->ActionHooks.Add(MoveTemp(ExecuteStartHook));

		FKCActionHookStruct ExecuteHook;
		ExecuteHook.HookTag = TAG_KC_ActionHook_OnExecute;
		ExecuteHook.Fragments.Add(
			MakeSelfDamageFragment(Action, -50.0f));
		Action->ActionHooks.Add(MoveTemp(ExecuteHook));

		FKCActionHookStruct ConfirmedHitHook;
		ConfirmedHitHook.HookTag = TAG_KC_ActionHook_OnConfirmedHit;
		ConfirmedHitHook.Fragments.Add(
			MakeSelfDamageFragment(Action, -7.0f));
		Action->ActionHooks.Add(MoveTemp(ConfirmedHitHook));

		Definition->UseAction = Action;
		return Definition;
	}

	bool ConfigureHolderHand(
		AKCPlayerCharacter* Holder,
		UKCHeldItemComponent* HeldItemComponent)
	{
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
		return HeldItemComponent->ConfigureAttachment(
			HandMesh,
			TEXT("HandItem"));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FKCActionExecuteStartHookTest,
	"ProjectKC.GAS.Action.ExecuteStartHook",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKCActionExecuteStartHookTest::RunTest(const FString& Parameters)
{
	const FName TestWorldName = MakeUniqueObjectName(
		nullptr,
		UWorld::StaticClass(),
		TEXT("KCActionHookTestWorld"),
		EUniqueObjectNameOptions::GloballyUnique);
	UWorld* TestWorld = UWorld::CreateWorld(
		EWorldType::Game,
		false,
		TestWorldName,
		GetTransientPackage());
	if (!TestNotNull(TEXT("Hook 검증용 World를 만든다."), TestWorld))
	{
		return false;
	}

	FWorldContext& WorldContext =
		GEngine->CreateNewWorldContext(EWorldType::Game);
	WorldContext.SetCurrentWorld(TestWorld);

	AKCPlayerCharacter* Holder = TestWorld->SpawnActor<AKCPlayerCharacter>(
		AKCPlayerCharacter::StaticClass(),
		FVector::ZeroVector,
		FRotator::ZeroRotator);
	if (TestNotNull(TEXT("휘두를 플레이어를 스폰한다."), Holder))
	{
		Holder->GetAbilitySystemComponent()->InitAbilityActorInfo(
			Holder,
			Holder);
		Holder->GetAbilitySystemComponent()->AddAttributeSetSubobject(
			Holder->GetCharacterAttributes());

		AKCWorldItemActor* Item = TestWorld->SpawnActor<AKCWorldItemActor>();
		UKCHeldItemComponent* HeldItemComponent = Holder->GetHeldItemComponent();
		if (TestNotNull(TEXT("휘두를 아이템을 스폰한다."), Item) &&
			TestTrue(
				TEXT("플레이어의 손 소켓을 설정한다."),
				KCActionHookTests::ConfigureHolderHand(
					Holder,
					HeldItemComponent)))
		{
			TestTrue(
				TEXT("두 Hook을 가진 Definition을 초기화한다."),
				Item->InitializeItem(
					KCActionHookTests::MakeSwingDefinition(Item)));
			TestTrue(
				TEXT("플레이어가 아이템을 든다."),
				HeldItemComponent->TryPickUp(Item));

			const float HealthBefore =
				Holder->GetCharacterAttributes()->GetHealth();
			TestTrue(
				TEXT("아무도 없는 곳을 향해 휘두른다."),
				HeldItemComponent->PressHeldItemUse());

			// Sweep이 아무도 잡지 못했으므로 OnExecute는 한 번도 돌지 않는다.
			// OnExecuteStart만 적용된 -5가 정확히 반영돼야 한다.
			TestEqual(
				TEXT("명중이 없어도 OnExecuteStart Hook은 실행된다."),
				Holder->GetCharacterAttributes()->GetHealth(),
				HealthBefore - 5.0f);
		}
	}

	TestWorld->DestroyWorld(false);
	GEngine->DestroyWorldContext(TestWorld);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FKCActionConfirmedHitHookTest,
	"ProjectKC.GAS.Action.ConfirmedHitHook",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKCActionConfirmedHitHookTest::RunTest(const FString& Parameters)
{
	const FName TestWorldName = MakeUniqueObjectName(
		nullptr,
		UWorld::StaticClass(),
		TEXT("KCConfirmedHitHookTestWorld"),
		EUniqueObjectNameOptions::GloballyUnique);
	UWorld* TestWorld = UWorld::CreateWorld(
		EWorldType::Game,
		false,
		TestWorldName,
		GetTransientPackage());
	if (!TestNotNull(TEXT("명중 Hook 검증용 World를 만든다."), TestWorld))
	{
		return false;
	}

	FWorldContext& WorldContext =
		GEngine->CreateNewWorldContext(EWorldType::Game);
	WorldContext.SetCurrentWorld(TestWorld);

	AKCPlayerCharacter* Holder = TestWorld->SpawnActor<AKCPlayerCharacter>(
		AKCPlayerCharacter::StaticClass(),
		FVector::ZeroVector,
		FRotator::ZeroRotator);
	AKCPlayerCharacter* Target = TestWorld->SpawnActor<AKCPlayerCharacter>(
		AKCPlayerCharacter::StaticClass(),
		FVector(120.0f, 0.0f, 0.0f),
		FRotator::ZeroRotator);
	if (TestNotNull(TEXT("휘두를 플레이어를 스폰한다."), Holder) &&
		TestNotNull(TEXT("맞을 플레이어를 스폰한다."), Target))
	{
		Holder->GetAbilitySystemComponent()->InitAbilityActorInfo(
			Holder,
			Holder);
		Holder->GetAbilitySystemComponent()->AddAttributeSetSubobject(
			Holder->GetCharacterAttributes());

		AKCWorldItemActor* Item = TestWorld->SpawnActor<AKCWorldItemActor>();
		UKCHeldItemComponent* HeldItemComponent = Holder->GetHeldItemComponent();
		if (TestNotNull(TEXT("휘두를 아이템을 스폰한다."), Item) &&
			TestTrue(
				TEXT("플레이어의 손 소켓을 설정한다."),
				KCActionHookTests::ConfigureHolderHand(
					Holder,
					HeldItemComponent)))
		{
			TestTrue(
				TEXT("명중 Hook을 가진 Definition을 초기화한다."),
				Item->InitializeItem(
					KCActionHookTests::MakeSwingDefinition(Item)));
			TestTrue(
				TEXT("플레이어가 아이템을 든다."),
				HeldItemComponent->TryPickUp(Item));

			const float HealthBefore =
				Holder->GetCharacterAttributes()->GetHealth();
			TestTrue(
				TEXT("대상이 있는 곳을 향해 휘두른다."),
				HeldItemComponent->PressHeldItemUse());

			// OnExecuteStart -5, 대상 OnExecute -50, 최초 확정 명중 -7이다.
			TestEqual(
				TEXT("명중한 실행 구간에는 OnConfirmedHit이 정확히 한 번 실행된다."),
				Holder->GetCharacterAttributes()->GetHealth(),
				HealthBefore - 62.0f);
		}
	}

	TestWorld->DestroyWorld(false);
	GEngine->DestroyWorldContext(TestWorld);
	return true;
}

#endif
