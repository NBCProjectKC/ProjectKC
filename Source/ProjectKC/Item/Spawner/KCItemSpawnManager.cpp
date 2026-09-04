#include "ProjectKC/Item/Spawner/KCItemSpawnManager.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/DataValidation.h"
#include "ProjectKC/GameSystem/KCGameState.h"
#include "ProjectKC/GameSystem/Recipe/KCRecipeStruct.h"
#include "ProjectKC/Item/Definition/KCItemDefinition.h"
#include "ProjectKC/Item/KCWorldItemActor.h"
#include "ProjectKC/Item/Spawner/KCItemSpawnPoint.h"
#include "ProjectKC/Messages/KCGameplayTags.h"
#include "ProjectKC/Messages/Struct/KCActiveRecipesChangedStruct.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogKCItemSpawnManager, Log, All);

AKCItemSpawnManager::AKCItemSpawnManager()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = true;
	SetReplicateMovement(false);
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
	ItemActorClass = AKCWorldItemActor::StaticClass();
}

void AKCItemSpawnManager::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AKCItemSpawnManager, ReplicatedOrbitItems);
}

bool AKCItemSpawnManager::ValidateSettings(FString& OutError) const
{
	if (!ItemActorClass || ItemActorClass->HasAnyClassFlags(CLASS_Abstract) ||
		MaxItemCount < 0 ||
		!FMath::IsFinite(IngredientRespawnDelayMin) ||
		!FMath::IsFinite(IngredientRespawnDelayMax) ||
		IngredientRespawnDelayMin < 0.f ||
		IngredientRespawnDelayMax < IngredientRespawnDelayMin ||
		!FMath::IsFinite(IngredientOrbitRadius) ||
		IngredientOrbitRadius < 1.f ||
		!FMath::IsFinite(IngredientOrbitHeight) ||
		!FMath::IsFinite(IngredientOrbitDegreesPerSecond) ||
		IngredientOrbitDegreesPerSecond < 0.f ||
		!FMath::IsFinite(ItemRespawnDelayMin) ||
		!FMath::IsFinite(ItemRespawnDelayMax) ||
		ItemRespawnDelayMin < 0.f ||
		ItemRespawnDelayMax < ItemRespawnDelayMin ||
		!FMath::IsFinite(RetryInterval) || RetryInterval < 0.05f)
	{
		OutError = TEXT("스폰 클래스, 수량 또는 재스폰/재시도 시간을 확인하세요.");
		return false;
	}

	TSet<FGameplayTag> ItemIds;
	auto ValidateDefinition = [&ItemIds, &OutError](UKCItemDefinition* Definition)
	{
		if (!IsValid(Definition))
		{
			OutError = TEXT("ItemDefinition이 비어 있습니다.");
			return false;
		}
		if (!Definition->Validate(OutError))
		{
			return false;
		}
		if (ItemIds.Contains(Definition->ItemId))
		{
			OutError = TEXT("재료/일반 아이템 목록에 중복 ItemId가 있습니다.");
			return false;
		}
		ItemIds.Add(Definition->ItemId);
		return true;
	};
	for (const FKCIngredientSpawnEntry& Entry : Ingredients)
	{
		if (!ValidateDefinition(Entry.ItemDefinition)) { return false; }
		if (Entry.TargetCount < 0)
		{
			OutError = TEXT("재료 목표 수량은 0 이상이어야 합니다.");
			return false;
		}
	}
	double TotalWeight = 0.0;
	for (const FKCWeightedItemSpawnEntry& Entry : Items)
	{
		if (!ValidateDefinition(Entry.ItemDefinition)) { return false; }
		if (!FMath::IsFinite(Entry.Weight) || Entry.Weight < 0.f)
		{
			OutError = TEXT("가중치는 유한한 0 이상의 값이어야 합니다.");
			return false;
		}
		TotalWeight += Entry.Weight;
	}
	if (!Items.IsEmpty() && MaxItemCount > 0 && TotalWeight <= 0.0)
	{
		OutError = TEXT("일반 아이템의 양수 가중치가 필요합니다.");
		return false;
	}
	const auto HasPoint = [](const TArray<TObjectPtr<AKCItemSpawnPoint>>& Points)
	{
		return Points.ContainsByPredicate([](const AKCItemSpawnPoint* Point) { return IsValid(Point); });
	};
	const bool bNeedsIngredients = Ingredients.ContainsByPredicate(
		[](const FKCIngredientSpawnEntry& Entry) { return Entry.TargetCount > 0; });
	// BP 기본값/템플릿에는 레벨 액터 참조를 지정할 수 없다. 배치된 인스턴스만 검사한다.
	const bool bNeedsIngredientPoints = bNeedsIngredients &&
		IngredientPlacementMode == EKCIngredientPlacementMode::SpawnPoints;
	if (!IsTemplate() && ((bNeedsIngredientPoints && !HasPoint(IngredientSpawnPoints)) ||
		(!Items.IsEmpty() && MaxItemCount > 0 && !HasPoint(ItemSpawnPoints))))
	{
		OutError = TEXT("생성할 재료/일반 아이템에 사용할 스폰 포인트를 지정하세요.");
		return false;
	}
	OutError.Reset();
	return true;
}

#if WITH_EDITOR
EDataValidationResult AKCItemSpawnManager::IsDataValid(FDataValidationContext& Context) const
{
	const EDataValidationResult Result = Super::IsDataValid(Context);
	FString Error;
	if (!ValidateSettings(Error))
	{
		Context.AddError(FText::FromString(Error));
		return EDataValidationResult::Invalid;
	}
	return Result == EDataValidationResult::Invalid ? Result : EDataValidationResult::Valid;
}
#endif

void AKCItemSpawnManager::BeginPlay()
{
	Super::BeginPlay();
	if (GetNetMode() == NM_Client)
	{
		SetActorTickEnabled(
			IngredientPlacementMode == EKCIngredientPlacementMode::Orbit);
		return;
	}
	if (!HasAuthority()) { return; }
	for (TActorIterator<AKCItemSpawnManager> It(GetWorld()); It; ++It)
	{
		if (*It != this && It->bRunning)
		{
			UE_LOG(LogKCItemSpawnManager, Error, TEXT("관리 액터 '%s'가 이미 실행 중이므로 '%s'는 시작하지 않습니다."), *It->GetName(), *GetName());
			return;
		}
	}
	FString Error;
	if (!ValidateSettings(Error))
	{
		UE_LOG(LogKCItemSpawnManager, Error, TEXT("%s: %s"), *GetName(), *Error);
		return;
	}
	bRunning = true;
	SetActorTickEnabled(
		IngredientPlacementMode == EKCIngredientPlacementMode::Orbit);
	if (!Items.IsEmpty()) { Slots.SetNum(MaxItemCount); }
	RecipeListener = UGameplayMessageSubsystem::Get(this).RegisterListener<FKCActiveRecipesChangedStruct>(
		KCGameplayTags::Message_Game_ActiveRecipesChanged, this, &ThisClass::HandleRecipesChanged);
	RefreshRecipes();
	ServiceSpawns();
}

void AKCItemSpawnManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	bRunning = false;
	GetWorldTimerManager().ClearTimer(ServiceTimer);
	RecipeListener.Unregister();
	for (const FSpawnSlot& Slot : Slots)
	{
		if (AKCWorldItemActor* Item = Slot.Item.Get())
		{
			Item->OnDestroyed.RemoveDynamic(this, &ThisClass::HandleItemDestroyed);
			Item->OnItemStateChanged.RemoveDynamic(
				this,
				&ThisClass::HandleItemStateChanged);
		}
	}
	ReplicatedOrbitItems.Reset();
	Slots.Reset();
	Super::EndPlay(EndPlayReason);
}

void AKCItemSpawnManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (IngredientPlacementMode != EKCIngredientPlacementMode::Orbit)
	{
		return;
	}
	if (GetNetMode() == NM_Client)
	{
		DisableReplicatedOrbitPhysics();
		return;
	}
	if (bRunning && HasAuthority())
	{
		UpdateIngredientOrbit(DeltaSeconds);
	}
}

void AKCItemSpawnManager::HandleRecipesChanged(FGameplayTag Channel, const FKCActiveRecipesChangedStruct& Message)
{
	RefreshRecipes();
}

void AKCItemSpawnManager::RefreshRecipes()
{
	const AKCGameState* GameState = GetWorld()->GetGameState<AKCGameState>();
	if (!GameState) { return; }
	bRecipesRead = true;
	TSet<FGameplayTag> RequiredIds;
	for (FName RowName : GameState->GetActiveRecipes())
	{
		const FKCRecipeStruct* Recipe = GameState->FindRecipeByRowName(RowName);
		if (!Recipe)
		{
			UE_LOG(LogKCItemSpawnManager, Error, TEXT("활성 레시피 '%s'를 찾을 수 없습니다."), *RowName.ToString());
			continue;
		}
		for (const FGameplayTag& Id : Recipe->RequiredIngredients) { RequiredIds.Add(Id); }
	}
	for (const FKCIngredientSpawnEntry& Entry : Ingredients)
	{
		const bool bRequired = RequiredIds.Remove(Entry.ItemDefinition->ItemId) > 0;
		const int32 Target = bRequired ? Entry.TargetCount : 0;
		int32 Count = 0;
		for (const FSpawnSlot& Slot : Slots)
		{
			if (Slot.bIngredient && Slot.Definition == Entry.ItemDefinition) { ++Count; }
		}
		// 이미 생성된 재료는 보유 여부와 무관하게 남기고, 빈 슬롯만 제거한다.
		for (int32 Index = Slots.Num() - 1; Index >= 0 && Count > Target; --Index)
		{
			const FSpawnSlot& Slot = Slots[Index];
			if (Slot.bIngredient && Slot.Definition == Entry.ItemDefinition && !Slot.Item.IsValid())
			{
				Slots.RemoveAt(Index);
				--Count;
			}
		}
		while (Count++ < Target)
		{
			FSpawnSlot& Slot = Slots.AddDefaulted_GetRef();
			Slot.bIngredient = true;
			Slot.Definition = Entry.ItemDefinition;
		}
	}
	for (const FGameplayTag& Id : RequiredIds)
	{
		UE_LOG(LogKCItemSpawnManager, Error, TEXT("레시피에 필요한 재료 '%s'가 Ingredients 목록에 없습니다."), *Id.ToString());
	}
}

UKCItemDefinition* AKCItemSpawnManager::SelectWeightedItem() const
{
	double TotalWeight = 0.0;
	for (const FKCWeightedItemSpawnEntry& Entry : Items) { TotalWeight += Entry.Weight; }
	double Draw = FMath::FRand() * TotalWeight;
	UKCItemDefinition* LastEligible = nullptr;
	for (const FKCWeightedItemSpawnEntry& Entry : Items)
	{
		if (Entry.Weight <= 0.f) { continue; }
		LastEligible = Entry.ItemDefinition;
		Draw -= Entry.Weight;
		if (Draw < 0.0) { return Entry.ItemDefinition; }
	}
	return LastEligible;
}

AKCWorldItemActor* AKCItemSpawnManager::TrySpawn(
	UKCItemDefinition& Definition,
	bool bIngredient,
	const FTransform* OverrideTransform)
{
	if (OverrideTransform)
	{
		AKCWorldItemActor* Item = GetWorld()->SpawnActorDeferred<AKCWorldItemActor>(
			ItemActorClass,
			*OverrideTransform,
			nullptr,
			nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Item)
		{
			return nullptr;
		}
		if (!Item->InitializeItem(&Definition))
		{
			Item->Destroy();
			return nullptr;
		}
		Item->FinishSpawning(*OverrideTransform);
		return IsValid(Item) ? Item : nullptr;
	}

	const TArray<TObjectPtr<AKCItemSpawnPoint>>& Points = bIngredient ? IngredientSpawnPoints : ItemSpawnPoints;
	if (Points.IsEmpty()) { return nullptr; }
	const int32 Start = FMath::RandHelper(Points.Num());
	for (int32 Offset = 0; Offset < Points.Num(); ++Offset)
	{
		AKCItemSpawnPoint* Point = Points[(Start + Offset) % Points.Num()];
		if (!IsValid(Point) || Point->GetWorld() != GetWorld() || !Point->IsAvailable(Definition)) { continue; }
		const FTransform Transform = Point->GetSpawnTransform();
		AKCWorldItemActor* Item = GetWorld()->SpawnActorDeferred<AKCWorldItemActor>(
			ItemActorClass, Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding);
		if (!Item) { continue; }
		if (!Item->InitializeItem(&Definition))
		{
			Item->Destroy();
			return nullptr;
		}
		Item->FinishSpawning(Transform);
		if (IsValid(Item)) { return Item; }
	}
	return nullptr;
}

FTransform AKCItemSpawnManager::BuildOrbitTransform(
	const FSpawnSlot& TargetSlot) const
{
	int32 IngredientCount = 0;
	int32 TargetIndex = INDEX_NONE;
	for (const FSpawnSlot& Slot : Slots)
	{
		if (!Slot.bIngredient)
		{
			continue;
		}
		if (&Slot == &TargetSlot)
		{
			TargetIndex = IngredientCount;
		}
		++IngredientCount;
	}

	const AActor* Center = IsValid(IngredientOrbitCenter)
		? IngredientOrbitCenter.Get()
		: this;
	const float SlotAngle = IngredientCount > 0 && TargetIndex != INDEX_NONE
		? 360.f * static_cast<float>(TargetIndex) /
			static_cast<float>(IngredientCount)
		: 0.f;
	const float Angle = IngredientOrbitAngle + SlotAngle;
	const FRotator CenterRotation(0.f, Center->GetActorRotation().Yaw, 0.f);
	const FVector Offset = CenterRotation.RotateVector(
		FVector(IngredientOrbitRadius, 0.f, IngredientOrbitHeight).RotateAngleAxis(
			Angle,
			FVector::UpVector));
	const FRotator ItemRotation(0.f, CenterRotation.Yaw + Angle + 90.f, 0.f);
	return FTransform(ItemRotation, Center->GetActorLocation() + Offset);
}

void AKCItemSpawnManager::UpdateIngredientOrbit(float DeltaSeconds)
{
	const float Direction = bIngredientOrbitClockwise ? -1.f : 1.f;
	IngredientOrbitAngle = FMath::Fmod(
		IngredientOrbitAngle +
			Direction * IngredientOrbitDegreesPerSecond * DeltaSeconds,
		360.f);
	for (FSpawnSlot& Slot : Slots)
	{
		AKCWorldItemActor* Item = Slot.Item.Get();
		if (!Slot.bIngredient || !Slot.bOrbiting || !Item ||
			Item->GetItemState() != EKCWorldItemState::World)
		{
			continue;
		}
		const FTransform Transform = BuildOrbitTransform(Slot);
		Item->SetActorLocationAndRotation(
			Transform.GetLocation(),
			Transform.GetRotation(),
			false,
			nullptr,
			ETeleportType::TeleportPhysics);
	}
}

void AKCItemSpawnManager::DisableReplicatedOrbitPhysics()
{
	for (AKCWorldItemActor* Item : ReplicatedOrbitItems)
	{
		if (IsValid(Item) && Item->GetItemMesh())
		{
			Item->GetItemMesh()->SetSimulatePhysics(false);
		}
	}
}

void AKCItemSpawnManager::OnRep_OrbitItems()
{
	DisableReplicatedOrbitPhysics();
}

void AKCItemSpawnManager::ServiceSpawns()
{
	if (!HasAuthority() || GetNetMode() == NM_Client || !bRunning) { return; }
	if (!bRecipesRead) { RefreshRecipes(); }
	const double Now = GetWorld()->GetTimeSeconds();
	float NextDelay = RetryInterval;
	for (FSpawnSlot& Slot : Slots)
	{
		if (Slot.Item.IsValid()) { continue; }
		if (Slot.ReadyTime > Now)
		{
			NextDelay = FMath::Min(NextDelay, static_cast<float>(Slot.ReadyTime - Now));
			continue;
		}
		// 실패 때 재추첨하면 작은 아이템 쪽으로 확률이 치우치므로 선택을 유지한다.
		if (!Slot.Definition.IsValid() && !Slot.bIngredient) { Slot.Definition = SelectWeightedItem(); }
		UKCItemDefinition* Definition = Slot.Definition.Get();
		const bool bUseOrbit = Slot.bIngredient &&
			IngredientPlacementMode == EKCIngredientPlacementMode::Orbit;
		const FTransform OrbitTransform = bUseOrbit
			? BuildOrbitTransform(Slot)
			: FTransform::Identity;
		AKCWorldItemActor* Item = Definition
			? TrySpawn(
				*Definition,
				Slot.bIngredient,
				bUseOrbit ? &OrbitTransform : nullptr)
			: nullptr;
		if (Item)
		{
			Slot.Item = Item;
			Slot.bOrbiting = bUseOrbit;
			Item->OnDestroyed.AddDynamic(this, &ThisClass::HandleItemDestroyed);
			if (bUseOrbit)
			{
				ReplicatedOrbitItems.AddUnique(Item);
				DisableReplicatedOrbitPhysics();
				ForceNetUpdate();
				Item->OnItemStateChanged.AddDynamic(
					this,
					&ThisClass::HandleItemStateChanged);
			}
		}
	}
	GetWorldTimerManager().SetTimer(ServiceTimer, this, &ThisClass::ServiceSpawns, FMath::Max(0.001f, NextDelay), false);
}

void AKCItemSpawnManager::HandleItemDestroyed(AActor* DestroyedActor)
{
	if (!bRunning || !HasAuthority() || GetNetMode() == NM_Client) { return; }
	for (FSpawnSlot& Slot : Slots)
	{
		if (Slot.Item.Get(true) == DestroyedActor)
		{
			ReplicatedOrbitItems.Remove(
				Cast<AKCWorldItemActor>(DestroyedActor));
			ForceNetUpdate();
			Slot.Item.Reset();
			if (!Slot.bIngredient) { Slot.Definition.Reset(); }
			const float DelayMin = Slot.bIngredient
				? IngredientRespawnDelayMin
				: ItemRespawnDelayMin;
			const float DelayMax = Slot.bIngredient
				? IngredientRespawnDelayMax
				: ItemRespawnDelayMax;
			Slot.ReadyTime = GetWorld()->GetTimeSeconds() + FMath::FRandRange(DelayMin, DelayMax);
			break;
		}
	}
	// 더 이상 레시피에 필요하지 않은 빈 재료 슬롯은 보충하지 않는다.
	RefreshRecipes();
}

void AKCItemSpawnManager::HandleItemStateChanged(
	EKCWorldItemState NewState,
	AActor* NewHolder)
{
	if (NewState != EKCWorldItemState::Held)
	{
		return;
	}
	for (FSpawnSlot& Slot : Slots)
	{
		if (Slot.Item.IsValid() &&
			Slot.Item->GetItemState() == EKCWorldItemState::Held)
		{
			Slot.bOrbiting = false;
			ReplicatedOrbitItems.Remove(Slot.Item.Get());
			ForceNetUpdate();
		}
	}
}
