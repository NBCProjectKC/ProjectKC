#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Animation/AnimMontage.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"
#include "GameplayEffect.h"
#include "ProjectKC/AbilitySystem/Ability/KCGAInstantSelfAction.h"
#include "ProjectKC/AbilitySystem/Definition/KCAbilityDefinition.h"
#include "ProjectKC/AbilitySystem/Fragment/KCApplyGameplayEffectFragment.h"
#include "ProjectKC/AbilitySystem/Tag/KCAbilityGameplayTags.h"
#include "ProjectKC/Item/Definition/KCItemDefinition.h"

namespace KCItemDefinitionTests
{
	UKCItemDefinition* MakeCarryOnlyItem()
	{
		UKCItemDefinition* Definition = NewObject<UKCItemDefinition>();
		Definition->DisplayName = FText::FromString(TEXT("Carry Item"));
		Definition->Presentation.StaticMesh =
			NewObject<UStaticMesh>(Definition);
		UStaticMeshSocket* GripSocket =
			NewObject<UStaticMeshSocket>(Definition->Presentation.StaticMesh);
		GripSocket->SocketName = Definition->Presentation.GripSocketName;
		Definition->Presentation.StaticMesh->Sockets.Add(GripSocket);
		return Definition;
	}

	UKCAbilityDefinition* MakeValidUseAbility(UObject* Outer)
	{
		UKCAbilityDefinition* Definition =
			NewObject<UKCAbilityDefinition>(Outer);
		Definition->ActionClass = UKCGAInstantSelfAction::StaticClass();
		Definition->ActionMontage.Montage = NewObject<UAnimMontage>(Definition);

		FKCActionHookStruct Hook;
		Hook.HookTag = TAG_KC_ActionHook_Self_OnActivate;
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

	UKCItemDefinition* InvalidUseItem =
		KCItemDefinitionTests::MakeCarryOnlyItem();
	InvalidUseItem->UseAction =
		NewObject<UKCAbilityDefinition>(InvalidUseItem);
	TestFalse(
		TEXT("잘못된 사용 Ability를 가진 아이템 Definition은 거부한다."),
		InvalidUseItem->Validate(Error));

	UKCItemDefinition* MissingMontageItem =
		KCItemDefinitionTests::MakeCarryOnlyItem();
	MissingMontageItem->UseAction =
		KCItemDefinitionTests::MakeValidUseAbility(MissingMontageItem);
	MissingMontageItem->UseAction->ActionMontage.Montage = nullptr;
	TestTrue(
		TEXT("Montage가 없어도 사용 Ability 자체의 계약은 여전히 유효하다."),
		MissingMontageItem->UseAction->ValidateWithActionContract(Error));
	TestFalse(
		TEXT("사용 가능한 아이템은 고유 사용 Montage가 없으면 거부한다."),
		MissingMontageItem->Validate(Error));

	UKCItemDefinition* MissingMeshItem = NewObject<UKCItemDefinition>();
	MissingMeshItem->DisplayName = FText::FromString(TEXT("No Mesh"));
	TestFalse(
		TEXT("표현 Mesh가 없는 아이템 Definition은 거부한다."),
		MissingMeshItem->Validate(Error));

	return true;
}

#endif
