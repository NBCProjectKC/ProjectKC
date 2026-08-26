#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AbilitySystemInterface.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "ProjectKC/AbilitySystem/Component/KCAbilitySystemComponent.h"
#include "ProjectKC/AbilitySystem/Component/KCKnockbackComponent.h"
#include "ProjectKC/Interaction/Interface/KCInteractableInterface.h"
#include "ProjectKC/Item/Component/KCHeldItemComponent.h"
#include "ProjectKC/Item/KCWorldItemActor.h"
#include "ProjectKC/Player/Interaction/KCPlayerInteractionComponent.h"
#include "ProjectKC/Player/KCPlayerCharacter.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FKCPlayerItemIntegrationTest,
	"ProjectKC.Item.PlayerIntegration.ComponentsAndContracts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKCPlayerItemIntegrationTest::RunTest(const FString& Parameters)
{
	TestTrue(
		TEXT("최종 플레이어가 ASC 제공 계약을 구현한다."),
		AKCPlayerCharacter::StaticClass()->ImplementsInterface(
			UAbilitySystemInterface::StaticClass()));

	const AKCPlayerCharacter* Character = GetDefault<AKCPlayerCharacter>();
	TestNotNull(
		TEXT("최종 플레이어가 KC ASC를 기본 구성요소로 소유한다."),
		Character->GetKCAbilitySystemComponent());
	TestNotNull(
		TEXT("최종 플레이어가 단일 HeldItem 컴포넌트를 소유한다."),
		Character->GetHeldItemComponent());
	TestNotNull(
		TEXT("최종 플레이어가 넉백 수신 컴포넌트를 소유한다."),
		Character->GetKnockbackComponent());
	TestNotNull(
		TEXT("최종 플레이어가 공용 상호작용 컴포넌트를 소유한다."),
		Character->GetInteractionComponent());

	TestTrue(
		TEXT("월드 아이템이 공용 Interactable 계약을 구현한다."),
		AKCWorldItemActor::StaticClass()->ImplementsInterface(
			UKCInteractableInterface::StaticClass()));

	const AKCWorldItemActor* WorldItem = GetDefault<AKCWorldItemActor>();
	const UStaticMeshComponent* ItemMesh = WorldItem->GetItemMesh();
	TestNotNull(
		TEXT("월드 아이템에 상호작용 대상 Mesh가 존재한다."),
		ItemMesh);
	if (ItemMesh)
	{
		TestTrue(
			TEXT("월드 아이템 Mesh에 Interactable 태그가 있다."),
			ItemMesh->ComponentHasTag(TEXT("Interactable")));
		TestTrue(
			TEXT("월드 아이템 Mesh가 상호작용 Overlap을 생성한다."),
			ItemMesh->GetGenerateOverlapEvents());
	}

	UClass* FinalPlayerBlueprintClass = LoadClass<AKCPlayerCharacter>(
		nullptr,
		TEXT("/Game/KC/Player/Blueprints/BP_KCPlayerCharacter."
			"BP_KCPlayerCharacter_C"));
	TestNotNull(
		TEXT("최종 Player Blueprint를 로드할 수 있다."),
		FinalPlayerBlueprintClass);
	if (FinalPlayerBlueprintClass)
	{
		AKCPlayerCharacter* FinalPlayer =
			FinalPlayerBlueprintClass->GetDefaultObject<AKCPlayerCharacter>();
		const UKCHeldItemComponent* FinalHeldItem =
			FinalPlayer->GetHeldItemComponent();
		const UStaticMeshComponent* AvatarHandRight =
			Cast<UStaticMeshComponent>(
				FinalPlayer->GetDefaultSubobjectByName(TEXT("AvatarHandRight")));
		USceneComponent* FinalAttachment =
			FinalHeldItem ? FinalHeldItem->GetAttachmentComponent() : nullptr;
		TestNotNull(
			TEXT("최종 Player Blueprint가 실제 손 소켓 제공 컴포넌트를 찾는다."),
			FinalAttachment);
		if (FinalHeldItem && FinalAttachment)
		{
			TestTrue(
				TEXT("최종 Player Blueprint의 설정된 손 소켓이 실제로 존재한다."),
				FinalAttachment->DoesSocketExist(
					FinalHeldItem->GetHandSocketName()));
		}
		TestNotNull(
			TEXT("보이는 오른손 컴포넌트를 찾을 수 있다."),
			AvatarHandRight);
		if (FinalHeldItem && AvatarHandRight)
		{
			TestEqual(
				TEXT("보이는 오른손과 아이템은 같은 스켈레톤 소켓을 따른다."),
				AvatarHandRight->GetAttachSocketName(),
				FinalHeldItem->GetHandSocketName());
		}
	}

	return true;
}

#endif
