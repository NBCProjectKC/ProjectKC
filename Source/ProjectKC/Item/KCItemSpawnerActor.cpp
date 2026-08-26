#include "ProjectKC/Item/KCItemSpawnerActor.h"

#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Misc/DataValidation.h"
#include "ProjectKC/Item/Definition/KCItemDefinition.h"
#include "ProjectKC/Item/KCWorldItemActor.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogKCItemSpawner, Log, All);

AKCItemSpawnerActor::AKCItemSpawnerActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnPoint"));
	SetRootComponent(SpawnPoint);

	ItemActorClass = AKCWorldItemActor::StaticClass();

#if WITH_EDITORONLY_DATA
	EditorArrow =
		CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("EditorArrow"));
	if (EditorArrow)
	{
		EditorArrow->SetupAttachment(SpawnPoint);
		EditorArrow->bIsScreenSizeScaled = true;
	}
#endif
}

#if WITH_EDITOR
EDataValidationResult AKCItemSpawnerActor::IsDataValid(
	FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	FString Error;
	if (!ItemDefinition || !ItemDefinition->Validate(Error))
	{
		Context.AddError(ItemDefinition
			? FText::FromString(Error)
			: FText::FromString(TEXT("ItemDefinition이 비어 있습니다.")));
		return EDataValidationResult::Invalid;
	}

	if (!ItemActorClass)
	{
		Context.AddError(
			FText::FromString(TEXT("ItemActorClass가 비어 있습니다.")));
		return EDataValidationResult::Invalid;
	}

	return Result == EDataValidationResult::NotValidated
		? EDataValidationResult::Valid
		: Result;
}
#endif

void AKCItemSpawnerActor::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority() || !bAutoStart)
	{
		return;
	}

	StartSpawning();
}

void AKCItemSpawnerActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(SpawnTimerHandle);

	Super::EndPlay(EndPlayReason);
}

void AKCItemSpawnerActor::StartSpawning()
{
	if (!HasAuthority() || IsSpawning())
	{
		return;
	}

	GetWorldTimerManager().SetTimer(
		SpawnTimerHandle,
		this,
		&AKCItemSpawnerActor::HandleSpawnTimer,
		SpawnInterval,
		true,
		InitialDelay);
}

void AKCItemSpawnerActor::StopSpawning()
{
	GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
}

bool AKCItemSpawnerActor::IsSpawning() const
{
	return GetWorldTimerManager().IsTimerActive(SpawnTimerHandle);
}

AKCWorldItemActor* AKCItemSpawnerActor::SpawnItem()
{
	UWorld* World = GetWorld();
	if (!HasAuthority() || !World)
	{
		return nullptr;
	}

	// 설정이 잘못된 스포너는 주기마다 같은 오류를 남기므로 주기 자체를 멈춘다.
	if (!IsValid(ItemDefinition) || !ItemActorClass)
	{
		UE_LOG(
			LogKCItemSpawner,
			Error,
			TEXT("Spawner '%s'에 ItemDefinition 또는 ItemActorClass가 없어 스폰을 멈춥니다."),
			*GetName());
		StopSpawning();
		return nullptr;
	}

	const FTransform SpawnTransform = BuildSpawnTransform();
	AKCWorldItemActor* SpawnedItem = World->SpawnActorDeferred<AKCWorldItemActor>(
		ItemActorClass,
		SpawnTransform,
		nullptr,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!SpawnedItem)
	{
		return nullptr;
	}

	// BeginPlay 전에 Definition을 넣어야 아이템이 무효 상태로 시작하지 않는다.
	if (!SpawnedItem->InitializeItem(ItemDefinition))
	{
		UE_LOG(
			LogKCItemSpawner,
			Error,
			TEXT("Spawner '%s'의 ItemDefinition '%s'이 유효하지 않아 스폰을 멈춥니다."),
			*GetName(),
			*GetNameSafe(ItemDefinition));
		SpawnedItem->Destroy();
		StopSpawning();
		return nullptr;
	}

	SpawnedItem->FinishSpawning(SpawnTransform);
	if (!IsValid(SpawnedItem))
	{
		return nullptr;
	}

	OnItemSpawned.Broadcast(SpawnedItem);
	return SpawnedItem;
}

void AKCItemSpawnerActor::HandleSpawnTimer()
{
	SpawnItem();
}

FTransform AKCItemSpawnerActor::BuildSpawnTransform() const
{
	FTransform SpawnTransform = SpawnPoint->GetComponentTransform();

	// 스포너를 크게 잡아둬도 아이템 크기는 아이템 쪽 기본값을 따른다.
	SpawnTransform.SetScale3D(FVector::OneVector);

	if (SpawnRadius > UE_KINDA_SMALL_NUMBER)
	{
		const FVector2D RandomOffset = FMath::RandPointInCircle(SpawnRadius);
		SpawnTransform.AddToTranslation(
			FVector(RandomOffset.X, RandomOffset.Y, 0.f));
	}

	return SpawnTransform;
}
