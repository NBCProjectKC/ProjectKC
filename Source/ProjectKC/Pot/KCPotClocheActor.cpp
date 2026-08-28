#include "ProjectKC/Pot/KCPotClocheActor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "ProjectKC/GameSystem/KCGameState.h"
#include "TimerManager.h"

AKCPotClocheActor::AKCPotClocheActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = true;

	ClocheMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ClocheMesh"));
	ClocheMesh->SetMobility(EComponentMobility::Movable);
	SetRootComponent(ClocheMesh);
}

void AKCPotClocheActor::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(
			OpenTimerHandle,
			this,
			&AKCPotClocheActor::StartOpening,
			OpenDelay,
			false);
	}
}

void AKCPotClocheActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bIsOpening)
	{
		return;
	}

	LiftElapsedTime += DeltaSeconds;
	const float Alpha = FMath::Clamp(LiftElapsedTime / LiftDuration, 0.0f, 1.0f);
	SetActorLocation(FMath::Lerp(ClosedLocation, OpenLocation, Alpha));

	if (Alpha >= 1.0f)
	{
		FinishOpening();
	}
}

void AKCPotClocheActor::StartOpening()
{
	if (!HasAuthority())
	{
		return;
	}

	if (AKCGameState* GameState = GetWorld()->GetGameState<AKCGameState>())
	{
		GameState->SetFarmingOpen(true);
	}

	MulticastStartOpening();
}

void AKCPotClocheActor::MulticastStartOpening_Implementation()
{
	ClosedLocation = GetActorLocation();
	OpenLocation = ClosedLocation + FVector::UpVector * LiftHeight;
	LiftElapsedTime = 0.0f;
	bIsOpening = true;
	SetActorTickEnabled(true);
}

void AKCPotClocheActor::FinishOpening()
{
	bIsOpening = false;
	SetActorLocation(OpenLocation);
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);
}
