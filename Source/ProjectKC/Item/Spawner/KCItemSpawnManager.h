#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "ProjectKC/Item/KCWorldItemActor.h"
#include "KCItemSpawnManager.generated.h"

class AKCItemSpawnPoint;
class UKCItemDefinition;
struct FKCActiveRecipesChangedStruct;

UENUM(BlueprintType)
enum class EKCIngredientPlacementMode : uint8
{
	SpawnPoints,
	Orbit
};

USTRUCT()
struct FKCIngredientSpawnEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Spawn")
	TObjectPtr<UKCItemDefinition> ItemDefinition;

	UPROPERTY(EditAnywhere, Category = "Spawn", meta = (ClampMin = "0"))
	int32 TargetCount = 1;
};

USTRUCT()
struct FKCWeightedItemSpawnEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Spawn")
	TObjectPtr<UKCItemDefinition> ItemDefinition;

	/** 0이면 선택하지 않는다. 나머지는 전체 가중치 합에 대한 비율로 선택한다. */
	UPROPERTY(EditAnywhere, Category = "Spawn", meta = (ClampMin = "0.0"))
	float Weight = 1.f;
};

/** 레벨에 하나만 배치한다. 자신이 생성한 아이템만 서버에서 집계한다. */
UCLASS()
class PROJECTKC_API AKCItemSpawnManager : public AActor
{
	GENERATED_BODY()

public:
	AKCItemSpawnManager();
	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	UPROPERTY(EditAnywhere, Category = "KC|Spawn|Ingredients", meta = (TitleProperty = "ItemDefinition"))
	TArray<FKCIngredientSpawnEntry> Ingredients;

	UPROPERTY(EditInstanceOnly, Category = "KC|Spawn|Ingredients")
	TArray<TObjectPtr<AKCItemSpawnPoint>> IngredientSpawnPoints;

	UPROPERTY(EditAnywhere, Category = "KC|Spawn|Ingredients")
	EKCIngredientPlacementMode IngredientPlacementMode =
		EKCIngredientPlacementMode::SpawnPoints;

	UPROPERTY(EditAnywhere, Category = "KC|Spawn|Ingredients", meta = (ClampMin = "0.0", Units = "s"))
	float IngredientRespawnDelayMin = 5.f;

	UPROPERTY(EditAnywhere, Category = "KC|Spawn|Ingredients", meta = (ClampMin = "0.0", Units = "s"))
	float IngredientRespawnDelayMax = 10.f;

	/** 비어 있으면 이 관리 액터의 위치를 회전 중심으로 사용한다. */
	UPROPERTY(EditInstanceOnly, Category = "KC|Spawn|Ingredients|Orbit")
	TObjectPtr<AActor> IngredientOrbitCenter;

	UPROPERTY(EditAnywhere, Category = "KC|Spawn|Ingredients|Orbit", meta = (ClampMin = "1.0", Units = "cm"))
	float IngredientOrbitRadius = 200.f;

	UPROPERTY(EditAnywhere, Category = "KC|Spawn|Ingredients|Orbit", meta = (Units = "cm"))
	float IngredientOrbitHeight = 0.f;

	UPROPERTY(EditAnywhere, Category = "KC|Spawn|Ingredients|Orbit", meta = (ClampMin = "0.0", Units = "deg/s"))
	float IngredientOrbitDegreesPerSecond = 30.f;

	UPROPERTY(EditAnywhere, Category = "KC|Spawn|Ingredients|Orbit")
	bool bIngredientOrbitClockwise = true;

	UPROPERTY(EditAnywhere, Category = "KC|Spawn|Items", meta = (TitleProperty = "ItemDefinition"))
	TArray<FKCWeightedItemSpawnEntry> Items;

	UPROPERTY(EditAnywhere, Category = "KC|Spawn|Items", meta = (ClampMin = "0"))
	int32 MaxItemCount = 3;

	UPROPERTY(EditInstanceOnly, Category = "KC|Spawn|Items")
	TArray<TObjectPtr<AKCItemSpawnPoint>> ItemSpawnPoints;

	UPROPERTY(EditAnywhere, Category = "KC|Spawn|Items", meta = (ClampMin = "0.0", Units = "s"))
	float ItemRespawnDelayMin = 5.f;

	UPROPERTY(EditAnywhere, Category = "KC|Spawn|Items", meta = (ClampMin = "0.0", Units = "s"))
	float ItemRespawnDelayMax = 10.f;

	UPROPERTY(EditAnywhere, Category = "KC|Spawn")
	TSubclassOf<AKCWorldItemActor> ItemActorClass;

	UPROPERTY(EditAnywhere, Category = "KC|Spawn", meta = (ClampMin = "0.05", Units = "s"))
	float RetryInterval = 1.f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

private:
	friend class FKCItemSpawnManagerTest;

	struct FSpawnSlot
	{
		bool bIngredient = false;
		bool bOrbiting = false;
		TWeakObjectPtr<UKCItemDefinition> Definition;
		TWeakObjectPtr<AKCWorldItemActor> Item;
		double ReadyTime = 0.0;
	};

	bool ValidateSettings(FString& OutError) const;
	void RefreshRecipes();
	void HandleRecipesChanged(FGameplayTag Channel, const FKCActiveRecipesChangedStruct& Message);
	void ServiceSpawns();
	UKCItemDefinition* SelectWeightedItem() const;
	AKCWorldItemActor* TrySpawn(
		UKCItemDefinition& Definition,
		bool bIngredient,
		const FTransform* OverrideTransform = nullptr);
	FTransform BuildOrbitTransform(const FSpawnSlot& TargetSlot) const;
	void UpdateIngredientOrbit(float DeltaSeconds);
	void DisableReplicatedOrbitPhysics();

	UFUNCTION()
	void HandleItemDestroyed(AActor* DestroyedActor);

	UFUNCTION()
	void HandleItemStateChanged(EKCWorldItemState NewState, AActor* NewHolder);

	UFUNCTION()
	void OnRep_OrbitItems();

	UPROPERTY(ReplicatedUsing = OnRep_OrbitItems)
	TArray<TObjectPtr<AKCWorldItemActor>> ReplicatedOrbitItems;

	TArray<FSpawnSlot> Slots;
	FGameplayMessageListenerHandle RecipeListener;
	FTimerHandle ServiceTimer;
	bool bRunning = false;
	bool bRecipesRead = false;
	float IngredientOrbitAngle = 0.f;
};
