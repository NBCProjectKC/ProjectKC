#include "Player/KCCapsulePlayerCharacter.h"

#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	void ConfigureAvatarPart(UStaticMeshComponent* Component, UStaticMesh* Mesh)
	{
		if (!Component)
		{
			return;
		}

		Component->SetStaticMesh(Mesh);
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Component->SetGenerateOverlapEvents(false);
		Component->SetCanEverAffectNavigation(false);
		Component->SetReceivesDecals(false);
	}
}

AKCCapsulePlayerCharacter::AKCCapsulePlayerCharacter()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshFinder(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	UStaticMesh* SphereMesh = SphereMeshFinder.Succeeded()
		? SphereMeshFinder.Object
		: nullptr;

	AvatarBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AvatarBody"));
	AvatarBody->SetupAttachment(GetCapsuleComponent());
	AvatarBody->SetRelativeLocation(FVector::ZeroVector);
	AvatarBody->SetRelativeScale3D(FVector(0.70f, 0.70f, 1.25f));
	ConfigureAvatarPart(AvatarBody, SphereMesh);

	AvatarHead = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AvatarHead"));
	AvatarHead->SetupAttachment(GetMesh(), TEXT("head"));
	AvatarHead->SetRelativeScale3D(FVector(0.40f));
	ConfigureAvatarPart(AvatarHead, SphereMesh);

	AvatarHandLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AvatarHandLeft"));
	AvatarHandLeft->SetupAttachment(GetMesh(), TEXT("hand_l"));
	AvatarHandLeft->SetRelativeLocation(FVector(0.0f, 30.0f, 0.0f));
	AvatarHandLeft->SetRelativeScale3D(FVector(0.26f));
	ConfigureAvatarPart(AvatarHandLeft, SphereMesh);

	AvatarHandRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AvatarHandRight"));
	AvatarHandRight->SetupAttachment(GetMesh(), TEXT("hand_r"));
	AvatarHandRight->SetRelativeLocation(FVector(0.0f, -25.0f, 0.0f));
	AvatarHandRight->SetRelativeScale3D(FVector(0.26f));
	ConfigureAvatarPart(AvatarHandRight, SphereMesh);

	AvatarFootLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AvatarFootLeft"));
	AvatarFootLeft->SetupAttachment(GetMesh(), TEXT("foot_l"));
	AvatarFootLeft->SetRelativeLocation(FVector(0.0f, -12.0f, -14.0f));
	AvatarFootLeft->SetRelativeScale3D(FVector(0.30f));
	ConfigureAvatarPart(AvatarFootLeft, SphereMesh);

	AvatarFootRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AvatarFootRight"));
	AvatarFootRight->SetupAttachment(GetMesh(), TEXT("foot_r"));
	AvatarFootRight->SetRelativeLocation(FVector(0.0f, -28.0f, -2.0f));
	AvatarFootRight->SetRelativeScale3D(FVector(0.30f));
	ConfigureAvatarPart(AvatarFootRight, SphereMesh);

	FaceAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("FaceAnchor"));
	FaceAnchor->SetupAttachment(AvatarHead);
	FaceAnchor->SetRelativeLocation(FVector(22.0f, 0.0f, 0.0f));

	ConfigureDriverMesh();
}

void AKCCapsulePlayerCharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ConfigureDriverMesh();
}

void AKCCapsulePlayerCharacter::ConfigureDriverMesh()
{
	USkeletalMeshComponent* DriverMesh = GetMesh();
	if (!DriverMesh)
	{
		return;
	}

	DriverMesh->VisibilityBasedAnimTickOption =
		EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	DriverMesh->SetVisibility(false, false);
	DriverMesh->SetHiddenInGame(true, false);
	DriverMesh->SetCastShadow(false);
}
