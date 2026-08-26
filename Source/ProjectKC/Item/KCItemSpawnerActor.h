#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KCItemSpawnerActor.generated.h"

class AKCWorldItemActor;
class UArrowComponent;
class UKCItemDefinition;
class USceneComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FKCItemSpawnedSignature,
	AKCWorldItemActor*,
	SpawnedItem);

/**
 * 고정된 Definition의 월드 아이템을 일정 간격으로 스폰한다.
 * 개수 상한이 없으므로 아무도 수거하지 않으면 아이템은 계속 쌓인다.
 * 스폰은 서버에서만 일어나고, 스폰된 아이템이 스스로 복제된다.
 */
UCLASS(Blueprintable)
class PROJECTKC_API AKCItemSpawnerActor : public AActor
{
	GENERATED_BODY()

public:
	AKCItemSpawnerActor();

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(
		FDataValidationContext& Context) const override;
#endif

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "KC|Item Spawner")
	void StartSpawning();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "KC|Item Spawner")
	void StopSpawning();

	/** 주기와 무관하게 한 개를 즉시 스폰한다. 실패하면 nullptr를 반환한다. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "KC|Item Spawner")
	AKCWorldItemActor* SpawnItem();

	UFUNCTION(BlueprintPure, Category = "KC|Item Spawner")
	bool IsSpawning() const;

	/** 서버에서만 Broadcast된다. 스폰 연출은 아이템 쪽 복제 상태에 맞춰 붙인다. */
	UPROPERTY(BlueprintAssignable, Category = "KC|Item Spawner")
	FKCItemSpawnedSignature OnItemSpawned;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KC|Item Spawner")
	TObjectPtr<UKCItemDefinition> ItemDefinition;

	/** 스폰할 Actor 클래스다. 표현만 다른 아이템 BP가 있을 때만 바꾼다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KC|Item Spawner")
	TSubclassOf<AKCWorldItemActor> ItemActorClass;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "KC|Item Spawner",
		meta = (ClampMin = "0.05", Units = "s"))
	float SpawnInterval = 5.f;

	/** 시작부터 첫 스폰까지의 지연이다. 0이면 시작 직후 한 개를 스폰한다. */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "KC|Item Spawner",
		meta = (ClampMin = "0.0", Units = "s"))
	float InitialDelay = 0.f;

	/**
	 * 스폰 지점 주변 수평면에 아이템을 흩어놓을 반경이다.
	 * 0이면 항상 같은 지점에 스폰해 물리 아이템끼리 겹친 채 쌓인다.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "KC|Item Spawner",
		meta = (ClampMin = "0.0", Units = "cm"))
	float SpawnRadius = 0.f;

	/** 끄면 BeginPlay에서 시작하지 않고 StartSpawning() 호출을 기다린다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KC|Item Spawner")
	bool bAutoStart = true;

	/** 스폰 기준점이다. 아이템은 이 컴포넌트의 위치와 회전으로 나온다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|Item Spawner")
	TObjectPtr<USceneComponent> SpawnPoint;

private:
	void HandleSpawnTimer();
	FTransform BuildSpawnTransform() const;

	FTimerHandle SpawnTimerHandle;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UArrowComponent> EditorArrow;
#endif
};
