// Copyright Shared Orbit 2026. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/NetSerialization.h"
#include "GameFramework/Actor.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "MeshPaintingCoreTypes.h"
#include "RuntimeMeshPaintReplicationTypes.generated.h"

class URuntimeMeshPaintTargetComponent;

UENUM()
enum class ERuntimeMeshPaintNetCommandFlags : uint8
{
	None = 0,
	Erase = 1 << 0,
	WriteMaterialSettings = 1 << 1,
	ScreenProjection = 1 << 2,
	PaintUV = 1 << 3
};
ENUM_CLASS_FLAGS(ERuntimeMeshPaintNetCommandFlags)

USTRUCT(BlueprintType)
struct MESHPAINTINGCORE_API FRuntimeMeshPaintNetCommand
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UActorComponent> TargetComponent = nullptr;

	UPROPERTY()
	TObjectPtr<AActor> SourceActor = nullptr;

	UPROPERTY()
	uint32 CommandId = 0;

	UPROPERTY()
	uint32 ClientPredictionKey = 0;

	UPROPERTY()
	uint16 StrokeId = 0;

	UPROPERTY()
	uint8 MeshTargetIndex = 0;

	UPROPERTY()
	uint8 LOD = 0;

	UPROPERTY()
	uint8 UVChannel = 0;

	UPROPERTY()
	uint16 BrushUVX = 0;

	UPROPERTY()
	uint16 BrushUVY = 0;

	UPROPERTY()
	FVector_NetQuantize10 BrushRayStart = FVector::ZeroVector;

	UPROPERTY()
	FVector_NetQuantize10 BrushRayEnd = FVector::ZeroVector;

	UPROPERTY()
	FVector_NetQuantize10 BrushWorldCenter = FVector::ZeroVector;

	UPROPERTY()
	FVector_NetQuantizeNormal BrushWorldNormal = FVector::UpVector;

	UPROPERTY()
	uint16 BrushSize = 0;

	UPROPERTY()
	FColor BrushColor = FColor::White;

	UPROPERTY()
	uint8 BrushOpacity = 255;

	UPROPERTY()
	uint8 BrushHardness = 255;

	UPROPERTY()
	uint8 BrushMetallic = 0;

	UPROPERTY()
	uint8 BrushRoughness = 128;

	UPROPERTY()
	uint8 CommandFlags = 0;

	FRuntimeMeshPaintScreenProjectionData ProjectionData;

	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);

	void SetBrushSettings(const FMeshPaintBrushMaterialSettings& BrushSettings, bool bWriteMaterialSettings);
	FMeshPaintBrushMaterialSettings ToBrushSettings() const;
	float GetBrushSize() const;
	bool IsEraseCommand() const;
	bool ShouldWriteMaterialSettings() const;

	bool operator==(const FRuntimeMeshPaintNetCommand& Other) const;
	bool operator!=(const FRuntimeMeshPaintNetCommand& Other) const { return !(*this == Other); }
};

template<>
struct TStructOpsTypeTraits<FRuntimeMeshPaintNetCommand> : public TStructOpsTypeTraitsBase2<FRuntimeMeshPaintNetCommand>
{
	enum
	{
		WithNetSerializer = true,
		WithIdenticalViaEquality = true
	};
};

USTRUCT(BlueprintType)
struct MESHPAINTINGCORE_API FRuntimeMeshPaintNetCommandBatch
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FRuntimeMeshPaintNetCommand> Commands;
};

USTRUCT()
struct MESHPAINTINGCORE_API FRuntimeMeshPaintReplicatedCommandItem : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY()
	FRuntimeMeshPaintNetCommand Command;

	void PostReplicatedAdd(const struct FRuntimeMeshPaintReplicatedCommandHistory& InArraySerializer);
	void PostReplicatedChange(const struct FRuntimeMeshPaintReplicatedCommandHistory& InArraySerializer);
};

USTRUCT()
struct MESHPAINTINGCORE_API FRuntimeMeshPaintReplicatedCommandHistory : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FRuntimeMeshPaintReplicatedCommandItem> Items;

	void SetOwner(URuntimeMeshPaintTargetComponent* InOwner);
	void AddCommand(const FRuntimeMeshPaintNetCommand& Command, int32 MaxStoredCommands);
	void ResetHistory();

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<
			FRuntimeMeshPaintReplicatedCommandItem,
			FRuntimeMeshPaintReplicatedCommandHistory>(Items, DeltaParms, *this);
	}

private:
	UPROPERTY(NotReplicated)
	TObjectPtr<URuntimeMeshPaintTargetComponent> Owner = nullptr;

	friend struct FRuntimeMeshPaintReplicatedCommandItem;
};

template<>
struct TStructOpsTypeTraits<FRuntimeMeshPaintReplicatedCommandHistory> :
	public TStructOpsTypeTraitsBase2<FRuntimeMeshPaintReplicatedCommandHistory>
{
	enum
	{
		WithNetDeltaSerializer = true
	};
};
