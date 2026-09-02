#include "Customization/KCCustomizationNetworkTypes.h"

#include "Kismet/GameplayStatics.h"
#include "Misc/Crc.h"

namespace
{
	const TArray<FString> ExpectedMeshTargetNames = {
		TEXT("EyesPaintMesh"),
		TEXT("EyesPaintMesh_R"),
		TEXT("ApronPaintMesh"),
		TEXT("ChefHatPaintMesh")
	};
}

uint32 KCCustomizationNetwork::ComputePayloadHash(const TArray<uint8>& Payload)
{
	return Payload.IsEmpty()
		? 0
		: FCrc::MemCrc32(Payload.GetData(), Payload.Num());
}

bool KCCustomizationNetwork::SerializePayload(
	const FRuntimeMeshPaintPatchHistory& PaintHistory,
	const bool bUseDefaultAppearance,
	TArray<uint8>& OutPayload)
{
	OutPayload.Reset();
	if (!ValidateCustomizationData(PaintHistory, bUseDefaultAppearance))
	{
		return false;
	}

	UKCCustomizationSaveGame* SaveData = NewObject<UKCCustomizationSaveGame>();
	if (!SaveData)
	{
		return false;
	}

	SaveData->bUseDefaultAppearance = bUseDefaultAppearance;
	SaveData->PaintHistory = PaintHistory;
	if (!UGameplayStatics::SaveGameToMemory(SaveData, OutPayload) ||
		OutPayload.IsEmpty() ||
		OutPayload.Num() > MaxPayloadBytes)
	{
		OutPayload.Reset();
		return false;
	}

	return true;
}

bool KCCustomizationNetwork::DeserializePayload(
	const TArray<uint8>& Payload,
	FRuntimeMeshPaintPatchHistory& OutPaintHistory,
	bool& bOutUseDefaultAppearance)
{
	OutPaintHistory = FRuntimeMeshPaintPatchHistory();
	bOutUseDefaultAppearance = true;
	if (Payload.IsEmpty() || Payload.Num() > MaxPayloadBytes)
	{
		return false;
	}

	UKCCustomizationSaveGame* SaveData = Cast<UKCCustomizationSaveGame>(
		UGameplayStatics::LoadGameFromMemory(Payload));
	if (!SaveData ||
		SaveData->SaveVersion != UKCCustomizationSaveGame::CurrentSaveVersion ||
		SaveData->TargetSchemaVersion != UKCCustomizationSaveGame::CurrentTargetSchemaVersion ||
		!ValidateCustomizationData(SaveData->PaintHistory, SaveData->bUseDefaultAppearance))
	{
		return false;
	}

	OutPaintHistory = MoveTemp(SaveData->PaintHistory);
	bOutUseDefaultAppearance = SaveData->bUseDefaultAppearance;
	return true;
}

bool KCCustomizationNetwork::ValidateCustomizationData(
	const FRuntimeMeshPaintPatchHistory& PaintHistory,
	const bool bUseDefaultAppearance)
{
	if (bUseDefaultAppearance)
	{
		return PaintHistory.IsEmpty();
	}

	if (PaintHistory.Version != 1 ||
		PaintHistory.Entries.IsEmpty() ||
		PaintHistory.Entries.Num() > MaxPatchEntries)
	{
		return false;
	}

	TSet<int32> SeenMeshTargetIndices;
	for (const FRuntimeMeshPaintPatchHistoryEntry& Entry : PaintHistory.Entries)
	{
		if (!Entry.IsValidPatch() ||
			Entry.MeshTargetIndex < 0 ||
			Entry.MeshTargetIndex >= ExpectedMeshTargetNames.Num() ||
			Entry.MeshTargetName != ExpectedMeshTargetNames[Entry.MeshTargetIndex] ||
			Entry.TextureType != ERuntimeMeshPaintPatchTextureType::Color ||
			Entry.RTWidth != ExpectedRenderTargetSize ||
			Entry.RTHeight != ExpectedRenderTargetSize ||
			Entry.RTFormat != RTF_RGBA16f ||
			Entry.UVChannel != 0 ||
			Entry.X < 0 ||
			Entry.Y < 0 ||
			Entry.X + Entry.Width > Entry.RTWidth ||
			Entry.Y + Entry.Height > Entry.RTHeight ||
			SeenMeshTargetIndices.Contains(Entry.MeshTargetIndex))
		{
			return false;
		}

		SeenMeshTargetIndices.Add(Entry.MeshTargetIndex);
	}

	return true;
}
