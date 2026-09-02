#include "Customization/KCCustomizationWorldSubsystem.h"

#include "Components/InputComponent.h"
#include "Customization/KCCustomizationSaveSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Painting/RuntimeMeshPaintTargetComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogKCCustomizationWorld, Log, All);

namespace
{
	const FName CustomizationPaintTargetName(TEXT("PaintTarget_Customization"));
}

bool UKCCustomizationWorldSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UKCCustomizationWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	// UWorldSubsystem callbacks happen before actors receive BeginPlay. Wait for the
	// world's public BeginPlay broadcast so the painting component cannot overwrite a load.
	WorldBeginPlayHandle = InWorld.OnWorldBeginPlay.AddUObject(
		this,
		&ThisClass::HandleActorsBegunPlay);
}

void UKCCustomizationWorldSubsystem::OnWorldEndPlay(UWorld& InWorld)
{
	if (WorldBeginPlayHandle.IsValid())
	{
		InWorld.OnWorldBeginPlay.Remove(WorldBeginPlayHandle);
		WorldBeginPlayHandle.Reset();
	}

	if (IsValid(PaintTarget))
	{
		PaintTarget->OnPaintApplied.RemoveDynamic(this, &ThisClass::HandlePaintApplied);
		if (AActor* Owner = PaintTarget->GetOwner(); Owner && Owner->InputComponent)
		{
			Owner->InputComponent->ClearBindingsForObject(this);
		}
	}

	PaintTarget = nullptr;
	Super::OnWorldEndPlay(InWorld);
}

void UKCCustomizationWorldSubsystem::HandleActorsBegunPlay()
{
	if (!ResolvePaintTarget())
	{
		// Most project levels do not contain a customization stand; this is expected.
		return;
	}

	PaintTarget->OnPaintApplied.AddUniqueDynamic(this, &ThisClass::HandlePaintApplied);
	const bool bLoaded = LoadCustomization();
	RegisterDevelopmentHotkeys();

	UE_LOG(LogKCCustomizationWorld, Log,
		TEXT("Customization runtime connected: Target=%s, Load=%s, Result=%s"),
		*GetNameSafe(PaintTarget),
		bLoaded ? TEXT("Success") : TEXT("Failed"),
		*StaticEnum<EKCCustomizationSaveResult>()->GetNameStringByValue(static_cast<int64>(LastResult)));
}

bool UKCCustomizationWorldSubsystem::SaveCustomization()
{
	UKCCustomizationSaveSubsystem* SaveSubsystem = GetSaveSubsystem();
	if (!SaveSubsystem || !ResolvePaintTarget())
	{
		LastResult = EKCCustomizationSaveResult::InvalidPaintTarget;
		OnSaveCompleted.Broadcast(false, LastResult);
		return false;
	}

	const bool bSucceeded = SaveSubsystem->SaveCustomization(
		PaintTarget,
		bUseDefaultAppearance,
		LastResult);
	OnSaveCompleted.Broadcast(bSucceeded, LastResult);
	return bSucceeded;
}

bool UKCCustomizationWorldSubsystem::LoadCustomization()
{
	UKCCustomizationSaveSubsystem* SaveSubsystem = GetSaveSubsystem();
	if (!SaveSubsystem || !ResolvePaintTarget())
	{
		LastResult = EKCCustomizationSaveResult::InvalidPaintTarget;
		OnLoadCompleted.Broadcast(false, LastResult);
		return false;
	}

	const bool bSucceeded = SaveSubsystem->LoadCustomization(
		PaintTarget,
		bLastLoadFoundSave,
		bUseDefaultAppearance,
		LastResult);
	OnLoadCompleted.Broadcast(bSucceeded, LastResult);
	return bSucceeded;
}

bool UKCCustomizationWorldSubsystem::ResetCustomization()
{
	UKCCustomizationSaveSubsystem* SaveSubsystem = GetSaveSubsystem();
	if (!SaveSubsystem || !ResolvePaintTarget())
	{
		LastResult = EKCCustomizationSaveResult::InvalidPaintTarget;
		OnResetCompleted.Broadcast(false, LastResult);
		return false;
	}

	const bool bSucceeded = SaveSubsystem->ResetCustomization(PaintTarget, LastResult);
	if (bSucceeded)
	{
		bUseDefaultAppearance = true;
	}
	OnResetCompleted.Broadcast(bSucceeded, LastResult);
	return bSucceeded;
}

bool UKCCustomizationWorldSubsystem::ResolvePaintTarget()
{
	if (IsValid(PaintTarget))
	{
		return true;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	TArray<URuntimeMeshPaintTargetComponent*> Candidates;
	for (TActorIterator<AActor> ActorIterator(World); ActorIterator; ++ActorIterator)
	{
		TArray<URuntimeMeshPaintTargetComponent*> ActorTargets;
		ActorIterator->GetComponents(ActorTargets);
		for (URuntimeMeshPaintTargetComponent* Candidate : ActorTargets)
		{
			if (!IsValid(Candidate))
			{
				continue;
			}

			if (Candidate->GetFName() == CustomizationPaintTargetName)
			{
				PaintTarget = Candidate;
				return true;
			}
			Candidates.Add(Candidate);
		}
	}

	if (Candidates.Num() == 1)
	{
		PaintTarget = Candidates[0];
		UE_LOG(LogKCCustomizationWorld, Warning,
			TEXT("Named customization paint target was not found; using '%s'."),
			*GetNameSafe(PaintTarget));
		return true;
	}

	return false;
}

UKCCustomizationSaveSubsystem* UKCCustomizationWorldSubsystem::GetSaveSubsystem() const
{
	UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UKCCustomizationSaveSubsystem>() : nullptr;
}

void UKCCustomizationWorldSubsystem::HandlePaintApplied(FRuntimeMeshPaintSampleResult PaintResult)
{
	bUseDefaultAppearance = false;
}

void UKCCustomizationWorldSubsystem::RegisterDevelopmentHotkeys()
{
#if !UE_BUILD_SHIPPING
	AActor* Owner = IsValid(PaintTarget) ? PaintTarget->GetOwner() : nullptr;
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!Owner || !PlayerController)
	{
		return;
	}

	Owner->EnableInput(PlayerController);
	UInputComponent* InputComponent = Owner->InputComponent;
	if (!InputComponent)
	{
		return;
	}

	FInputKeyBinding& SaveBinding = InputComponent->BindKey(
		EKeys::P, IE_Pressed, this, &ThisClass::HandleSaveHotkey);
	SaveBinding.bConsumeInput = false;

	FInputKeyBinding& LoadBinding = InputComponent->BindKey(
		EKeys::O, IE_Pressed, this, &ThisClass::HandleLoadHotkey);
	LoadBinding.bConsumeInput = false;

	FInputKeyBinding& ResetBinding = InputComponent->BindKey(
		EKeys::R, IE_Pressed, this, &ThisClass::HandleResetHotkey);
	ResetBinding.bConsumeInput = false;
#endif
}

void UKCCustomizationWorldSubsystem::ShowDevelopmentResult(
	const TCHAR* Operation,
	bool bSucceeded) const
{
#if !UE_BUILD_SHIPPING
	const FString ResultName = StaticEnum<EKCCustomizationSaveResult>()->GetNameStringByValue(
		static_cast<int64>(LastResult));
	const FString Message = FString::Printf(
		TEXT("Customization %s: %s (%s)"),
		Operation,
		bSucceeded ? TEXT("SUCCESS") : TEXT("FAILED"),
		*ResultName);

	UE_LOG(LogKCCustomizationWorld, Log, TEXT("%s"), *Message);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			3.0f,
			bSucceeded ? FColor::Green : FColor::Red,
			Message);
	}
#endif
}

void UKCCustomizationWorldSubsystem::HandleSaveHotkey()
{
	ShowDevelopmentResult(TEXT("Save"), SaveCustomization());
}

void UKCCustomizationWorldSubsystem::HandleLoadHotkey()
{
	ShowDevelopmentResult(TEXT("Load"), LoadCustomization());
}

void UKCCustomizationWorldSubsystem::HandleResetHotkey()
{
	ShowDevelopmentResult(TEXT("Reset"), ResetCustomization());
}
