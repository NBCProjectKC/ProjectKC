#include "ProjectKC/Item/Spawner/KCItemSpawnPoint.h"

#include "Components/ArrowComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "ProjectKC/Item/Definition/KCItemDefinition.h"

AKCItemSpawnPoint::AKCItemSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;
	PreviewSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PreviewSphere"));
	SetRootComponent(PreviewSphere);
	PreviewSphere->InitSphereRadius(ClearanceRadius);
	PreviewSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PreviewSphere->SetGenerateOverlapEvents(false);
	PreviewSphere->SetHiddenInGame(true);
	PreviewSphere->SetAbsolute(false, false, true);

#if WITH_EDITORONLY_DATA
	EditorArrow = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("EditorArrow"));
	if (EditorArrow)
	{
		EditorArrow->SetupAttachment(PreviewSphere);
	}
#endif
}

void AKCItemSpawnPoint::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	PreviewSphere->SetSphereRadius(FMath::Max(1.f, ClearanceRadius));
}

bool AKCItemSpawnPoint::IsAvailable(const UKCItemDefinition& Definition) const
{
	return GetWorld() && !GetWorld()->OverlapBlockingTestByProfile(
		GetActorLocation(), FQuat::Identity,
		Definition.Presentation.WorldCollisionProfile,
		FCollisionShape::MakeSphere(FMath::Max(1.f, ClearanceRadius)),
		FCollisionQueryParams(SCENE_QUERY_STAT(KCItemSpawnPoint), false, this));
}

FTransform AKCItemSpawnPoint::GetSpawnTransform() const
{
	return FTransform(GetActorQuat(), GetActorLocation(), FVector::OneVector);
}
