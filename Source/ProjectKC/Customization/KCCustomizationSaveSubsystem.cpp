#include "Customization/KCCustomizationSaveSubsystem.h"

#include "Kismet/GameplayStatics.h"
#include "Painting/RuntimeMeshPaintTargetComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogKCCustomizationSave, Log, All);

const FString UKCCustomizationSaveSubsystem::CustomizationSlotName = TEXT("KC_Customization_Local");

bool UKCCustomizationSaveSubsystem::SaveCustomization(
	URuntimeMeshPaintTargetComponent* PaintTarget,
	bool bUseDefaultAppearance,
	EKCCustomizationSaveResult& OutResult)
{
	OutResult = EKCCustomizationSaveResult::InvalidPaintTarget;
	if (!IsValid(PaintTarget))
	{
		return false;
	}

	UKCCustomizationSaveGame* SaveData = Cast<UKCCustomizationSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UKCCustomizationSaveGame::StaticClass()));
	if (!SaveData)
	{
		OutResult = EKCCustomizationSaveResult::SaveFailed;
		return false;
	}

	SaveData->bUseDefaultAppearance = bUseDefaultAppearance;
	if (!bUseDefaultAppearance && !PaintTarget->CompactPaintPatchHistory(SaveData->PaintHistory))
	{
		OutResult = EKCCustomizationSaveResult::NoPaintData;
		UE_LOG(LogKCCustomizationSave, Warning, TEXT("Save rejected because the paint target has no patch history."));
		return false;
	}

	if (!UGameplayStatics::SaveGameToSlot(SaveData, CustomizationSlotName, CustomizationUserIndex))
	{
		OutResult = EKCCustomizationSaveResult::SaveFailed;
		UE_LOG(LogKCCustomizationSave, Error, TEXT("Failed to save slot '%s'."), *CustomizationSlotName);
		return false;
	}

	OutResult = EKCCustomizationSaveResult::Success;
	UE_LOG(LogKCCustomizationSave, Log, TEXT("Saved local customization to slot '%s' (Default=%s, Entries=%d)."),
		*CustomizationSlotName,
		bUseDefaultAppearance ? TEXT("true") : TEXT("false"),
		SaveData->PaintHistory.Entries.Num());
	return true;
}

bool UKCCustomizationSaveSubsystem::LoadCustomization(
	URuntimeMeshPaintTargetComponent* PaintTarget,
	bool& bOutSaveFound,
	bool& bOutUseDefaultAppearance,
	EKCCustomizationSaveResult& OutResult)
{
	bOutSaveFound = false;
	bOutUseDefaultAppearance = true;
	OutResult = EKCCustomizationSaveResult::InvalidPaintTarget;
	if (!IsValid(PaintTarget))
	{
		return false;
	}

	if (!DoesCustomizationSaveExist())
	{
		const bool bResetSucceeded = ResetCustomization(PaintTarget, OutResult);
		if (bResetSucceeded)
		{
			OutResult = EKCCustomizationSaveResult::NoSaveFound;
		}
		return bResetSucceeded;
	}

	bOutSaveFound = true;
	UKCCustomizationSaveGame* SaveData = Cast<UKCCustomizationSaveGame>(
		UGameplayStatics::LoadGameFromSlot(CustomizationSlotName, CustomizationUserIndex));
	if (!SaveData)
	{
		OutResult = EKCCustomizationSaveResult::LoadFailed;
		return false;
	}

	if (SaveData->SaveVersion != UKCCustomizationSaveGame::CurrentSaveVersion ||
		SaveData->TargetSchemaVersion != UKCCustomizationSaveGame::CurrentTargetSchemaVersion)
	{
		OutResult = EKCCustomizationSaveResult::IncompatibleVersion;
		return false;
	}

	bOutUseDefaultAppearance = SaveData->bUseDefaultAppearance;
	if (bOutUseDefaultAppearance)
	{
		return ResetCustomization(PaintTarget, OutResult);
	}

	if (!PaintTarget->ImportPaintPatchHistory(SaveData->PaintHistory, true, true))
	{
		OutResult = EKCCustomizationSaveResult::ApplyFailed;
		return false;
	}

	OutResult = EKCCustomizationSaveResult::Success;
	UE_LOG(LogKCCustomizationSave, Log, TEXT("Loaded local customization from slot '%s' (%d entries)."),
		*CustomizationSlotName,
		SaveData->PaintHistory.Entries.Num());
	return true;
}

bool UKCCustomizationSaveSubsystem::ResetCustomization(
	URuntimeMeshPaintTargetComponent* PaintTarget,
	EKCCustomizationSaveResult& OutResult)
{
	OutResult = EKCCustomizationSaveResult::InvalidPaintTarget;
	if (!IsValid(PaintTarget))
	{
		return false;
	}

	PaintTarget->ClearPaintPatchHistory();
	if (!PaintTarget->InitializeRuntimePaintTarget())
	{
		OutResult = EKCCustomizationSaveResult::ApplyFailed;
		return false;
	}

	OutResult = EKCCustomizationSaveResult::Success;
	return true;
}

bool UKCCustomizationSaveSubsystem::DoesCustomizationSaveExist() const
{
	return UGameplayStatics::DoesSaveGameExist(CustomizationSlotName, CustomizationUserIndex);
}

bool UKCCustomizationSaveSubsystem::DeleteCustomizationSave(EKCCustomizationSaveResult& OutResult)
{
	if (!DoesCustomizationSaveExist())
	{
		OutResult = EKCCustomizationSaveResult::Success;
		return true;
	}

	if (!UGameplayStatics::DeleteGameInSlot(CustomizationSlotName, CustomizationUserIndex))
	{
		OutResult = EKCCustomizationSaveResult::SaveFailed;
		return false;
	}

	OutResult = EKCCustomizationSaveResult::Success;
	return true;
}

FString UKCCustomizationSaveSubsystem::GetCustomizationSlotName() const
{
	return CustomizationSlotName;
}
