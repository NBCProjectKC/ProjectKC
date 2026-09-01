// Copyright Shared Orbit 2026. All Rights Reserved.
#include "Painting/RuntimeMeshPaintReplicationTypes.h"

#include "Engine/PackageMapClient.h"
#include "Painting/RuntimeMeshPaintTargetComponent.h"

namespace
{
	constexpr float RuntimeMeshPaintNetBrushSizeScale = 4096.0f;

	uint8 QuantizeUnitFloat(float Value)
	{
		return static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(Value * 255.0f), 0, 255));
	}

	float DequantizeUnitFloat(uint8 Value)
	{
		return Value / 255.0f;
	}

	uint16 QuantizeBrushSize(float BrushSize)
	{
		return static_cast<uint16>(FMath::Clamp(
			FMath::RoundToInt(FMath::Max(0.001f, BrushSize) * RuntimeMeshPaintNetBrushSizeScale),
			1,
			MAX_uint16));
	}
}

bool FRuntimeMeshPaintNetCommand::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	bOutSuccess = true;

	UObject* TargetObject = TargetComponent.Get();
	UObject* SourceObject = SourceActor.Get();
	if (Map)
	{
		bOutSuccess &= Map->SerializeObject(Ar, UActorComponent::StaticClass(), TargetObject);
		bOutSuccess &= Map->SerializeObject(Ar, AActor::StaticClass(), SourceObject);
	}
	else
	{
		Ar << TargetObject;
		Ar << SourceObject;
	}

	if (Ar.IsLoading())
	{
		TargetComponent = Cast<UActorComponent>(TargetObject);
		SourceActor = Cast<AActor>(SourceObject);
	}

	Ar.SerializeIntPacked(CommandId);
	Ar.SerializeIntPacked(ClientPredictionKey);
	Ar << StrokeId;
	Ar << MeshTargetIndex;
	Ar << LOD;
	Ar << UVChannel;
	Ar << BrushUVX;
	Ar << BrushUVY;

	bool bVectorSuccess = true;
	BrushRayStart.NetSerialize(Ar, Map, bVectorSuccess);
	bOutSuccess &= bVectorSuccess;
	BrushRayEnd.NetSerialize(Ar, Map, bVectorSuccess);
	bOutSuccess &= bVectorSuccess;
	BrushWorldCenter.NetSerialize(Ar, Map, bVectorSuccess);
	bOutSuccess &= bVectorSuccess;
	BrushWorldNormal.NetSerialize(Ar, Map, bVectorSuccess);
	bOutSuccess &= bVectorSuccess;

	Ar << BrushSize;
	Ar << BrushColor;
	Ar << BrushOpacity;
	Ar << BrushHardness;
	Ar << BrushMetallic;
	Ar << BrushRoughness;
	Ar << CommandFlags;

	if ((CommandFlags & static_cast<uint8>(ERuntimeMeshPaintNetCommandFlags::ScreenProjection)) != 0)
	{
		if (Ar.IsLoading())
		{
			ProjectionData = FRuntimeMeshPaintScreenProjectionData();
			ProjectionData.bHasScreenProjection = true;
		}

		FVector_NetQuantize10 ViewOrigin = ProjectionData.ViewOrigin;
		ViewOrigin.NetSerialize(Ar, Map, bVectorSuccess);
		bOutSuccess &= bVectorSuccess;
		if (Ar.IsLoading())
		{
			ProjectionData.ViewOrigin = FVector(ViewOrigin);
		}

		FVector_NetQuantizeNormal ViewForward = ProjectionData.ViewForward;
		FVector_NetQuantizeNormal ViewRight = ProjectionData.ViewRight;
		FVector_NetQuantizeNormal ViewUp = ProjectionData.ViewUp;
		ViewForward.NetSerialize(Ar, Map, bVectorSuccess);
		bOutSuccess &= bVectorSuccess;
		ViewRight.NetSerialize(Ar, Map, bVectorSuccess);
		bOutSuccess &= bVectorSuccess;
		ViewUp.NetSerialize(Ar, Map, bVectorSuccess);
		bOutSuccess &= bVectorSuccess;
		if (Ar.IsLoading())
		{
			ProjectionData.ViewForward = FVector(ViewForward).GetSafeNormal(SMALL_NUMBER, FVector::ForwardVector);
			ProjectionData.ViewRight = FVector(ViewRight).GetSafeNormal(SMALL_NUMBER, FVector::RightVector);
			ProjectionData.ViewUp = FVector(ViewUp).GetSafeNormal(SMALL_NUMBER, FVector::UpVector);
		}
	}
	else if (Ar.IsLoading())
	{
		ProjectionData = FRuntimeMeshPaintScreenProjectionData();
	}

	return bOutSuccess;
}

void FRuntimeMeshPaintNetCommand::SetBrushSettings(
	const FMeshPaintBrushMaterialSettings& BrushSettings,
	bool bWriteMaterialSettings)
{
	FMeshPaintBrushMaterialSettings SafeBrushSettings = BrushSettings;
	SafeBrushSettings.Clamp();

	BrushSize = QuantizeBrushSize(SafeBrushSettings.BrushSize);
	BrushColor = FColor(
		QuantizeUnitFloat(SafeBrushSettings.Color.R),
		QuantizeUnitFloat(SafeBrushSettings.Color.G),
		QuantizeUnitFloat(SafeBrushSettings.Color.B),
		255);
	BrushOpacity = 255;
	BrushHardness = 255;
	BrushMetallic = QuantizeUnitFloat(SafeBrushSettings.Metallic);
	BrushRoughness = QuantizeUnitFloat(SafeBrushSettings.Roughness);

	CommandFlags = 0;
	if (SafeBrushSettings.bErase) CommandFlags |= static_cast<uint8>(ERuntimeMeshPaintNetCommandFlags::Erase);
	if (bWriteMaterialSettings)
	{
		CommandFlags |= static_cast<uint8>(ERuntimeMeshPaintNetCommandFlags::WriteMaterialSettings);
	}
}

FMeshPaintBrushMaterialSettings FRuntimeMeshPaintNetCommand::ToBrushSettings() const
{
	FMeshPaintBrushMaterialSettings BrushSettings;
	BrushSettings.Color = FLinearColor(
		DequantizeUnitFloat(BrushColor.R),
		DequantizeUnitFloat(BrushColor.G),
		DequantizeUnitFloat(BrushColor.B),
		1.0f);
	BrushSettings.BrushSize = GetBrushSize();
	BrushSettings.Metallic = DequantizeUnitFloat(BrushMetallic);
	BrushSettings.Roughness = DequantizeUnitFloat(BrushRoughness);
	BrushSettings.bErase = IsEraseCommand();
	BrushSettings.Clamp();
	return BrushSettings;
}

float FRuntimeMeshPaintNetCommand::GetBrushSize() const
{
	return FMath::Max(0.001f, BrushSize / RuntimeMeshPaintNetBrushSizeScale);
}

bool FRuntimeMeshPaintNetCommand::IsEraseCommand() const
{
	return (CommandFlags & static_cast<uint8>(ERuntimeMeshPaintNetCommandFlags::Erase)) != 0;
}

bool FRuntimeMeshPaintNetCommand::ShouldWriteMaterialSettings() const
{
	return (CommandFlags & static_cast<uint8>(ERuntimeMeshPaintNetCommandFlags::WriteMaterialSettings)) != 0;
}

bool FRuntimeMeshPaintNetCommand::operator==(const FRuntimeMeshPaintNetCommand& Other) const
{
	return TargetComponent == Other.TargetComponent &&
		SourceActor == Other.SourceActor &&
		CommandId == Other.CommandId &&
		ClientPredictionKey == Other.ClientPredictionKey &&
		StrokeId == Other.StrokeId &&
		MeshTargetIndex == Other.MeshTargetIndex &&
		LOD == Other.LOD &&
		UVChannel == Other.UVChannel &&
		BrushUVX == Other.BrushUVX &&
		BrushUVY == Other.BrushUVY &&
		BrushRayStart == Other.BrushRayStart &&
		BrushRayEnd == Other.BrushRayEnd &&
		BrushWorldCenter == Other.BrushWorldCenter &&
		BrushWorldNormal == Other.BrushWorldNormal &&
		BrushSize == Other.BrushSize &&
		BrushColor == Other.BrushColor &&
		BrushOpacity == Other.BrushOpacity &&
		BrushHardness == Other.BrushHardness &&
		BrushMetallic == Other.BrushMetallic &&
		BrushRoughness == Other.BrushRoughness &&
		CommandFlags == Other.CommandFlags &&
		ProjectionData.bHasScreenProjection == Other.ProjectionData.bHasScreenProjection &&
		(!ProjectionData.bHasScreenProjection ||
			(ProjectionData.ViewOrigin.Equals(Other.ProjectionData.ViewOrigin, 0.1) &&
			 ProjectionData.ViewForward.Equals(Other.ProjectionData.ViewForward, 0.0001) &&
			 ProjectionData.ViewRight.Equals(Other.ProjectionData.ViewRight, 0.0001) &&
			 ProjectionData.ViewUp.Equals(Other.ProjectionData.ViewUp, 0.0001)));
}

void FRuntimeMeshPaintReplicatedCommandItem::PostReplicatedAdd(
	const FRuntimeMeshPaintReplicatedCommandHistory& InArraySerializer)
{
	if (InArraySerializer.Owner)
	{
		InArraySerializer.Owner->HandleReplicatedPaintCommand(Command);
	}
}

void FRuntimeMeshPaintReplicatedCommandItem::PostReplicatedChange(
	const FRuntimeMeshPaintReplicatedCommandHistory& InArraySerializer)
{
	if (InArraySerializer.Owner)
	{
		InArraySerializer.Owner->HandleReplicatedPaintCommand(Command);
	}
}

void FRuntimeMeshPaintReplicatedCommandHistory::SetOwner(URuntimeMeshPaintTargetComponent* InOwner)
{
	Owner = InOwner;
}

void FRuntimeMeshPaintReplicatedCommandHistory::AddCommand(
	const FRuntimeMeshPaintNetCommand& Command,
	int32 MaxStoredCommands)
{
	FRuntimeMeshPaintReplicatedCommandItem& Item = Items.AddDefaulted_GetRef();
	Item.Command = Command;
	MarkItemDirty(Item);

	if (MaxStoredCommands > 0 && Items.Num() > MaxStoredCommands)
	{
		Items.RemoveAt(0, Items.Num() - MaxStoredCommands, EAllowShrinking::No);
		MarkArrayDirty();
	}
}

void FRuntimeMeshPaintReplicatedCommandHistory::ResetHistory()
{
	Items.Reset();
	MarkArrayDirty();
}
