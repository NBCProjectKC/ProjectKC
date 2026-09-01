#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameplayEffect.h"
#include "ProjectKC/AbilitySystem/Attribute/KCCharacterAttributeSet.h"
#include "ProjectKC/AbilitySystem/Definition/KCSingleActionDefinition.h"
#include "ProjectKC/AbilitySystem/Effect/KCGE_Damage.h"
#include "ProjectKC/AbilitySystem/Fragment/KCThrowProjectileFragment.h"
#include "ProjectKC/AbilitySystem/Projectile/KCActionProjectile.h"
#include "ProjectKC/AbilitySystem/Struct/KCSetByCallerValueStruct.h"
#include "ProjectKC/AbilitySystem/Tag/KCAbilityGameplayTags.h"
#include "ProjectKC/AbilitySystem/Targeting/KCSelfTargeting.h"
#include "ProjectKC/Item/Component/KCHeldItemComponent.h"
#include "ProjectKC/Item/Definition/KCItemDefinition.h"
#include "ProjectKC/Item/KCWorldItemActor.h"
#include "ProjectKC/Player/KCPlayerCharacter.h"

namespace KCProjectileTests
{
	void ConfigureValidFragment(
		UKCThrowProjectileFragment* Fragment,
		UObject* AssetOuter)
	{
		Fragment->LaunchConfig.ProjectileMesh =
			NewObject<UStaticMesh>(AssetOuter);
		Fragment->ExplosionConfig.EffectRecipe.EffectClass =
			UGameplayEffect::StaticClass();
		Fragment->ExplosionConfig.Knockback.bEnabled = false;
	}

	UKCItemDefinition* MakeSeaUrchinDefinition(UObject* Outer)
	{
		UKCItemDefinition* Definition = NewObject<UKCItemDefinition>(Outer);
		Definition->ItemId = FGameplayTag::RequestGameplayTag(
			TEXT("Item.Id.SeaUrchin"));
		Definition->DisplayName = FText::FromString(TEXT("Sea Urchin"));
		Definition->Presentation.StaticMesh = NewObject<UStaticMesh>(Definition);
		Definition->Presentation.bSimulatePhysicsInWorld = false;
		Definition->UseLifecycle =
			EKCItemUseLifecycle::ConsumeOnSuccessfulExecute;

		UKCSingleActionDefinition* Action =
			NewObject<UKCSingleActionDefinition>(Definition);
		Action->ActionTargeting = NewObject<UKCSelfTargeting>(Action);
		FKCActionHookStruct ExecuteHook;
		ExecuteHook.HookTag = TAG_KC_ActionHook_OnExecute;
		UKCThrowProjectileFragment* ThrowFragment =
			NewObject<UKCThrowProjectileFragment>(Action);
		ConfigureValidFragment(ThrowFragment, Definition);
		ExecuteHook.Fragments.Add(ThrowFragment);
		Action->ActionHooks.Add(MoveTemp(ExecuteHook));
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
	FKCProjectileDefinitionValidationTest,
	"ProjectKC.Projectile.Definition.Validation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKCProjectileDefinitionValidationTest::RunTest(const FString& Parameters)
{
	UKCThrowProjectileFragment* Fragment =
		NewObject<UKCThrowProjectileFragment>();
	FString Error;
	TestFalse(
		TEXT("외형과 폭발 GE가 없는 Throw Projectile 설정은 거부한다."),
		Fragment->Validate(Error));

	KCProjectileTests::ConfigureValidFragment(Fragment, Fragment);
	TestTrue(
		TEXT("투사체 클래스·메시·폭발 GE를 갖춘 설정은 유효하다."),
		Fragment->Validate(Error));
	TestTrue(
		TEXT("Throw Projectile은 성공 여부가 소비를 확정하도록 기본 필수 Fragment다."),
		Fragment->bRequired);
	TestEqual(
		TEXT("Throw Projectile은 소스에서 실행되는 Fragment다."),
		Fragment->ApplicationScope,
		EKCActionScope::Source);

	Fragment->ExplosionConfig.FuseDuration =
		Fragment->ExplosionConfig.MaximumLifetime;
	TestFalse(
		TEXT("안전 수명보다 짧지 않은 퓨즈는 거부한다."),
		Fragment->Validate(Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FKCProjectileRuntimeTest,
	"ProjectKC.Projectile.Runtime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKCProjectileRuntimeTest::RunTest(const FString& Parameters)
{
	const FName TestWorldName = MakeUniqueObjectName(
		nullptr,
		UWorld::StaticClass(),
		TEXT("KCProjectileTestWorld"),
		EUniqueObjectNameOptions::GloballyUnique);
	UWorld* TestWorld = UWorld::CreateWorld(
		EWorldType::Game,
		false,
		TestWorldName,
		GetTransientPackage());
	if (!TestNotNull(TEXT("투사체 검증용 World를 만든다."), TestWorld))
	{
		return false;
	}

	FWorldContext& WorldContext =
		GEngine->CreateNewWorldContext(EWorldType::Game);
	WorldContext.SetCurrentWorld(TestWorld);

	AKCPlayerCharacter* SourcePlayer = TestWorld->SpawnActor<AKCPlayerCharacter>(
		AKCPlayerCharacter::StaticClass(),
		FVector(-150.0f, 0.0f, 100.0f),
		FRotator::ZeroRotator);
	AKCPlayerCharacter* OtherPlayer = TestWorld->SpawnActor<AKCPlayerCharacter>(
		AKCPlayerCharacter::StaticClass(),
		FVector(150.0f, 0.0f, 100.0f),
		FRotator::ZeroRotator);
	if (TestNotNull(TEXT("투척자를 스폰한다."), SourcePlayer) &&
		TestNotNull(TEXT("다른 플레이어를 스폰한다."), OtherPlayer))
	{
		SourcePlayer->GetAbilitySystemComponent()->InitAbilityActorInfo(
			SourcePlayer,
			SourcePlayer);
		SourcePlayer->GetAbilitySystemComponent()->AddAttributeSetSubobject(
			SourcePlayer->GetCharacterAttributes());
		OtherPlayer->GetAbilitySystemComponent()->InitAbilityActorInfo(
			OtherPlayer,
			OtherPlayer);
		OtherPlayer->GetAbilitySystemComponent()->AddAttributeSetSubobject(
			OtherPlayer->GetCharacterAttributes());

		FKCProjectileLaunchConfigStruct LaunchConfig;
		LaunchConfig.ProjectileClass = AKCActionProjectile::StaticClass();
		LaunchConfig.ProjectileMesh = NewObject<UStaticMesh>(TestWorld);

		FKCProjectileExplosionConfigStruct ExplosionConfig;
		ExplosionConfig.EffectRecipe.EffectClass = UKCGE_Damage::StaticClass();
		FKCSetByCallerValueStruct DamageValue;
		DamageValue.DataTag = TAG_KC_Data_Damage_Flat;
		DamageValue.Magnitude = -20.0f;
		ExplosionConfig.EffectRecipe.SetByCallers.Add(DamageValue);
		ExplosionConfig.ExplosionRadius = 500.0f;
		ExplosionConfig.Knockback.bEnabled = false;

		AKCActionProjectile* Projectile =
			TestWorld->SpawnActor<AKCActionProjectile>(
				AKCActionProjectile::StaticClass(),
				FVector(0.0f, 0.0f, 100.0f),
				FRotator::ZeroRotator);
		if (TestNotNull(TEXT("공용 투사체를 스폰한다."), Projectile))
		{
			Projectile->SetOwner(SourcePlayer);
			Projectile->SetInstigator(SourcePlayer);
			TestTrue(
				TEXT("서버에서 유효한 설정으로 투사체를 초기화한다."),
				Projectile->InitializeProjectile(
					LaunchConfig,
					ExplosionConfig,
					SourcePlayer->GetAbilitySystemComponent(),
					SourcePlayer,
					SourcePlayer,
					SourcePlayer,
					FVector(1000.0f, 0.0f, 300.0f)));
			TestTrue(
				TEXT("투사체 위치는 네트워크로 복제한다."),
				Projectile->IsReplicatingMovement());
			TestEqual(
				TEXT("투사체는 다른 Pawn을 Blocking 대상으로 유지한다."),
				Projectile->GetCollisionComponent()->
					GetCollisionResponseToChannel(ECC_Pawn),
				ECR_Block);
			TestTrue(
				TEXT("투사체 이동은 투척자 Actor만 무시한다."),
				Projectile->GetCollisionComponent()->GetMoveIgnoreActors().Contains(
					SourcePlayer));
			TestFalse(
				TEXT("다른 플레이어는 투사체 이동 무시 목록에 넣지 않는다."),
				Projectile->GetCollisionComponent()->GetMoveIgnoreActors().Contains(
					OtherPlayer));

			UPrimitiveComponent* SourceRoot =
				Cast<UPrimitiveComponent>(SourcePlayer->GetRootComponent());
			if (TestNotNull(
				TEXT("투척자의 이동 Root Primitive를 찾는다."),
				SourceRoot))
			{
				TestTrue(
					TEXT("투척자 이동도 방금 던진 투사체를 무시한다."),
					SourceRoot->GetMoveIgnoreActors().Contains(Projectile));
			}

			const float SourceHealthBefore =
				SourcePlayer->GetCharacterAttributes()->GetHealth();
			const float OtherHealthBefore =
				OtherPlayer->GetCharacterAttributes()->GetHealth();
			TestTrue(TEXT("서버가 투사체를 폭발시킨다."), Projectile->Detonate());
			TestEqual(
				TEXT("투척자는 기본 폭발 대상에서 제외한다."),
				SourcePlayer->GetCharacterAttributes()->GetHealth(),
				SourceHealthBefore);
			TestEqual(
				TEXT("피아 구분 없이 반경 안의 다른 플레이어는 폭발 GE를 받는다."),
				OtherPlayer->GetCharacterAttributes()->GetHealth(),
				OtherHealthBefore - 20.0f);
			TestFalse(
				TEXT("폭발 결과 처리 뒤 투사체 Actor를 제거한다."),
				IsValid(Projectile));
		}

		AKCWorldItemActor* SeaUrchin =
			TestWorld->SpawnActor<AKCWorldItemActor>();
		UKCHeldItemComponent* HeldItemComponent =
			SourcePlayer->GetHeldItemComponent();
		if (TestNotNull(TEXT("성게 원본 아이템을 스폰한다."), SeaUrchin) &&
			TestTrue(
				TEXT("테스트 플레이어의 손 소켓을 설정한다."),
				KCProjectileTests::ConfigureHolderHand(
					SourcePlayer,
					HeldItemComponent)))
		{
			UKCItemDefinition* Definition =
				KCProjectileTests::MakeSeaUrchinDefinition(SeaUrchin);
			TestTrue(
				TEXT("Throw Projectile을 가진 일회용 성게 Definition을 초기화한다."),
				SeaUrchin->InitializeItem(Definition));
			TestTrue(
				TEXT("플레이어가 성게를 든다."),
				HeldItemComponent->TryPickUp(SeaUrchin));

			int32 ProjectileCountBefore = 0;
			for (TActorIterator<AKCActionProjectile> It(TestWorld); It; ++It)
			{
				++ProjectileCountBefore;
			}

			TestTrue(
				TEXT("성게 사용은 Throw Projectile Fragment를 성공시킨다."),
				HeldItemComponent->PressHeldItemUse());
			TestTrue(
				TEXT("투사체 생성 성공은 성게 원본 소비를 예약한다."),
				SeaUrchin->IsUseConsumptionPending());

			AKCActionProjectile* SpawnedProjectile = nullptr;
			int32 ProjectileCountAfter = 0;
			for (TActorIterator<AKCActionProjectile> It(TestWorld); It; ++It)
			{
				SpawnedProjectile = *It;
				++ProjectileCountAfter;
			}
			TestEqual(
				TEXT("성게 한 번 사용은 투사체 하나를 생성한다."),
				ProjectileCountAfter,
				ProjectileCountBefore + 1);
			TestTrue(
				TEXT("생성된 성게 투사체는 투척자를 무시한다."),
				SpawnedProjectile &&
					SpawnedProjectile->GetIgnoredSourceActor() == SourcePlayer);

			TestWorld->Tick(LEVELTICK_All, 1.0f / 60.0f);
			TestFalse(
				TEXT("성공 사용 뒤 원본 성게 아이템은 다음 틱에 제거된다."),
				IsValid(SeaUrchin));
			TestTrue(
				TEXT("원본 성게가 소비되어도 생성된 투사체는 유지된다."),
				IsValid(SpawnedProjectile));
		}
	}

	TestWorld->DestroyWorld(false);
	GEngine->DestroyWorldContext(TestWorld);
	return true;
}

#endif
