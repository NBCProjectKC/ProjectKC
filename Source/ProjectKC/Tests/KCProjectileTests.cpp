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
#include "GameFramework/ProjectileMovementComponent.h"
#include "ProjectKC/AbilitySystem/Attribute/KCCharacterAttributeSet.h"
#include "ProjectKC/AbilitySystem/Definition/KCSingleActionDefinition.h"
#include "ProjectKC/AbilitySystem/Effect/KCGE_Damage.h"
#include "ProjectKC/AbilitySystem/Fragment/KCApplyGameplayEffectFragment.h"
#include "ProjectKC/AbilitySystem/Fragment/KCDropHeldItemFragment.h"
#include "ProjectKC/AbilitySystem/Fragment/KCKnockbackFragment.h"
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
		ThrowFragment->LaunchConfig.bEnableCharge = true;
		ThrowFragment->LaunchConfig.MinimumForwardSpeed = 400.0f;
		ThrowFragment->LaunchConfig.MaximumChargeDuration = 1.0f;
		ThrowFragment->LaunchConfig.bShowTrajectoryPreview = false;
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
		TEXT("투사체 외형이 없는 Throw Projectile 설정은 거부한다."),
		Fragment->Validate(Error));

	KCProjectileTests::ConfigureValidFragment(Fragment, Fragment);
	TestTrue(
		TEXT("투사체 클래스·메시와 폭발 범위를 갖춘 설정은 유효하다."),
		Fragment->Validate(Error));
	TestTrue(
		TEXT("Throw Projectile은 성공 여부가 소비를 확정하도록 기본 필수 Fragment다."),
		Fragment->bRequired);
	TestEqual(
		TEXT("Throw Projectile은 소스에서 실행되는 Fragment다."),
		Fragment->ApplicationScope,
		EKCActionScope::Source);

	Fragment->LaunchConfig.bEnableCharge = true;
	Fragment->LaunchConfig.MinimumForwardSpeed =
		Fragment->LaunchConfig.ForwardSpeed + 1.0f;
	TestFalse(
		TEXT("최대 속도보다 큰 충전 최소 속도는 거부한다."),
		Fragment->Validate(Error));
	Fragment->LaunchConfig.MinimumForwardSpeed = 400.0f;
	Fragment->LaunchConfig.MaximumChargeDuration = 2.0f;
	TestTrue(
		TEXT("유효한 충전 속도와 시간은 허용한다."),
		Fragment->Validate(Error));
	TestEqual(
		TEXT("최소 충전은 최소 전방 속도를 사용한다."),
		Fragment->LaunchConfig.ResolveForwardSpeed(0.0f),
		400.0f);
	TestEqual(
		TEXT("절반 충전은 전방 속도를 선형 보간한다."),
		Fragment->LaunchConfig.ResolveForwardSpeed(
			Fragment->LaunchConfig.CalculateChargeAlpha(1.0f)),
		800.0f);
	TestEqual(
		TEXT("최대 시간을 넘긴 충전은 최대 전방 속도로 고정한다."),
		Fragment->LaunchConfig.ResolveForwardSpeed(
			Fragment->LaunchConfig.CalculateChargeAlpha(10.0f)),
		Fragment->LaunchConfig.ForwardSpeed);

	UKCApplyGameplayEffectFragment* UnsupportedDeferredFragment =
		NewObject<UKCApplyGameplayEffectFragment>(Fragment);
	UnsupportedDeferredFragment->EffectRecipe.EffectClass =
		UGameplayEffect::StaticClass();
	UnsupportedDeferredFragment->bTrackUntilAbilityEnds = true;
	Fragment->ExplosionTargetFragments.Add(UnsupportedDeferredFragment);
	TestFalse(
		TEXT("GA 종료까지 추적하는 Effect Fragment는 폭발 지연 실행에서 거부한다."),
		Fragment->Validate(Error));
	UnsupportedDeferredFragment->bTrackUntilAbilityEnds = false;
	TestTrue(
		TEXT("일회성 Gameplay Effect Fragment는 폭발 지연 실행을 지원한다."),
		Fragment->Validate(Error));

	UKCDropHeldItemFragment* DropHeldItem =
		NewObject<UKCDropHeldItemFragment>(Fragment);
	DropHeldItem->ApplicationScope = EKCActionScope::Source;
	Fragment->ExplosionTargetFragments.Add(DropHeldItem);
	TestFalse(
		TEXT("폭발 대상 Fragment는 Source Scope를 사용할 수 없다."),
		Fragment->Validate(Error));
	DropHeldItem->ApplicationScope = EKCActionScope::Target;
	TestTrue(
		TEXT("지연 실행 가능한 Target Fragment는 Throw Projectile 아래 중첩할 수 있다."),
		Fragment->Validate(Error));

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

		AKCWorldItemActor* OtherHeldItem =
			TestWorld->SpawnActor<AKCWorldItemActor>();
		UKCHeldItemComponent* OtherHeldItemComponent =
			OtherPlayer->GetHeldItemComponent();
		if (TestNotNull(
				TEXT("폭발 Target Fragment가 드롭시킬 아이템을 스폰한다."),
				OtherHeldItem) &&
			TestTrue(
				TEXT("폭발 대상 플레이어의 손 소켓을 설정한다."),
				KCProjectileTests::ConfigureHolderHand(
					OtherPlayer,
					OtherHeldItemComponent)))
		{
			UKCItemDefinition* OtherHeldDefinition =
				KCProjectileTests::MakeSeaUrchinDefinition(OtherHeldItem);
			TestTrue(
				TEXT("폭발 대상이 들 아이템 Definition을 초기화한다."),
				OtherHeldItem->InitializeItem(OtherHeldDefinition));
			TestTrue(
				TEXT("폭발 대상 플레이어가 아이템을 든다."),
				OtherHeldItemComponent->TryPickUp(OtherHeldItem));
		}

		FKCProjectileLaunchConfigStruct LaunchConfig;
		LaunchConfig.ProjectileClass = AKCActionProjectile::StaticClass();
		LaunchConfig.ProjectileMesh = NewObject<UStaticMesh>(TestWorld);

		FKCProjectileExplosionConfigStruct ExplosionConfig;
		ExplosionConfig.ExplosionRadius = 500.0f;

		TArray<TObjectPtr<UKCActionFragment>> ExplosionTargetFragments;
		UKCApplyGameplayEffectFragment* DamageFragment =
			NewObject<UKCApplyGameplayEffectFragment>(TestWorld);
		DamageFragment->ApplicationScope = EKCActionScope::Target;
		DamageFragment->bRequired = true;
		DamageFragment->EffectRecipe.EffectClass = UKCGE_Damage::StaticClass();
		FKCSetByCallerValueStruct DamageValue;
		DamageValue.DataTag = TAG_KC_Data_Damage_Flat;
		DamageValue.Magnitude = -20.0f;
		DamageFragment->EffectRecipe.SetByCallers.Add(DamageValue);
		ExplosionTargetFragments.Add(DamageFragment);

		UKCKnockbackFragment* KnockbackFragment =
			NewObject<UKCKnockbackFragment>(TestWorld);
		KnockbackFragment->ApplicationScope = EKCActionScope::Target;
		KnockbackFragment->bRequired = true;
		KnockbackFragment->HorizontalSpeed = 120.0f;
		KnockbackFragment->VerticalSpeed = 80.0f;
		ExplosionTargetFragments.Add(KnockbackFragment);

		UKCDropHeldItemFragment* DropHeldItem =
			NewObject<UKCDropHeldItemFragment>(TestWorld);
		DropHeldItem->ApplicationScope = EKCActionScope::Target;
		DropHeldItem->bRequired = true;
		ExplosionTargetFragments.Add(DropHeldItem);

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
					ExplosionTargetFragments,
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
				TEXT("중첩 Damage Fragment가 설정한 피해만 적용한다."),
				OtherPlayer->GetCharacterAttributes()->GetHealth(),
				OtherHealthBefore - 20.0f);
			TestFalse(
				TEXT("폭발 대상에게 중첩한 Drop Held Item Fragment가 실행된다."),
				OtherHeldItemComponent->HasHeldItem());
			TestTrue(
				TEXT("드롭된 아이템 Actor는 소비되지 않고 World에 남는다."),
				IsValid(OtherHeldItem));
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
				TEXT("성게 Press는 충전 Action을 시작한다."),
				HeldItemComponent->PressHeldItemUse());
			TestFalse(
				TEXT("충전 중에는 투사체 생성 성공과 소비를 확정하지 않는다."),
				SeaUrchin->IsUseConsumptionPending());

			int32 ProjectileCountWhileCharging = 0;
			for (TActorIterator<AKCActionProjectile> It(TestWorld); It; ++It)
			{
				++ProjectileCountWhileCharging;
			}
			TestEqual(
				TEXT("Press만으로는 충전 투사체를 생성하지 않는다."),
				ProjectileCountWhileCharging,
				ProjectileCountBefore);

			const float ChargeStartWorldTime = TestWorld->GetTimeSeconds();
			TestWorld->Tick(LEVELTICK_All, 0.5f);
			const float ActualHeldDuration = static_cast<float>(
				TestWorld->GetTimeSeconds() - ChargeStartWorldTime);
			TestTrue(
				TEXT("테스트 World에서 충전 시간이 진행된다."),
				ActualHeldDuration > 0.0f);
			TestTrue(
				TEXT("Release가 현재 충전량으로 투척을 실행한다."),
				HeldItemComponent->ReleaseHeldItemUse());
			TestTrue(
				TEXT("Release의 투사체 생성 성공은 성게 원본 소비를 예약한다."),
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
			if (SpawnedProjectile)
			{
				const FVector ChargedVelocity =
					SpawnedProjectile->GetProjectileMovement()->Velocity;
				const UKCSingleActionDefinition* SingleAction =
					Cast<UKCSingleActionDefinition>(Definition->UseAction);
				const UKCThrowProjectileFragment* ChargedThrow = SingleAction
					? SingleAction->FindChargedThrowProjectileFragment()
					: nullptr;
				const float ExpectedForwardSpeed = ChargedThrow
					? ChargedThrow->LaunchConfig.ResolveForwardSpeed(
						ChargedThrow->LaunchConfig.CalculateChargeAlpha(
							ActualHeldDuration))
					: 0.0f;
				TestEqual(
					TEXT("실제 전방 속도는 서버 충전 시간으로 계산한 값과 같다."),
					static_cast<float>(FVector::DotProduct(
						ChargedVelocity,
						SourcePlayer->GetActorForwardVector())),
					ExpectedForwardSpeed,
					5.0f);
				TestTrue(
					TEXT("입력을 유지한 만큼 최소 투척 속도보다 강해진다."),
					ExpectedForwardSpeed > 400.0f);
				TestTrue(
					TEXT("충전 중에도 상향 속도는 350으로 유지한다."),
					FMath::IsNearlyEqual(ChargedVelocity.Z, 350.0f, 1.0f));
			}

			TestTrue(
				TEXT("원본 성게의 소비 예약 뒤에도 생성된 투사체는 유지된다."),
				IsValid(SpawnedProjectile));
		}
	}

	TestWorld->DestroyWorld(false);
	GEngine->DestroyWorldContext(TestWorld);
	return true;
}

#endif
