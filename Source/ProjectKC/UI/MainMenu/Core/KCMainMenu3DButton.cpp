#include "ProjectKC/UI/MainMenu/Core/KCMainMenu3DButton.h"

#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "InputCoreTypes.h"
#include "ProjectKC/UI/MainMenu/Core/KCMainMenu3DManager.h"
#include "Text3DComponent.h"

AKCMainMenu3DButton::AKCMainMenu3DButton()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	Text3DComponent = CreateDefaultSubobject<UText3DComponent>(TEXT("Text3D"));
	Text3DComponent->SetupAttachment(SceneRoot);

	CollisionComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision"));
	CollisionComponent->SetupAttachment(SceneRoot);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComponent->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	CollisionComponent->SetGenerateOverlapEvents(false);

	MenuText = FText::FromString(TEXT("Create Lobby"));
}

void AKCMainMenu3DButton::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ApplyTextSettings();
	ApplyVisualState();
}

void AKCMainMenu3DButton::BeginPlay()
{
	Super::BeginPlay();

	if (CollisionComponent)
	{
		CollisionComponent->OnBeginCursorOver.AddDynamic(this, &ThisClass::HandleCursorOver);
		CollisionComponent->OnEndCursorOver.AddDynamic(this, &ThisClass::HandleCursorOut);
		CollisionComponent->OnClicked.AddDynamic(this, &ThisClass::HandleClicked);
	}

	ApplyTextSettings();
	ApplyVisualState();
}

void AKCMainMenu3DButton::InitializeButton(AKCMainMenu3DManager* InOwningManager)
{
	OwningManager = InOwningManager;
}

void AKCMainMenu3DButton::SetHovered(bool bNewHovered)
{
	if (bIsHovered == bNewHovered)
	{
		return;
	}

	bIsHovered = bNewHovered;
	ApplyVisualState();
}

void AKCMainMenu3DButton::SetPressed(bool bNewPressed)
{
	if (bIsPressed == bNewPressed)
	{
		return;
	}

	bIsPressed = bNewPressed;
	ApplyVisualState();
}

void AKCMainMenu3DButton::ApplyTextSettings()
{
	if (Text3DComponent)
	{
		Text3DComponent->SetText(MenuText);
	}

	if (CollisionComponent)
	{
		CollisionComponent->SetBoxExtent(CollisionBoxExtent);
	}

	AlignCollisionToTextBounds();
}

void AKCMainMenu3DButton::AlignCollisionToTextBounds()
{
	if (!Text3DComponent || !CollisionComponent)
	{
		return;
	}

	FVector TextBoundsOrigin = FVector::ZeroVector;
	FVector TextBoundsExtent = FVector::ZeroVector;
	Text3DComponent->GetBounds(TextBoundsOrigin, TextBoundsExtent);

	if (TextBoundsExtent.IsNearlyZero())
	{
		CollisionComponent->SetRelativeLocation(CollisionRelativeOffset);
		return;
	}

	const FTransform TextRelativeTransformWithoutScale(
		Text3DComponent->GetRelativeRotation(),
		Text3DComponent->GetRelativeLocation(),
		FVector::OneVector);
	const FVector CollisionRelativeLocation =
		TextRelativeTransformWithoutScale.TransformPosition(TextBoundsOrigin) + CollisionRelativeOffset;

	CollisionComponent->SetRelativeLocation(CollisionRelativeLocation);
}

void AKCMainMenu3DButton::ApplyVisualState()
{
	UMaterialInterface* TargetMaterial = NormalMaterial;
	FVector TargetScale = NormalScale;

	if (bIsPressed)
	{
		TargetMaterial = PressedMaterial ? PressedMaterial.Get() : TargetMaterial;
		TargetScale = PressedScale;
	}
	else if (bIsHovered)
	{
		TargetMaterial = HoverMaterial ? HoverMaterial.Get() : TargetMaterial;
		TargetScale = HoverScale;
	}

	ApplyMaterial(TargetMaterial);
	if (Text3DComponent)
	{
		Text3DComponent->SetRelativeScale3D(TargetScale);
	}
}

void AKCMainMenu3DButton::ApplyMaterial(UMaterialInterface* Material)
{
	if (!Text3DComponent || !Material)
	{
		return;
	}

	Text3DComponent->SetFrontMaterial(Material);
	Text3DComponent->SetBevelMaterial(Material);
	Text3DComponent->SetExtrudeMaterial(Material);
	Text3DComponent->SetBackMaterial(Material);
}

void AKCMainMenu3DButton::HandleCursorOver(UPrimitiveComponent* TouchedComponent)
{
	SetHovered(true);
	if (OwningManager)
	{
		OwningManager->NotifyButtonHovered(this);
	}
}

void AKCMainMenu3DButton::HandleCursorOut(UPrimitiveComponent* TouchedComponent)
{
	SetHovered(false);
	if (OwningManager)
	{
		OwningManager->NotifyButtonUnhovered(this);
	}
}

void AKCMainMenu3DButton::HandleClicked(UPrimitiveComponent* TouchedComponent, FKey ButtonPressed)
{
	SetPressed(true);
	if (OwningManager)
	{
		OwningManager->HandleButtonClicked(this);
	}
	SetPressed(false);
}
