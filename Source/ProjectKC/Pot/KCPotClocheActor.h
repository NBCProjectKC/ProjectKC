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
	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

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
	void ApplyOpeningState();
	void FinishOpening();

	UFUNCTION()
	void OnRep_OpenStartedServerTime();
	UFUNCTION()
	void OnRep_OpeningFinished();

	/** 음수면 아직 열리지 않았고, 그 외에는 서버 기준 열림 시작 시각이다. */
	UPROPERTY(ReplicatedUsing = OnRep_OpenStartedServerTime)
	float OpenStartedServerTime = -1.0f;

	UPROPERTY(ReplicatedUsing = OnRep_OpeningFinished)
	bool bOpeningFinished = false;

	FVector ClosedLocation = FVector::ZeroVector;
	FVector OpenLocation = FVector::ZeroVector;
	float LiftElapsedTime = 0.0f;
	bool bIsOpening = false;
	FTimerHandle OpenTimerHandle;
};
