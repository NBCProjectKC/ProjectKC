#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KCPotClocheActor.generated.h"

class UStaticMeshComponent;

/** 게임 시작 후 열리며 재료 파밍을 허용하는 냄비 클로슈다. */
UCLASS(Blueprintable)
class PROJECTKC_API AKCPotClocheActor : public AActor
{
	GENERATED_BODY()

public:
	AKCPotClocheActor();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|Pot|Cloche")
	TObjectPtr<UStaticMeshComponent> ClocheMesh;

	/** 게임 시작 후 클로슈가 열리기까지 기다리는 시간이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KC|Pot|Cloche", meta = (ClampMin = "0.0"))
	float OpenDelay = 5.0f;

	/** 클로슈가 위로 이동할 거리(cm)다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KC|Pot|Cloche", meta = (ClampMin = "0.0"))
	float LiftHeight = 200.0f;

	/** 클로슈가 이동을 완료하는 데 걸리는 시간이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KC|Pot|Cloche", meta = (ClampMin = "0.01"))
	float LiftDuration = 1.0f;

private:
	void StartOpening();
	void FinishOpening();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStartOpening();

	FVector ClosedLocation = FVector::ZeroVector;
	FVector OpenLocation = FVector::ZeroVector;
	float LiftElapsedTime = 0.0f;
	bool bIsOpening = false;
	FTimerHandle OpenTimerHandle;
};
