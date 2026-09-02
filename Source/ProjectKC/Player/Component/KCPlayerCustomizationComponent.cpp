#include "Player/Component/KCPlayerCustomizationComponent.h"

#include "Components/MeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Customization/KCCustomizationNetworkTypes.h"
#include "Customization/KCCustomizationSaveSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Painting/RuntimeMeshPaintTargetComponent.h"
#include "Player/KCPlayerController.h"
#include "Player/KCPlayerState.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogKCPlayerCustomization, Log, All);

namespace
{
	const FName AvatarBodyName(TEXT("AvatarBody"));
	const FName LegacyEyeName(TEXT("Eyes_Basic_Male_01"));
	const FName PaintedColorParameterName(TEXT("PaintedColorTexture"));
	constexpr int32 CustomizationRenderTargetSize = 512;
}

UKCPlayerCustomizationComponent::UKCPlayerCustomizationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> EyeMeshFinder(
		TEXT("/Game/KC/Player/Customization/Meshes/SM_EyeWhite_LowPoly.SM_EyeWhite_LowPoly"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ApronMeshFinder(
		TEXT("/Game/KC/Player/Customization/Meshes/SM_Apron_LowPoly.SM_Apron_LowPoly"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ChefHatMeshFinder(
		TEXT("/Game/KC/Player/Customization/Meshes/SM_ChefHat_LowPoly.SM_ChefHat_LowPoly"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PaintMaterialFinder(
		TEXT("/Game/KC/Player/Customization/Materials/M_CustomizationPaintBase.M_CustomizationPaintBase"));

	EyeMesh = EyeMeshFinder.Object;
	ApronMesh = ApronMeshFinder.Object;
	ChefHatMesh = ChefHatMeshFinder.Object;
	PaintMaterial = PaintMaterialFinder.Object;

	LeftEyeTransform = FTransform(
		FRotator::ZeroRotator,
		FVector(50.961315f, -18.038551f, 56.034377f),
		FVector(0.18f, 0.32f, 0.42f));
	RightEyeTransform = FTransform(
		FRotator::ZeroRotator,
		FVector(50.961315f, 17.961449f, 56.034377f),
		FVector(0.18f, 0.32f, 0.42f));
	ApronTransform = FTransform(
		FRotator::ZeroRotator,
		FVector(0.187529f, 0.087705f, -52.033743f),
		FVector::OneVector);
	ChefHatTransform = FTransform(
		FRotator(19.998790f, 0.036939f, -0.194630f),
		FVector(27.539447f, -0.048058f, -21.737259f),
		FVector::OneVector);
}

void UKCPlayerCustomizationComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializeForPawn();
}

void UKCPlayerCustomizationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindCustomizationPlayerState();
	DestroyRuntimeAppearance();
	Super::EndPlay(EndPlayReason);
}

void UKCPlayerCustomizationComponent::InitializeForPawn()
{
	if (!CreateRuntimeAppearance())
	{
		LastApplyResult = EKCCustomizationSaveResult::InvalidPaintTarget;
		return;
	}

	BindCustomizationPlayerState();

	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn && OwnerPawn->IsLocallyControlled() && !bLocalSaveApplied)
	{
		ApplyLocalSavedCustomization();
	}
	else if (OwnerPawn && OwnerPawn->IsLocallyControlled() && bLocalSaveApplied)
	{
		TryUploadLocalCustomization();
	}
}

bool UKCPlayerCustomizationComponent::ApplyLocalSavedCustomization()
{
	if (!CreateRuntimeAppearance())
	{
		LastApplyResult = EKCCustomizationSaveResult::InvalidPaintTarget;
		return false;
	}

	const UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	UKCCustomizationSaveSubsystem* SaveSubsystem = GameInstance
		? GameInstance->GetSubsystem<UKCCustomizationSaveSubsystem>()
		: nullptr;
	if (!SaveSubsystem)
	{
		LastApplyResult = EKCCustomizationSaveResult::LoadFailed;
		return false;
	}

	bool bSaveFound = false;
	bool bUseDefaultAppearance = true;
	const bool bSucceeded = SaveSubsystem->LoadCustomization(
		RuntimePaintTarget,
		bSaveFound,
		bUseDefaultAppearance,
		LastApplyResult);
	bLocalSaveApplied = bSucceeded;
	if (bSucceeded)
	{
		bCurrentUseDefaultAppearance = bUseDefaultAppearance;
		TryUploadLocalCustomization();
	}

	UE_LOG(LogKCPlayerCustomization, Log,
		TEXT("Local customization apply: Owner=%s, Success=%s, SaveFound=%s, Default=%s, Result=%s"),
		*GetNameSafe(GetOwner()),
		bSucceeded ? TEXT("true") : TEXT("false"),
		bSaveFound ? TEXT("true") : TEXT("false"),
		bUseDefaultAppearance ? TEXT("true") : TEXT("false"),
		*StaticEnum<EKCCustomizationSaveResult>()->GetNameStringByValue(
			static_cast<int64>(LastApplyResult)));
	return bSucceeded;
}

bool UKCPlayerCustomizationComponent::ApplyCustomizationData(
	const FRuntimeMeshPaintPatchHistory& PaintHistory,
	const bool bUseDefaultAppearance)
{
	if (!CreateRuntimeAppearance())
	{
		LastApplyResult = EKCCustomizationSaveResult::InvalidPaintTarget;
		return false;
	}

	if (bUseDefaultAppearance)
	{
		RuntimePaintTarget->ClearPaintPatchHistory();
		const bool bReset = RuntimePaintTarget->InitializeRuntimePaintTarget();
		LastApplyResult = bReset
			? EKCCustomizationSaveResult::Success
			: EKCCustomizationSaveResult::ApplyFailed;
		if (bReset)
		{
			bCurrentUseDefaultAppearance = true;
		}
		return bReset;
	}

	const bool bApplied = RuntimePaintTarget->ImportPaintPatchHistory(
		PaintHistory,
		true,
		true);
	LastApplyResult = bApplied
		? EKCCustomizationSaveResult::Success
		: EKCCustomizationSaveResult::ApplyFailed;
	if (bApplied)
	{
		bCurrentUseDefaultAppearance = false;
	}
	return bApplied;
}

bool UKCPlayerCustomizationComponent::ApplyNetworkCustomizationData(
	const FRuntimeMeshPaintPatchHistory& PaintHistory,
	const bool bUseDefaultAppearance,
	const FKCCustomizationDescriptor& Descriptor)
{
	if (!Descriptor.IsPublished() ||
		Descriptor.TargetSchemaVersion != UKCCustomizationSaveGame::CurrentTargetSchemaVersion ||
		Descriptor.bUseDefaultAppearance != bUseDefaultAppearance ||
		!ApplyCustomizationData(PaintHistory, bUseDefaultAppearance))
	{
		return false;
	}

	AppliedCustomizationRevision = Descriptor.Revision;
	AppliedCustomizationHash = Descriptor.ContentHash;
	return true;
}

bool UKCPlayerCustomizationComponent::IsRuntimeAppearanceReady() const
{
	return IsValid(EyesPaintMesh) &&
		IsValid(EyesPaintMesh_R) &&
		IsValid(ApronPaintMesh) &&
		IsValid(ChefHatPaintMesh) &&
		IsValid(RuntimePaintTarget);
}

bool UKCPlayerCustomizationComponent::CreateRuntimeAppearance()
{
	if (IsRuntimeAppearanceReady())
	{
		return true;
	}

	AActor* Owner = GetOwner();
	UStaticMeshComponent* AvatarBody = FindAvatarBody();
	if (!Owner || !AvatarBody || !EyeMesh || !ApronMesh || !ChefHatMesh || !PaintMaterial)
	{
		UE_LOG(LogKCPlayerCustomization, Error,
			TEXT("Unable to create customization appearance for '%s': Body=%s Eye=%s Apron=%s Hat=%s Material=%s"),
			*GetNameSafe(Owner),
			*GetNameSafe(AvatarBody),
			*GetNameSafe(EyeMesh),
			*GetNameSafe(ApronMesh),
			*GetNameSafe(ChefHatMesh),
			*GetNameSafe(PaintMaterial));
		return false;
	}

	EyesPaintMesh = CreatePaintMeshComponent(
		TEXT("EyesPaintMesh"), EyeMesh, LeftEyeTransform, AvatarBody);
	EyesPaintMesh_R = CreatePaintMeshComponent(
		TEXT("EyesPaintMesh_R"), EyeMesh, RightEyeTransform, AvatarBody);
	ApronPaintMesh = CreatePaintMeshComponent(
		TEXT("ApronPaintMesh"), ApronMesh, ApronTransform, AvatarBody);
	ChefHatPaintMesh = CreatePaintMeshComponent(
		TEXT("ChefHatPaintMesh"), ChefHatMesh, ChefHatTransform, AvatarBody);
	if (!EyesPaintMesh || !EyesPaintMesh_R || !ApronPaintMesh || !ChefHatPaintMesh)
	{
		DestroyRuntimeAppearance();
		return false;
	}

	RuntimePaintTarget = NewObject<URuntimeMeshPaintTargetComponent>(
		Owner,
		TEXT("PaintTarget_PlayerCustomization"));
	if (!RuntimePaintTarget)
	{
		DestroyRuntimeAppearance();
		return false;
	}

	RuntimePaintTarget->RuntimeRenderTargetWidth = CustomizationRenderTargetSize;
	RuntimePaintTarget->RuntimeRenderTargetHeight = CustomizationRenderTargetSize;
	RuntimePaintTarget->RuntimeRenderTargetFormat = RTF_RGBA16f;
	RuntimePaintTarget->PaintedColorTextureParameterName = PaintedColorParameterName;
	RuntimePaintTarget->bCreatePaintedMaterialSettingsRenderTarget = false;
	RuntimePaintTarget->bRecordPaintPatchHistory = false;
	RuntimePaintTarget->bReplicateRuntimePaint = false;
	Owner->AddInstanceComponent(RuntimePaintTarget);
	RuntimePaintTarget->RegisterComponent();

	TArray<UMeshComponent*> PaintMeshes;
	PaintMeshes.Reserve(4);
	PaintMeshes.Add(EyesPaintMesh);
	PaintMeshes.Add(EyesPaintMesh_R);
	PaintMeshes.Add(ApronPaintMesh);
	PaintMeshes.Add(ChefHatPaintMesh);
	RuntimePaintTarget->SetMeshTargets(PaintMeshes);

	HideLegacyEyeMesh();
	return IsRuntimeAppearanceReady();
}

UStaticMeshComponent* UKCPlayerCustomizationComponent::CreatePaintMeshComponent(
	const FName ComponentName,
	UStaticMesh* Mesh,
	const FTransform& RelativeTransform,
	UStaticMeshComponent* AttachParent)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Mesh || !AttachParent)
	{
		return nullptr;
	}

	UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(Owner, ComponentName);
	if (!Component)
	{
		return nullptr;
	}

	Component->SetStaticMesh(Mesh);
	Component->SetMaterial(0, PaintMaterial);
	Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Component->SetGenerateOverlapEvents(false);
	Component->SetCanEverAffectNavigation(false);
	Component->SetReceivesDecals(false);
	Component->SetupAttachment(AttachParent);
	Component->SetRelativeTransform(RelativeTransform);
	Owner->AddInstanceComponent(Component);
	Component->RegisterComponent();
	return Component;
}

UStaticMeshComponent* UKCPlayerCustomizationComponent::FindAvatarBody() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	TArray<UStaticMeshComponent*> StaticMeshComponents;
	Owner->GetComponents(StaticMeshComponents);
	for (UStaticMeshComponent* Component : StaticMeshComponents)
	{
		if (IsValid(Component) && Component->GetFName() == AvatarBodyName)
		{
			return Component;
		}
	}
	return nullptr;
}

void UKCPlayerCustomizationComponent::HideLegacyEyeMesh() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	TArray<UMeshComponent*> MeshComponents;
	Owner->GetComponents(MeshComponents);
	for (UMeshComponent* Component : MeshComponents)
	{
		if (!IsValid(Component))
		{
			continue;
		}

		FString NormalizedName = Component->GetName();
		NormalizedName.RemoveFromEnd(TEXT("_GEN_VARIABLE"));
		if (NormalizedName.Equals(LegacyEyeName.ToString(), ESearchCase::IgnoreCase))
		{
			Component->SetVisibility(false, true);
			Component->SetHiddenInGame(true, true);
		}
	}
}

void UKCPlayerCustomizationComponent::BindCustomizationPlayerState()
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	AKCPlayerState* PlayerState = OwnerPawn
		? OwnerPawn->GetPlayerState<AKCPlayerState>()
		: nullptr;
	if (BoundCustomizationPlayerState.Get() == PlayerState)
	{
		if (PlayerState)
		{
			HandleCustomizationDescriptorChanged(PlayerState->GetCustomizationDescriptor());
		}
		return;
	}

	UnbindCustomizationPlayerState();
	BoundCustomizationPlayerState = PlayerState;
	if (PlayerState)
	{
		PlayerState->OnCustomizationDescriptorChanged.AddUObject(
			this,
			&ThisClass::HandleCustomizationDescriptorChanged);
		HandleCustomizationDescriptorChanged(PlayerState->GetCustomizationDescriptor());
	}
}

void UKCPlayerCustomizationComponent::UnbindCustomizationPlayerState()
{
	if (AKCPlayerState* PlayerState = BoundCustomizationPlayerState.Get())
	{
		PlayerState->OnCustomizationDescriptorChanged.RemoveAll(this);
	}
	BoundCustomizationPlayerState.Reset();
}

void UKCPlayerCustomizationComponent::TryUploadLocalCustomization()
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn || !OwnerPawn->IsLocallyControlled() || !bLocalSaveApplied)
	{
		return;
	}

	AKCPlayerController* PlayerController = Cast<AKCPlayerController>(OwnerPawn->GetController());
	if (!PlayerController)
	{
		return;
	}

	FRuntimeMeshPaintPatchHistory PaintHistory;
	if (!bCurrentUseDefaultAppearance &&
		(!RuntimePaintTarget || !RuntimePaintTarget->CompactPaintPatchHistory(PaintHistory)))
	{
		return;
	}

	TArray<uint8> Payload;
	if (!KCCustomizationNetwork::SerializePayload(
		PaintHistory,
		bCurrentUseDefaultAppearance,
		Payload))
	{
		return;
	}

	AppliedCustomizationHash = KCCustomizationNetwork::ComputePayloadHash(Payload);
	PlayerController->UploadCustomizationPayload(Payload);
}

void UKCPlayerCustomizationComponent::HandleCustomizationDescriptorChanged(
	const FKCCustomizationDescriptor& Descriptor)
{
	if (!Descriptor.IsPublished() ||
		Descriptor.TargetSchemaVersion != UKCCustomizationSaveGame::CurrentTargetSchemaVersion ||
		Descriptor.Revision == AppliedCustomizationRevision)
	{
		return;
	}

	if (AppliedCustomizationHash != 0 && AppliedCustomizationHash == Descriptor.ContentHash)
	{
		AppliedCustomizationRevision = Descriptor.Revision;
		return;
	}

	if (Descriptor.bUseDefaultAppearance)
	{
		ApplyNetworkCustomizationData(
			FRuntimeMeshPaintPatchHistory(),
			true,
			Descriptor);
		return;
	}

	AKCPlayerController* LocalPlayerController = Cast<AKCPlayerController>(
		UGameplayStatics::GetPlayerController(this, 0));
	if (LocalPlayerController && BoundCustomizationPlayerState.IsValid())
	{
		LocalPlayerController->RequestCustomizationPayload(
			BoundCustomizationPlayerState.Get(),
			this);
	}
}

void UKCPlayerCustomizationComponent::DestroyRuntimeAppearance()
{
	if (RuntimePaintTarget)
	{
		RuntimePaintTarget->DestroyComponent();
		RuntimePaintTarget = nullptr;
	}

	for (TObjectPtr<UStaticMeshComponent>* Component : {
		&EyesPaintMesh,
		&EyesPaintMesh_R,
		&ApronPaintMesh,
		&ChefHatPaintMesh })
	{
		if (Component->Get())
		{
			Component->Get()->DestroyComponent();
			*Component = nullptr;
		}
	}
}
