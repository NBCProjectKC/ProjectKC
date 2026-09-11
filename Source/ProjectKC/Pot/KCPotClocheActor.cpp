#include "ProjectKC/Pot/KCPotClocheActor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "ProjectKC/GameSystem/KCGameState.h"
#include "TimerManager.h"

AKCPotClocheActor::AKCPotClocheActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = true;
	bAlwaysRelevant = true;

	ClocheMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ClocheMesh"));
	ClocheMesh->SetMobility(EComponentMobility::Movable);
	SetRootComponent(ClocheMesh);
}

void AKCPotClocheActor::BeginPlay()
{
	Super::BeginPlay();
	ClosedLocation = GetActorLocation();
	OpenLocation = ClosedLocation + FVector::UpVector * LiftHeight;

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

void AKCPotClocheActor::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AKCPotClocheActor, OpenStartedServerTime);
	DOREPLIFETIME(AKCPotClocheActor, bOpeningFinished);
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
		OpenStartedServerTime = GameState->GetServerWorldTimeSeconds();
		ApplyOpeningState();
		ForceNetUpdate();
	}
}

void AKCPotClocheActor::OnRep_OpenStartedServerTime()
{
	ApplyOpeningState();
}

void AKCPotClocheActor::OnRep_OpeningFinished()
{
	if (bOpeningFinished)
	{
		FinishOpening();
	}
}

void AKCPotClocheActor::ApplyOpeningState()
{
	if (bOpeningFinished)
	{
		FinishOpening();
		return;
	}

	if (OpenStartedServerTime < 0.0f)
	{
		return;
	}

	const AKCGameState* GameState = GetWorld()->GetGameState<AKCGameState>();
	if (!GameState)
	{
		return;
	}

	LiftElapsedTime = FMath::Max(
		0.0f,
		GameState->GetServerWorldTimeSeconds() - OpenStartedServerTime);
	if (LiftElapsedTime >= LiftDuration)
	{
		FinishOpening();
		return;
	}

	SetActorLocation(FMath::Lerp(
		ClosedLocation,
		OpenLocation,
		LiftElapsedTime / LiftDuration));
	bIsOpening = true;
	SetActorTickEnabled(true);
}

void AKCPotClocheActor::FinishOpening()
{
	const bool bShouldBroadcast = HasAuthority() && !bOpeningFinished;
	if (bShouldBroadcast)
	{
		bOpeningFinished = true;
		ForceNetUpdate();
	}

	bIsOpening = false;
	SetActorLocation(OpenLocation);
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);

	if (bShouldBroadcast)
	{
		OnOpeningFinished.Broadcast();
	}
}
