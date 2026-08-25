#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AbilitySystemComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "ProjectKC/AbilitySystem/Attribute/KCCookingProgressAttributeSet.h"
#include "ProjectKC/AbilitySystem/Effect/KCGE_CookingProgressDecrease.h"
#include "ProjectKC/AbilitySystem/Effect/KCGE_CookingProgressIncrease.h"
#include "ProjectKC/AbilitySystem/Fragment/KCApplyGameplayEffectFragment.h"
#include "ProjectKC/AbilitySystem/Tag/KCAbilityGameplayTags.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FKCCookingProgressSystemTest,
	"ProjectKC.GAS.Attributes.CookingProgress",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKCCookingProgressSystemTest::RunTest(const FString& Parameters)
{
	const UKCCookingProgressAttributeSet* Defaults =
		GetDefault<UKCCookingProgressAttributeSet>();
	TestEqual(
		TEXT("조리 진행도는 0에서 시작한다."),
		Defaults->GetCookingProgress(),
		0.0f);
	TestEqual(
		TEXT("기본 최대 조리 진행도는 100이다."),
		Defaults->GetMaxCookingProgress(),
		100.0f);

	const UKCGE_CookingProgressIncrease* IncreaseEffect =
		GetDefault<UKCGE_CookingProgressIncrease>();
	TestTrue(
		TEXT("조리 진행도 증가 GE는 Instant다."),
		IncreaseEffect->DurationPolicy == EGameplayEffectDurationType::Instant);
	TestEqual(
		TEXT("조리 진행도 증가 GE는 ExecutionCalculation을 사용하지 않는다."),
		IncreaseEffect->Executions.Num(),
		0);
	TestEqual(
		TEXT("조리 진행도 증가 GE는 Modifier 하나만 가진다."),
		IncreaseEffect->Modifiers.Num(),
		1);
	if (IncreaseEffect->Modifiers.Num() == 1)
	{
		const FGameplayModifierInfo& Modifier = IncreaseEffect->Modifiers[0];
		TestTrue(
			TEXT("증가 Modifier가 CookingProgress에 직접 적용된다."),
			Modifier.Attribute ==
				UKCCookingProgressAttributeSet::GetCookingProgressAttribute());
		TestTrue(
			TEXT("증가 Modifier는 Additive 연산을 사용한다."),
			Modifier.ModifierOp == EGameplayModOp::Additive);
		TestTrue(
			TEXT("증가 Modifier는 증가 태그를 사용한다."),
			Modifier.ModifierMagnitude.GetSetByCallerFloat().DataTag ==
				TAG_KC_Data_Cooking_Progress_Increase);
	}

	const UKCGE_CookingProgressDecrease* DecreaseEffect =
		GetDefault<UKCGE_CookingProgressDecrease>();
	TestTrue(
		TEXT("조리 진행도 감소 GE는 Instant다."),
		DecreaseEffect->DurationPolicy == EGameplayEffectDurationType::Instant);
	TestEqual(
		TEXT("조리 진행도 감소 GE는 ExecutionCalculation을 사용하지 않는다."),
		DecreaseEffect->Executions.Num(),
		0);
	TestEqual(
		TEXT("조리 진행도 감소 GE는 Modifier 하나만 가진다."),
		DecreaseEffect->Modifiers.Num(),
		1);
	if (DecreaseEffect->Modifiers.Num() == 1)
	{
		const FGameplayModifierInfo& Modifier = DecreaseEffect->Modifiers[0];
		TestTrue(
			TEXT("감소 Modifier가 CookingProgress에 직접 적용된다."),
			Modifier.Attribute ==
				UKCCookingProgressAttributeSet::GetCookingProgressAttribute());
		TestTrue(
			TEXT("감소 Modifier는 Additive 연산을 사용한다."),
			Modifier.ModifierOp == EGameplayModOp::Additive);
		TestTrue(
			TEXT("감소 Modifier는 감소 태그를 사용한다."),
			Modifier.ModifierMagnitude.GetSetByCallerFloat().DataTag ==
				TAG_KC_Data_Cooking_Progress_Decrease);
	}

	const FName TestWorldName = MakeUniqueObjectName(
		nullptr,
		UWorld::StaticClass(),
		TEXT("KCCookingProgressTestWorld"),
		EUniqueObjectNameOptions::GloballyUnique);
	UWorld* TestWorld = UWorld::CreateWorld(
		EWorldType::Game,
		false,
		TestWorldName,
		GetTransientPackage());
	if (TestWorld)
	{
		FWorldContext& WorldContext =
			GEngine->CreateNewWorldContext(EWorldType::Game);
		WorldContext.SetCurrentWorld(TestWorld);
	}
	AActor* TestOwner = TestWorld
		? TestWorld->SpawnActor<AActor>()
		: nullptr;
	if (!TestNotNull(TEXT("GE 실행용 테스트 World를 만든다."), TestWorld) ||
		!TestNotNull(TEXT("GE 실행용 테스트 Actor를 만든다."), TestOwner))
	{
		if (TestWorld)
		{
			TestWorld->DestroyWorld(false);
			GEngine->DestroyWorldContext(TestWorld);
		}
		return false;
	}

	UAbilitySystemComponent* AbilitySystem =
		NewObject<UAbilitySystemComponent>(TestOwner);
	AbilitySystem->RegisterComponent();
	AbilitySystem->InitAbilityActorInfo(TestOwner, TestOwner);
	UKCCookingProgressAttributeSet* CookingAttributes =
		NewObject<UKCCookingProgressAttributeSet>(TestOwner);
	AbilitySystem->AddAttributeSetSubobject(CookingAttributes);
	TestTrue(
		TEXT("ASC에 조리 진행도 AttributeSet이 등록된다."),
		AbilitySystem->HasAttributeSetForAttribute(
			UKCCookingProgressAttributeSet::GetCookingProgressAttribute()));

	auto ApplyProgress = [this, AbilitySystem](
		TSubclassOf<UGameplayEffect> EffectClass,
		FGameplayTag DataTag,
		float Magnitude)
	{
		FGameplayEffectSpecHandle SpecHandle = AbilitySystem->MakeOutgoingSpec(
			EffectClass,
			1.0f,
			AbilitySystem->MakeEffectContext());
		if (!SpecHandle.IsValid())
		{
			AddError(TEXT("조리 진행도 GE Spec을 만들지 못했습니다."));
			return false;
		}

		SpecHandle.Data->SetSetByCallerMagnitude(
			DataTag,
			Magnitude);
		AbilitySystem->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		return true;
	};

	TestTrue(
		TEXT("증가 GE를 적용한다."),
		ApplyProgress(
			UKCGE_CookingProgressIncrease::StaticClass(),
			TAG_KC_Data_Cooking_Progress_Increase,
			60.0f));
	TestEqual(
		TEXT("양수 Delta만큼 진행도가 증가한다."),
		CookingAttributes->GetCookingProgress(),
		60.0f);

	TestTrue(
		TEXT("감소 GE를 적용한다."),
		ApplyProgress(
			UKCGE_CookingProgressDecrease::StaticClass(),
			TAG_KC_Data_Cooking_Progress_Decrease,
			-12.0f));
	TestEqual(
		TEXT("음수 Delta만큼 진행도가 감소한다."),
		CookingAttributes->GetCookingProgress(),
		48.0f);

	ApplyProgress(
		UKCGE_CookingProgressIncrease::StaticClass(),
		TAG_KC_Data_Cooking_Progress_Increase,
		200.0f);
	TestEqual(
		TEXT("진행도는 MaxCookingProgress를 넘지 않는다."),
		CookingAttributes->GetCookingProgress(),
		100.0f);

	ApplyProgress(
		UKCGE_CookingProgressDecrease::StaticClass(),
		TAG_KC_Data_Cooking_Progress_Decrease,
		-150.0f);
	TestEqual(
		TEXT("진행도는 0 아래로 내려가지 않는다."),
		CookingAttributes->GetCookingProgress(),
		0.0f);

	UKCApplyGameplayEffectFragment* DecreaseFragment =
		NewObject<UKCApplyGameplayEffectFragment>();
	DecreaseFragment->EffectRecipe.EffectClass =
		UKCGE_CookingProgressDecrease::StaticClass();
	FKCSetByCallerValueStruct ProgressDecrease;
	ProgressDecrease.DataTag = TAG_KC_Data_Cooking_Progress_Decrease;
	ProgressDecrease.Magnitude = -25.0f;
	DecreaseFragment->EffectRecipe.SetByCallers.Add(ProgressDecrease);

	FString Error;
	TestTrue(
		TEXT("조리 진행도 감소 Recipe는 유효하다."),
		DecreaseFragment->Validate(Error));

	TestOwner->Destroy();
	TestWorld->DestroyWorld(false);
	GEngine->DestroyWorldContext(TestWorld);

	return true;
}

#endif
