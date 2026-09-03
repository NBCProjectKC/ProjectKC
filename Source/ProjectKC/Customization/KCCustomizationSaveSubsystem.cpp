#include "Customization/KCCustomizationSaveSubsystem.h"

#include "Customization/KCCustomizationNetworkTypes.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Painting/RuntimeMeshPaintTargetComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogKCCustomizationSave, Log, All);

namespace
{
	FString MakeRuntimePaintTargetIdentity(
		const URuntimeMeshPaintTargetComponent* PaintTarget)
	{
		if (!PaintTarget)
		{
			return FString();
		}

		const AActor* Owner = PaintTarget->GetOwner();
		return Owner
			? FString::Printf(
				TEXT("%s.%s"),
				*Owner->GetName(),
				*PaintTarget->GetName())
			: PaintTarget->GetName();
	}
}

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
	KCCustomizationNetwork::NormalizePaintTargetIdentity(
		SaveData->PaintHistory,
		PaintTarget->GetName());

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

	// PaintTargetName에는 생성 당시 액터 인스턴스 이름이 포함됩니다.
	// 현재 인스턴스 이름으로 맞춰야 이후 추가 페인트가 같은 압축 그룹에
	// 합쳐지고 메시/텍스 종류별 중복 패치가 생기지 않습니다.
	KCCustomizationNetwork::NormalizePaintTargetIdentity(
		SaveData->PaintHistory,
		MakeRuntimePaintTargetIdentity(PaintTarget));

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
