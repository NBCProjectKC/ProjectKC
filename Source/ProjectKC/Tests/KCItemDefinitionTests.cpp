#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Animation/AnimMontage.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"
#include "GameplayEffect.h"
#include "ProjectKC/AbilitySystem/Definition/KCChannelActionDefinition.h"
#include "ProjectKC/AbilitySystem/Definition/KCSingleActionDefinition.h"
#include "ProjectKC/AbilitySystem/Fragment/KCApplyGameplayEffectFragment.h"
#include "ProjectKC/AbilitySystem/Tag/KCAbilityGameplayTags.h"
#include "ProjectKC/AbilitySystem/Targeting/KCItemSocketTrailTargeting.h"
#include "ProjectKC/AbilitySystem/Targeting/KCSelfTargeting.h"
#include "ProjectKC/AbilitySystem/Targeting/KCSweepTargeting.h"
#include "ProjectKC/Item/Definition/KCItemDefinition.h"

namespace KCItemDefinitionTests
{
	UKCItemDefinition* MakeCarryOnlyItem()
	{
		UKCItemDefinition* Definition = NewObject<UKCItemDefinition>();
		Definition->ItemId = FGameplayTag::RequestGameplayTag(
			TEXT("Item.Id.FryingPan"));
		Definition->DisplayName = FText::FromString(TEXT("Carry Item"));
		Definition->Presentation.StaticMesh =
			NewObject<UStaticMesh>(Definition);
		UStaticMeshSocket* GripSocket =
			NewObject<UStaticMeshSocket>(Definition->Presentation.StaticMesh);
		GripSocket->SocketName = Definition->Presentation.GripSocketName;
		Definition->Presentation.StaticMesh->Sockets.Add(GripSocket);
		return Definition;
	}

	UKCSingleActionDefinition* MakeValidUseAbility(UObject* Outer)
	{
		UKCSingleActionDefinition* Definition =
			NewObject<UKCSingleActionDefinition>(Outer);
		Definition->ActionTargeting = NewObject<UKCSelfTargeting>(Definition);
		Definition->ActionMontage.Montage = NewObject<UAnimMontage>(Definition);

		FKCActionHookStruct Hook;
		Hook.HookTag = TAG_KC_ActionHook_OnExecute;
		UKCApplyGameplayEffectFragment* Fragment =
			NewObject<UKCApplyGameplayEffectFragment>(Definition);
		Fragment->EffectRecipe.EffectClass = UGameplayEffect::StaticClass();
		Hook.Fragments.Add(Fragment);
		Definition->ActionHooks.Add(MoveTemp(Hook));
		return Definition;
	}

	UKCChannelActionDefinition* MakeValidChannelUseAbility(UObject* Outer)
	{
		UKCChannelActionDefinition* Definition =
			NewObject<UKCChannelActionDefinition>(Outer);
		Definition->ActionTargeting = NewObject<UKCSelfTargeting>(Definition);
		Definition->ActionMontage.Montage = NewObject<UAnimMontage>(Definition);

		FKCActionHookStruct Hook;
		Hook.HookTag = TAG_KC_ActionHook_OnExecute;
		UKCApplyGameplayEffectFragment* Fragment =
			NewObject<UKCApplyGameplayEffectFragment>(Definition);
		Fragment->EffectRecipe.EffectClass = UGameplayEffect::StaticClass();
		Hook.Fragments.Add(Fragment);
		Definition->ActionHooks.Add(MoveTemp(Hook));
		return Definition;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FKCItemDefinitionValidationTest,
	"ProjectKC.Item.Definition.Validation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKCItemDefinitionValidationTest::RunTest(const FString& Parameters)
{
	FString Error;
	UKCItemDefinition* MissingIdItem =
		KCItemDefinitionTests::MakeCarryOnlyItem();
	MissingIdItem->ItemId = FGameplayTag();
	TestFalse(
		TEXT("ItemId가 없는 아이템 Definition은 거부한다."),
		MissingIdItem->Validate(Error));

	UKCItemDefinition* CarryOnlyItem =
		KCItemDefinitionTests::MakeCarryOnlyItem();
	TestTrue(
		TEXT("Ability가 없는 운반 전용 아이템도 유효하다."),
		CarryOnlyItem->Validate(Error));
	TestFalse(
		TEXT("Ability가 없는 아이템은 사용할 수 없다."),
		CarryOnlyItem->IsUsable());

	UStaticMeshSocket* GripSocket =
		CarryOnlyItem->Presentation.StaticMesh->FindSocket(
			CarryOnlyItem->Presentation.GripSocketName);
	GripSocket->RelativeLocation = FVector(12.0f, -4.0f, 7.0f);
	GripSocket->RelativeRotation = FRotator(0.0f, 90.0f, 15.0f);
	FTransform AlignmentTransform;
	TestTrue(
		TEXT("유효한 Grip 소켓에서 장착 정렬 Transform을 만들 수 있다."),
		CarryOnlyItem->Presentation.TryGetGripAlignmentTransform(
			AlignmentTransform));
	const FTransform GripTransform(
		GripSocket->RelativeRotation,
		GripSocket->RelativeLocation,
		FVector::OneVector);
	TestTrue(
		TEXT("Grip과 장착 정렬 Transform을 합성하면 Hand 기준 원점이 된다."),
		(GripTransform * AlignmentTransform).Equals(
			FTransform::Identity,
			KINDA_SMALL_NUMBER));

	UKCItemDefinition* UsableItem =
		KCItemDefinitionTests::MakeCarryOnlyItem();
	UsableItem->UseAction =
		KCItemDefinitionTests::MakeValidUseAbility(UsableItem);
	TestTrue(
		TEXT("유효한 Ability를 조립한 아이템은 사용할 수 있다."),
		UsableItem->IsUsable());
	TestTrue(
		TEXT("유효한 Ability를 조립한 아이템 Definition을 허용한다."),
		UsableItem->Validate(Error));
	TestTrue(
		TEXT("사용 Action은 별도 Data Asset이 아니라 Item Definition에 포함된다."),
		UsableItem->UseAction->GetOuter() == UsableItem);
	TestTrue(
		TEXT("기존 아이템의 기본 사용 수명주기는 Persistent다."),
		UsableItem->UseLifecycle == EKCItemUseLifecycle::Persistent);

	UKCItemDefinition* ConsumableWithoutUse =
		KCItemDefinitionTests::MakeCarryOnlyItem();
	ConsumableWithoutUse->UseLifecycle =
		EKCItemUseLifecycle::ConsumeOnSuccessfulExecute;
	TestFalse(
		TEXT("성공 후 소비 아이템에는 UseAction이 필요하다."),
		ConsumableWithoutUse->Validate(Error));

	UKCItemDefinition* ConsumableWithoutRequiredFragment =
		KCItemDefinitionTests::MakeCarryOnlyItem();
	ConsumableWithoutRequiredFragment->UseAction =
		KCItemDefinitionTests::MakeValidUseAbility(
			ConsumableWithoutRequiredFragment);
	ConsumableWithoutRequiredFragment->UseLifecycle =
		EKCItemUseLifecycle::ConsumeOnSuccessfulExecute;
	TestFalse(
		TEXT("성공 후 소비에는 성공을 확정할 필수 Fragment가 필요하다."),
		ConsumableWithoutRequiredFragment->Validate(Error));

	UKCItemDefinition* ConsumableItem =
		KCItemDefinitionTests::MakeCarryOnlyItem();
	ConsumableItem->UseAction =
		KCItemDefinitionTests::MakeValidUseAbility(ConsumableItem);
	ConsumableItem->UseAction->ActionHooks[0].Fragments[0]->bRequired = true;
	ConsumableItem->UseLifecycle =
		EKCItemUseLifecycle::ConsumeOnSuccessfulExecute;
	TestTrue(
		TEXT("필수 실행 결과가 있는 Single Action은 성공 후 소비할 수 있다."),
		ConsumableItem->Validate(Error));

	UKCItemDefinition* ConsumableChannelItem =
		KCItemDefinitionTests::MakeCarryOnlyItem();
	ConsumableChannelItem->UseAction =
		KCItemDefinitionTests::MakeValidChannelUseAbility(
			ConsumableChannelItem);
	ConsumableChannelItem->UseAction->ActionHooks[0].Fragments[0]->bRequired = true;
	ConsumableChannelItem->UseLifecycle =
		EKCItemUseLifecycle::ConsumeOnSuccessfulExecute;
	TestFalse(
		TEXT("성공 후 소비는 반복 실행되는 Channel Action에 설정할 수 없다."),
		ConsumableChannelItem->Validate(Error));

	UKCItemDefinition* ConsumableDurabilityItem =
		KCItemDefinitionTests::MakeCarryOnlyItem();
	ConsumableDurabilityItem->UseAction =
		KCItemDefinitionTests::MakeValidUseAbility(
			ConsumableDurabilityItem);
	ConsumableDurabilityItem->UseAction->ActionHooks[0].Fragments[0]->bRequired = true;
	ConsumableDurabilityItem->UseLifecycle =
		EKCItemUseLifecycle::ConsumeOnSuccessfulExecute;
	ConsumableDurabilityItem->Durability.ConsumeMode =
		EKCItemDurabilityConsumeMode::OnUse;
	ConsumableDurabilityItem->Durability.ConsumeAmount = 100.0f;
	TestFalse(
		TEXT("일회용 소비와 내구도 소모를 동시에 설정할 수 없다."),
		ConsumableDurabilityItem->Validate(Error));

	UKCItemDefinition* DurabilityWithoutUse =
		KCItemDefinitionTests::MakeCarryOnlyItem();
	DurabilityWithoutUse->Durability.ConsumeMode =
		EKCItemDurabilityConsumeMode::OnUse;
	DurabilityWithoutUse->Durability.ConsumeAmount = 25.0f;
	TestFalse(
		TEXT("UseAction이 없는 아이템에는 내구도 소모 규칙을 설정할 수 없다."),
		DurabilityWithoutUse->Validate(Error));

	UKCItemDefinition* InvalidDurabilityAmount =
		KCItemDefinitionTests::MakeCarryOnlyItem();
	InvalidDurabilityAmount->UseAction =
		KCItemDefinitionTests::MakeValidUseAbility(InvalidDurabilityAmount);
	InvalidDurabilityAmount->Durability.ConsumeMode =
		EKCItemDurabilityConsumeMode::OnUse;
	InvalidDurabilityAmount->Durability.ConsumeAmount = 0.0f;
	TestFalse(
		TEXT("활성 내구도 규칙의 소모량은 0보다 커야 한다."),
		InvalidDurabilityAmount->Validate(Error));

	UKCItemDefinition* OnUseItem =
		KCItemDefinitionTests::MakeCarryOnlyItem();
	OnUseItem->UseAction =
		KCItemDefinitionTests::MakeValidUseAbility(OnUseItem);
	OnUseItem->Durability.ConsumeMode =
		EKCItemDurabilityConsumeMode::OnUse;
	OnUseItem->Durability.ConsumeAmount = 25.0f;
	TestTrue(
		TEXT("일반 Action은 사용 확정 시 내구도를 소모할 수 있다."),
		OnUseItem->Validate(Error));

	UKCItemDefinition* NonTraceHitItem =
		KCItemDefinitionTests::MakeCarryOnlyItem();
	NonTraceHitItem->UseAction =
		KCItemDefinitionTests::MakeValidUseAbility(NonTraceHitItem);
	NonTraceHitItem->Durability.ConsumeMode =
		EKCItemDurabilityConsumeMode::OnFirstHit;
	NonTraceHitItem->Durability.ConsumeAmount = 25.0f;
	TestFalse(
		TEXT("명중 소모 규칙은 HitResult를 제공하는 Targeting이 필요하다."),
		NonTraceHitItem->Validate(Error));

	UKCItemDefinition* SweepHitItem =
		KCItemDefinitionTests::MakeCarryOnlyItem();
	SweepHitItem->UseAction =
		KCItemDefinitionTests::MakeValidUseAbility(SweepHitItem);
	SweepHitItem->UseAction->ActionTargeting =
		NewObject<UKCSweepTargeting>(SweepHitItem->UseAction);
	SweepHitItem->Durability.ConsumeMode =
		EKCItemDurabilityConsumeMode::OnFirstHit;
	SweepHitItem->Durability.ConsumeAmount = 25.0f;
	TestTrue(
		TEXT("Sweep Action은 최초 유효 명중 시 내구도를 소모할 수 있다."),
		SweepHitItem->Validate(Error));

	UKCItemDefinition* TraceHitItem =
		KCItemDefinitionTests::MakeCarryOnlyItem();
	TraceHitItem->UseAction =
		KCItemDefinitionTests::MakeValidUseAbility(TraceHitItem);
	TraceHitItem->UseAction->ActionTargeting =
		NewObject<UKCItemSocketTrailTargeting>(TraceHitItem->UseAction);
	TraceHitItem->Durability.ConsumeMode =
		EKCItemDurabilityConsumeMode::OnFirstHit;
	TraceHitItem->Durability.ConsumeAmount = 25.0f;
	TestTrue(
		TEXT("TraceWindow Action은 최초 유효 명중 시 내구도를 소모할 수 있다."),
		TraceHitItem->Validate(Error));

	UKCItemDefinition* NonChannelContinuousItem =
		KCItemDefinitionTests::MakeCarryOnlyItem();
	NonChannelContinuousItem->UseAction =
		KCItemDefinitionTests::MakeValidUseAbility(NonChannelContinuousItem);
	NonChannelContinuousItem->Durability.ConsumeMode =
		EKCItemDurabilityConsumeMode::WhileActive;
	NonChannelContinuousItem->Durability.ConsumeAmount = 20.0f;
	TestFalse(
		TEXT("초당 내구도 소모 규칙은 Channel Action이 필요하다."),
		NonChannelContinuousItem->Validate(Error));

	UKCItemDefinition* ContinuousItem =
		KCItemDefinitionTests::MakeCarryOnlyItem();
	ContinuousItem->UseAction =
		KCItemDefinitionTests::MakeValidChannelUseAbility(ContinuousItem);
	ContinuousItem->Durability.ConsumeMode =
		EKCItemDurabilityConsumeMode::WhileActive;
	ContinuousItem->Durability.ConsumeAmount = 20.0f;
	TestTrue(
		TEXT("Channel Action은 활성 시간에 비례해 내구도를 소모할 수 있다."),
		ContinuousItem->Validate(Error));

	UKCItemDefinition* InvalidUseItem =
		KCItemDefinitionTests::MakeCarryOnlyItem();
	InvalidUseItem->UseAction =
		NewObject<UKCSingleActionDefinition>(InvalidUseItem);
	TestFalse(
		TEXT("잘못된 사용 Ability를 가진 아이템 Definition은 거부한다."),
		InvalidUseItem->Validate(Error));

	UKCItemDefinition* MissingMontageItem =
		KCItemDefinitionTests::MakeCarryOnlyItem();
	MissingMontageItem->UseAction =
		KCItemDefinitionTests::MakeValidUseAbility(MissingMontageItem);
	MissingMontageItem->UseAction->ActionMontage.Montage = nullptr;
	TestTrue(
		TEXT("Single Action은 Montage가 없어도 계약이 유효하다."),
		MissingMontageItem->UseAction->ValidateWithActionContract(Error));
	TestTrue(
		TEXT("Montage 없는 Single Action 아이템도 유효하며 즉시 실행된다."),
		MissingMontageItem->Validate(Error));

	UKCItemDefinition* MissingMeshItem = NewObject<UKCItemDefinition>();
	MissingMeshItem->ItemId = FGameplayTag::RequestGameplayTag(
		TEXT("Item.Id.FryingPan"));
	MissingMeshItem->DisplayName = FText::FromString(TEXT("No Mesh"));
	TestFalse(
		TEXT("표현 Mesh가 없는 아이템 Definition은 거부한다."),
		MissingMeshItem->Validate(Error));

	return true;
}

#endif
