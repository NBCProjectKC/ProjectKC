#include "Customization/KCCustomizationNetworkComponent.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Player/KCPlayerState.h"
#include "Player/Component/KCPlayerCustomizationComponent.h"

UKCCustomizationNetworkComponent::UKCCustomizationNetworkComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UKCCustomizationNetworkComponent::UploadCustomizationPayload(
	const TArray<uint8>& Payload)
{
	const APlayerController* OwnerController = GetOwningPlayerController();
	if (!OwnerController ||
		!OwnerController->IsLocalController() ||
		Payload.IsEmpty() ||
		Payload.Num() > KCCustomizationNetwork::MaxPayloadBytes)
	{
		return;
	}

	const int32 UploadId = NextCustomizationUploadId++;
	if (NextCustomizationUploadId <= 0)
	{
		NextCustomizationUploadId = 1;
	}

	ServerBeginCustomizationUpload(
		UploadId,
		Payload.Num(),
		KCCustomizationNetwork::ComputePayloadHash(Payload));

	int32 ChunkIndex = 0;
	for (int32 Offset = 0;
		Offset < Payload.Num();
		Offset += KCCustomizationNetwork::ChunkSizeBytes)
	{
		const int32 BytesThisChunk = FMath::Min(
			KCCustomizationNetwork::ChunkSizeBytes,
			Payload.Num() - Offset);
		TArray<uint8> Chunk;
		Chunk.Append(Payload.GetData() + Offset, BytesThisChunk);
		ServerUploadCustomizationChunk(UploadId, ChunkIndex++, Chunk);
	}

	ServerCommitCustomizationUpload(UploadId);
}

void UKCCustomizationNetworkComponent::RequestCustomizationPayload(
	AKCPlayerState* TargetPlayerState,
	UKCPlayerCustomizationComponent* TargetComponent)
{
	APlayerController* OwnerController = GetOwningPlayerController();
	if (!OwnerController ||
		!OwnerController->IsLocalController() ||
		!TargetPlayerState ||
		!TargetComponent)
	{
		return;
	}

	const FKCCustomizationDescriptor& Descriptor =
		TargetPlayerState->GetCustomizationDescriptor();
	if (!Descriptor.IsPublished())
	{
		return;
	}

	if (Descriptor.bUseDefaultAppearance)
	{
		TargetComponent->ApplyNetworkCustomizationData(
			FRuntimeMeshPaintPatchHistory(),
			true,
			Descriptor);
		return;
	}

	if (const FKCCachedCustomizationData* CachedData =
		CustomizationCache.Find(TargetPlayerState);
		CachedData &&
		CachedData->Revision == Descriptor.Revision &&
		CachedData->ContentHash == Descriptor.ContentHash)
	{
		TargetComponent->ApplyNetworkCustomizationData(
			CachedData->PaintHistory,
			CachedData->bUseDefaultAppearance,
			Descriptor);
		return;
	}

	PendingCustomizationTargets.Add(TargetPlayerState, TargetComponent);
	if (const uint32* PendingRevision =
		PendingCustomizationRevisions.Find(TargetPlayerState);
		PendingRevision && *PendingRevision == Descriptor.Revision)
	{
		return;
	}
	PendingCustomizationRevisions.Add(TargetPlayerState, Descriptor.Revision);

	if (OwnerController->HasAuthority())
	{
		TArray<uint8> Payload;
		if (TargetPlayerState->GetCustomizationPayload(
			Descriptor.Revision,
			Descriptor.ContentHash,
			Payload))
		{
			ApplyReceivedCustomization(TargetPlayerState, Payload);
		}
		else
		{
			ResetCustomizationDownload(TargetPlayerState);
		}
		return;
	}

	ServerRequestCustomizationPayload(
		TargetPlayerState,
		Descriptor.Revision,
		Descriptor.ContentHash);
}

void UKCCustomizationNetworkComponent::ServerBeginCustomizationUpload_Implementation(
	const int32 UploadId,
	const int32 TotalBytes,
	const uint32 ExpectedHash)
{
	ActiveCustomizationUpload.Reset();
	if (UploadId <= 0 ||
		TotalBytes <= 0 ||
		TotalBytes > KCCustomizationNetwork::MaxPayloadBytes)
	{
		return;
	}

	ActiveCustomizationUpload.UploadId = UploadId;
	ActiveCustomizationUpload.ExpectedBytes = TotalBytes;
	ActiveCustomizationUpload.ExpectedHash = ExpectedHash;
	ActiveCustomizationUpload.Bytes.Reserve(TotalBytes);
}

void UKCCustomizationNetworkComponent::ServerUploadCustomizationChunk_Implementation(
	const int32 UploadId,
	const int32 ChunkIndex,
	const TArray<uint8>& ChunkBytes)
{
	if (ActiveCustomizationUpload.UploadId != UploadId ||
		ActiveCustomizationUpload.NextChunkIndex != ChunkIndex ||
		ChunkBytes.IsEmpty() ||
		ChunkBytes.Num() > KCCustomizationNetwork::ChunkSizeBytes ||
		ActiveCustomizationUpload.Bytes.Num() + ChunkBytes.Num() >
			ActiveCustomizationUpload.ExpectedBytes)
	{
		ActiveCustomizationUpload.Reset();
		return;
	}

	ActiveCustomizationUpload.Bytes.Append(ChunkBytes);
	++ActiveCustomizationUpload.NextChunkIndex;
}

void UKCCustomizationNetworkComponent::ServerCommitCustomizationUpload_Implementation(
	const int32 UploadId)
{
	if (ActiveCustomizationUpload.UploadId != UploadId ||
		ActiveCustomizationUpload.Bytes.Num() !=
			ActiveCustomizationUpload.ExpectedBytes ||
		KCCustomizationNetwork::ComputePayloadHash(
			ActiveCustomizationUpload.Bytes) !=
			ActiveCustomizationUpload.ExpectedHash)
	{
		ActiveCustomizationUpload.Reset();
		return;
	}

	TArray<uint8> CompletedPayload = MoveTemp(ActiveCustomizationUpload.Bytes);
	ActiveCustomizationUpload.Reset();
	if (APlayerController* OwnerController = GetOwningPlayerController())
	{
		if (AKCPlayerState* KCPlayerState =
			OwnerController->GetPlayerState<AKCPlayerState>())
		{
			KCPlayerState->PublishCustomizationPayload(CompletedPayload);
		}
	}
}

void UKCCustomizationNetworkComponent::ServerRequestCustomizationPayload_Implementation(
	AKCPlayerState* TargetPlayerState,
	const uint32 Revision,
	const uint32 ContentHash)
{
	TArray<uint8> Payload;
	if (!TargetPlayerState ||
		!TargetPlayerState->GetCustomizationPayload(Revision, ContentHash, Payload))
	{
		return;
	}

	const int32 TotalChunks = FMath::DivideAndRoundUp(
		Payload.Num(),
		KCCustomizationNetwork::ChunkSizeBytes);
	ClientBeginCustomizationDownload(
		TargetPlayerState,
		Revision,
		ContentHash,
		Payload.Num(),
		TotalChunks);

	int32 ChunkIndex = 0;
	for (int32 Offset = 0;
		Offset < Payload.Num();
		Offset += KCCustomizationNetwork::ChunkSizeBytes)
	{
		const int32 BytesThisChunk = FMath::Min(
			KCCustomizationNetwork::ChunkSizeBytes,
			Payload.Num() - Offset);
		TArray<uint8> Chunk;
		Chunk.Append(Payload.GetData() + Offset, BytesThisChunk);
		ClientReceiveCustomizationChunk(
			TargetPlayerState,
			Revision,
			ChunkIndex++,
			Chunk);
	}

	ClientCompleteCustomizationDownload(TargetPlayerState, Revision);
}

void UKCCustomizationNetworkComponent::ClientBeginCustomizationDownload_Implementation(
	AKCPlayerState* TargetPlayerState,
	const uint32 Revision,
	const uint32 ContentHash,
	const int32 TotalBytes,
	const int32 TotalChunks)
{
	ActiveCustomizationDownload.Reset();
	if (!TargetPlayerState ||
		Revision == 0 ||
		TotalBytes <= 0 ||
		TotalBytes > KCCustomizationNetwork::MaxPayloadBytes ||
		TotalChunks != FMath::DivideAndRoundUp(
			TotalBytes,
			KCCustomizationNetwork::ChunkSizeBytes))
	{
		ResetCustomizationDownload(TargetPlayerState);
		return;
	}

	ActiveCustomizationDownload.PlayerState = TargetPlayerState;
	ActiveCustomizationDownload.Revision = Revision;
	ActiveCustomizationDownload.ExpectedHash = ContentHash;
	ActiveCustomizationDownload.ExpectedBytes = TotalBytes;
	ActiveCustomizationDownload.ExpectedChunks = TotalChunks;
	ActiveCustomizationDownload.Bytes.Reserve(TotalBytes);
}

void UKCCustomizationNetworkComponent::ClientReceiveCustomizationChunk_Implementation(
	AKCPlayerState* TargetPlayerState,
	const uint32 Revision,
	const int32 ChunkIndex,
	const TArray<uint8>& ChunkBytes)
{
	if (ActiveCustomizationDownload.PlayerState.Get() != TargetPlayerState ||
		ActiveCustomizationDownload.Revision != Revision ||
		ActiveCustomizationDownload.NextChunkIndex != ChunkIndex ||
		ChunkBytes.IsEmpty() ||
		ChunkBytes.Num() > KCCustomizationNetwork::ChunkSizeBytes ||
		ActiveCustomizationDownload.Bytes.Num() + ChunkBytes.Num() >
			ActiveCustomizationDownload.ExpectedBytes)
	{
		ActiveCustomizationDownload.Reset();
		ResetCustomizationDownload(TargetPlayerState);
		return;
	}

	ActiveCustomizationDownload.Bytes.Append(ChunkBytes);
	++ActiveCustomizationDownload.NextChunkIndex;
}

void UKCCustomizationNetworkComponent::ClientCompleteCustomizationDownload_Implementation(
	AKCPlayerState* TargetPlayerState,
	const uint32 Revision)
{
	if (ActiveCustomizationDownload.PlayerState.Get() != TargetPlayerState ||
		ActiveCustomizationDownload.Revision != Revision ||
		ActiveCustomizationDownload.NextChunkIndex !=
			ActiveCustomizationDownload.ExpectedChunks ||
		ActiveCustomizationDownload.Bytes.Num() !=
			ActiveCustomizationDownload.ExpectedBytes ||
		KCCustomizationNetwork::ComputePayloadHash(
			ActiveCustomizationDownload.Bytes) !=
			ActiveCustomizationDownload.ExpectedHash)
	{
		ActiveCustomizationDownload.Reset();
		ResetCustomizationDownload(TargetPlayerState);
		return;
	}

	TArray<uint8> CompletedPayload = MoveTemp(ActiveCustomizationDownload.Bytes);
	ActiveCustomizationDownload.Reset();
	if (!ApplyReceivedCustomization(TargetPlayerState, CompletedPayload))
	{
		ResetCustomizationDownload(TargetPlayerState);
	}
}

APlayerController* UKCCustomizationNetworkComponent::GetOwningPlayerController() const
{
	return Cast<APlayerController>(GetOwner());
}

bool UKCCustomizationNetworkComponent::ApplyReceivedCustomization(
	AKCPlayerState* TargetPlayerState,
	const TArray<uint8>& Payload)
{
	if (!TargetPlayerState)
	{
		return false;
	}

	const FKCCustomizationDescriptor& Descriptor =
		TargetPlayerState->GetCustomizationDescriptor();
	if (!Descriptor.IsPublished() ||
		Descriptor.ContentHash !=
			KCCustomizationNetwork::ComputePayloadHash(Payload))
	{
		return false;
	}

	FRuntimeMeshPaintPatchHistory PaintHistory;
	bool bUseDefaultAppearance = true;
	if (!KCCustomizationNetwork::DeserializePayload(
		Payload,
		PaintHistory,
		bUseDefaultAppearance) ||
		Descriptor.bUseDefaultAppearance != bUseDefaultAppearance)
	{
		return false;
	}

	FKCCachedCustomizationData& CachedData =
		CustomizationCache.FindOrAdd(TargetPlayerState);
	CachedData.Revision = Descriptor.Revision;
	CachedData.ContentHash = Descriptor.ContentHash;
	CachedData.bUseDefaultAppearance = bUseDefaultAppearance;
	CachedData.PaintHistory = PaintHistory;

	UKCPlayerCustomizationComponent* TargetComponent = nullptr;
	if (const TWeakObjectPtr<UKCPlayerCustomizationComponent>* PendingTarget =
		PendingCustomizationTargets.Find(TargetPlayerState))
	{
		TargetComponent = PendingTarget->Get();
	}
	if (!TargetComponent)
	{
		TargetComponent = ResolveCustomizationComponent(TargetPlayerState);
	}

	PendingCustomizationTargets.Remove(TargetPlayerState);
	PendingCustomizationRevisions.Remove(TargetPlayerState);
	return TargetComponent && TargetComponent->ApplyNetworkCustomizationData(
		PaintHistory,
		bUseDefaultAppearance,
		Descriptor);
}

UKCPlayerCustomizationComponent*
UKCCustomizationNetworkComponent::ResolveCustomizationComponent(
	AKCPlayerState* TargetPlayerState) const
{
	APawn* TargetPawn = TargetPlayerState ? TargetPlayerState->GetPawn() : nullptr;
	return TargetPawn
		? TargetPawn->FindComponentByClass<UKCPlayerCustomizationComponent>()
		: nullptr;
}

void UKCCustomizationNetworkComponent::ResetCustomizationDownload(
	AKCPlayerState* TargetPlayerState)
{
	if (TargetPlayerState)
	{
		PendingCustomizationTargets.Remove(TargetPlayerState);
		PendingCustomizationRevisions.Remove(TargetPlayerState);
	}
}
