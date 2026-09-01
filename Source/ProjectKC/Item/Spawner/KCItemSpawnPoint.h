#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KCItemSpawnPoint.generated.h"

class UKCItemDefinition;
class USphereComponent;
class UArrowComponent;

/** 위치만 제공한다. 수량과 재스폰은 KCItemSpawnManager가 관리한다. */
UCLASS()
class PROJECTKC_API AKCItemSpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	AKCItemSpawnPoint();
	bool IsAvailable(const UKCItemDefinition& Definition) const;
	FTransform GetSpawnTransform() const;

	/** 바닥 위에 이 반경만큼 여유를 두고 배치한다. */
	UPROPERTY(EditAnywhere, Category = "KC|Spawn Point", meta = (ClampMin = "1.0", Units = "cm"))
	float ClearanceRadius = 35.f;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(VisibleAnywhere, Category = "KC|Spawn Point")
	TObjectPtr<USphereComponent> PreviewSphere;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UArrowComponent> EditorArrow;
#endif
};
