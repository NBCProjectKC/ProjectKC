#include "ProjectKC/Player/Component/KCProjectileTrajectoryPreviewComponent.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "ProjectKC/AbilitySystem/Definition/KCSingleActionDefinition.h"
#include "ProjectKC/AbilitySystem/Fragment/KCThrowProjectileFragment.h"
#include "ProjectKC/Item/Definition/KCItemDefinition.h"
#include "ProjectKC/Item/KCWorldItemActor.h"
#include "UObject/ConstructorHelpers.h"

UKCProjectileTrajectoryPreviewComponent::
UKCProjectileTrajectoryPreviewComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereFinder.Succeeded())
	{
		TrajectoryPointMesh = SphereFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderFinder.Succeeded())
	{
		ImpactMarkerMesh = CylinderFinder.Object;
	}
}

bool UKCProjectileTrajectoryPreviewComponent::BeginPreview(
	AKCWorldItemActor* SourceItem)
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	const UKCThrowProjectileFragment* ThrowFragment =
		ResolvePreviewFragment(SourceItem);
	if (!OwnerPawn || !OwnerPawn->IsLocallyControlled() ||
		!IsValid(SourceItem) || !ThrowFragment ||
		SourceItem->GetHolder() != GetOwner() || !GetWorld())
	{
		EndPreview();
		return false;
	}

	ActiveSourceItem = SourceItem;
	ActiveThrowFragment = const_cast<UKCThrowProjectileFragment*>(ThrowFragment);
	ChargeStartTimeSeconds = GetWorld()->GetTimeSeconds();
	CurrentChargeAlpha = 0.0f;
	EnsureVisualizationComponents();
	SetComponentTickEnabled(true);
	UpdatePreview();
	return true;
}

void UKCProjectileTrajectoryPreviewComponent::EndPreview()
{
	SetComponentTickEnabled(false);
	ActiveSourceItem = nullptr;
	ActiveThrowFragment = nullptr;
	ChargeStartTimeSeconds = 0.0;
	CurrentChargeAlpha = 0.0f;
	ClearVisualization();
}

bool UKCProjectileTrajectoryPreviewComponent::IsPreviewActive() const
{
	return ActiveSourceItem.IsValid() && ActiveThrowFragment.IsValid();
}

float UKCProjectileTrajectoryPreviewComponent::GetCurrentChargeAlpha() const
{
	return CurrentChargeAlpha;
}

bool UKCProjectileTrajectoryPreviewComponent::HasPredictedImpact() const
{
	return bHasPredictedImpact;
}

FVector UKCProjectileTrajectoryPreviewComponent::
GetPredictedImpactLocation() const
{
	return PredictedImpactLocation;
}

void UKCProjectileTrajectoryPreviewComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdatePreview();
}

void UKCProjectileTrajectoryPreviewComponent::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	EndPreview();
	Super::EndPlay(EndPlayReason);
}

const UKCThrowProjectileFragment*
UKCProjectileTrajectoryPreviewComponent::ResolvePreviewFragment(
	const AKCWorldItemActor* SourceItem) const
{
	const UKCItemDefinition* ItemDefinition = SourceItem
		? SourceItem->GetItemDefinition()
		: nullptr;
	const UKCSingleActionDefinition* ActionDefinition =
		Cast<UKCSingleActionDefinition>(
			ItemDefinition ? ItemDefinition->UseAction : nullptr);
	const UKCThrowProjectileFragment* ThrowFragment = ActionDefinition
		? ActionDefinition->FindChargedThrowProjectileFragment()
		: nullptr;
	return ThrowFragment &&
		ThrowFragment->LaunchConfig.bShowTrajectoryPreview
			? ThrowFragment
			: nullptr;
}

void UKCProjectileTrajectoryPreviewComponent::
EnsureVisualizationComponents()
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->GetRootComponent())
	{
		return;
	}

	if (!TrajectoryPointInstances)
	{
		TrajectoryPointInstances =
			NewObject<UInstancedStaticMeshComponent>(
				OwnerActor,
				TEXT("ProjectileTrajectoryPreviewPoints"));
		OwnerActor->AddInstanceComponent(TrajectoryPointInstances);
		TrajectoryPointInstances->SetupAttachment(
			OwnerActor->GetRootComponent());
		TrajectoryPointInstances->SetCollisionEnabled(
			ECollisionEnabled::NoCollision);
		TrajectoryPointInstances->SetGenerateOverlapEvents(false);
		TrajectoryPointInstances->SetCanEverAffectNavigation(false);
		TrajectoryPointInstances->SetCastShadow(false);
		TrajectoryPointInstances->SetReceivesDecals(false);
		TrajectoryPointInstances->SetStaticMesh(TrajectoryPointMesh);
		if (TrajectoryPointMaterial)
		{
			TrajectoryPointInstances->SetMaterial(0, TrajectoryPointMaterial);
		}
		TrajectoryPointInstances->RegisterComponent();
	}

	if (!ImpactMarkerComponent)
	{
		ImpactMarkerComponent = NewObject<UStaticMeshComponent>(
			OwnerActor,
			TEXT("ProjectileTrajectoryImpactMarker"));
		OwnerActor->AddInstanceComponent(ImpactMarkerComponent);
		ImpactMarkerComponent->SetupAttachment(OwnerActor->GetRootComponent());
		ImpactMarkerComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ImpactMarkerComponent->SetGenerateOverlapEvents(false);
		ImpactMarkerComponent->SetCanEverAffectNavigation(false);
		ImpactMarkerComponent->SetCastShadow(false);
		ImpactMarkerComponent->SetReceivesDecals(false);
		ImpactMarkerComponent->SetStaticMesh(ImpactMarkerMesh);
		if (ImpactMarkerMaterial)
		{
			ImpactMarkerComponent->SetMaterial(0, ImpactMarkerMaterial);
		}
		ImpactMarkerComponent->RegisterComponent();
	}
}

void UKCProjectileTrajectoryPreviewComponent::UpdatePreview()
{
	AKCWorldItemActor* SourceItem = ActiveSourceItem.Get();
	const UKCThrowProjectileFragment* ThrowFragment =
		ActiveThrowFragment.Get();
	AActor* OwnerActor = GetOwner();
	UWorld* World = GetWorld();
	if (!SourceItem || !ThrowFragment || !OwnerActor || !World ||
		SourceItem->GetHolder() != OwnerActor)
	{
		EndPreview();
		return;
	}

	EnsureVisualizationComponents();
	if (!TrajectoryPointInstances || !ImpactMarkerComponent)
	{
		return;
	}

	const FKCProjectileLaunchConfigStruct& LaunchConfig =
		ThrowFragment->LaunchConfig;
	const float HeldDuration = static_cast<float>(FMath::Max(
		0.0,
		World->GetTimeSeconds() - ChargeStartTimeSeconds));
	CurrentChargeAlpha = LaunchConfig.CalculateChargeAlpha(HeldDuration);

	FTransform SpawnTransform;
	FVector InitialVelocity;
	if (!ThrowFragment->BuildLaunchSolution(
		OwnerActor,
		SourceItem,
		CurrentChargeAlpha,
		SpawnTransform,
		InitialVelocity))
	{
		ClearVisualization();
		return;
	}

	FPredictProjectilePathParams Params;
	Params.StartLocation = SpawnTransform.GetLocation();
	Params.LaunchVelocity = InitialVelocity;
	Params.bTraceWithCollision = true;
	Params.bTraceWithChannel = true;
	Params.bTraceComplex = false;
	Params.ProjectileRadius = LaunchConfig.CollisionRadius;
	Params.MaxSimTime = LaunchConfig.PreviewMaximumSimulationTime;
	Params.SimFrequency = LaunchConfig.PreviewSimulationFrequency;
	Params.TraceChannel = LaunchConfig.PreviewTraceChannel;
	Params.OverrideGravityZ =
		World->GetGravityZ() * LaunchConfig.GravityScale;
	Params.ActorsToIgnore.Add(OwnerActor);
	Params.ActorsToIgnore.Add(SourceItem);

	FPredictProjectilePathResult Result;
	const bool bHit = UGameplayStatics::PredictProjectilePath(
		this,
		Params,
		Result);

	TrajectoryPointInstances->ClearInstances();
	const FVector PointScale(TrajectoryPointScale);
	for (const FPredictProjectilePathPointData& Point : Result.PathData)
	{
		TrajectoryPointInstances->AddInstance(
			FTransform(FQuat::Identity, Point.Location, PointScale),
			true);
	}
	TrajectoryPointInstances->SetVisibility(
		Result.PathData.Num() > 0,
		true);

	bHasPredictedImpact = bHit && Result.HitResult.IsValidBlockingHit();
	PredictedImpactLocation = bHasPredictedImpact
		? Result.HitResult.ImpactPoint
		: FVector::ZeroVector;
	if (!bHasPredictedImpact)
	{
		ImpactMarkerComponent->SetVisibility(false, true);
		return;
	}

	const FVector ImpactNormal = Result.HitResult.ImpactNormal.GetSafeNormal(
		UE_SMALL_NUMBER,
		FVector::UpVector);
	const FQuat MarkerRotation = FQuat::FindBetweenNormals(
		FVector::UpVector,
		ImpactNormal);
	const float MarkerXYScale = ImpactMarkerRadius / 50.0f;
	ImpactMarkerComponent->SetWorldTransform(FTransform(
		MarkerRotation,
		PredictedImpactLocation + ImpactNormal * ImpactMarkerSurfaceOffset,
		FVector(MarkerXYScale, MarkerXYScale, 0.025f)));
	ImpactMarkerComponent->SetVisibility(true, true);
}

void UKCProjectileTrajectoryPreviewComponent::ClearVisualization()
{
	if (TrajectoryPointInstances)
	{
		TrajectoryPointInstances->ClearInstances();
		TrajectoryPointInstances->SetVisibility(false, true);
	}
	if (ImpactMarkerComponent)
	{
		ImpactMarkerComponent->SetVisibility(false, true);
	}
	bHasPredictedImpact = false;
	PredictedImpactLocation = FVector::ZeroVector;
}
