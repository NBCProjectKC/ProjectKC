#include "Customization/KCCustomizationNetworkComponent.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Player/KCPlayerState.h"
#include "Player/Component/KCPlayerCustomizationComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogKCCustomizationNetwork, Log, All);

UKCCustomizationNetworkComponent::UKCCustomizationNetworkComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UKCCustomizationNetworkComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ResetTransientCustomizationData();
	Super::EndPlay(EndPlayReason);
}

void UKCCustomizationNetworkComponent::ForgetCustomizationData(
	AKCPlayerState* TargetPlayerState)
{
	if (!TargetPlayerState)
	{
		return;
	}

	CustomizationCache.Remove(TargetPlayerState);
	PendingCustomizationTargets.Remove(TargetPlayerState);
	PendingCustomizationRevisions.Remove(TargetPlayerState);

	if (ActiveCustomizationRequestPlayerState.Get() == TargetPlayerState)
	{
		const uint32 ActiveRevision = ActiveCustomizationRequestRevision;
		ActiveCustomizationRequestPlayerState.Reset();
		ActiveCustomizationRequestRevision = 0;
		ActiveCustomizationDownload.Reset();
		if (ActiveRevision != 0)
		{
			ServerCancelCustomizationDownload(TargetPlayerState, ActiveRevision);
		}
		TryStartNextCustomizationDownload();
	}

	if (ActiveServerCustomizationDownload.PlayerState.Get() == TargetPlayerState)
	{
		ActiveServerCustomizationDownload.Reset();
	}
}

void UKCCustomizationNetworkComponent::ResetTransientCustomizationData()
{
	ActiveClientCustomizationUpload.Reset();
	PendingClientCustomizationUpload.Reset();
	ActiveCustomizationUpload.Reset();
	ActiveServerCustomizationDownload.Reset();
	ActiveCustomizationDownload.Reset();
	ActiveCustomizationRequestPlayerState.Reset();
	ActiveCustomizationRequestRevision = 0;
	CustomizationCache.Empty();
	PendingCustomizationTargets.Empty();
	PendingCustomizationRevisions.Empty();
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

	const uint32 ContentHash =
		KCCustomizationNetwork::ComputePayloadHash(Payload);
	if (ActiveClientCustomizationUpload.UploadId != INDEX_NONE)
	{
		if (ActiveClientCustomizationUpload.ContentHash == ContentHash)
		{
			PendingClientCustomizationUpload.Reset();
		}
		else
		{
			PendingClientCustomizationUpload = Payload;
		}
		return;
	}

	const int32 UploadId = NextCustomizationUploadId++;
	if (NextCustomizationUploadId <= 0)
	{
		NextCustomizationUploadId = 1;
	}

	ActiveClientCustomizationUpload.Reset();
	ActiveClientCustomizationUpload.UploadId = UploadId;
	ActiveClientCustomizationUpload.ContentHash = ContentHash;
	ActiveClientCustomizationUpload.Bytes = Payload;

	ServerBeginCustomizationUpload(
		UploadId,
		Payload.Num(),
		ContentHash);
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
			FinishCustomizationDownload(
				TargetPlayerState,
				Descriptor.Revision);
		}
		return;
	}

	TryStartNextCustomizationDownload();
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
		ClientFinishCustomizationUpload(UploadId, false);
		return;
	}

	ActiveCustomizationUpload.UploadId = UploadId;
	ActiveCustomizationUpload.ExpectedBytes = TotalBytes;
	ActiveCustomizationUpload.ExpectedHash = ExpectedHash;
	ActiveCustomizationUpload.Bytes.Reserve(TotalBytes);
	ClientRequestCustomizationUploadChunk(UploadId, 0);
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
		ClientFinishCustomizationUpload(UploadId, false);
		return;
	}

	ActiveCustomizationUpload.Bytes.Append(ChunkBytes);
	++ActiveCustomizationUpload.NextChunkIndex;
	ClientRequestCustomizationUploadChunk(
		UploadId,
		ActiveCustomizationUpload.NextChunkIndex);
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
		ClientFinishCustomizationUpload(UploadId, false);
		return;
	}

	TArray<uint8> CompletedPayload = MoveTemp(ActiveCustomizationUpload.Bytes);
	ActiveCustomizationUpload.Reset();
	bool bPublished = false;
	if (APlayerController* OwnerController = GetOwningPlayerController())
	{
		if (AKCPlayerState* KCPlayerState =
			OwnerController->GetPlayerState<AKCPlayerState>())
		{
			bPublished = KCPlayerState->PublishCustomizationPayload(CompletedPayload);
		}
	}
	ClientFinishCustomizationUpload(UploadId, bPublished);
}

void UKCCustomizationNetworkComponent::ClientRequestCustomizationUploadChunk_Implementation(
	const int32 UploadId,
	const int32 ChunkIndex)
{
	if (ActiveClientCustomizationUpload.UploadId != UploadId ||
		ActiveClientCustomizationUpload.NextChunkIndex != ChunkIndex)
	{
		return;
	}

	const int32 Offset = ChunkIndex * KCCustomizationNetwork::ChunkSizeBytes;
	if (Offset >= ActiveClientCustomizationUpload.Bytes.Num())
	{
		ServerCommitCustomizationUpload(UploadId);
		return;
	}

	const int32 BytesThisChunk = FMath::Min(
		KCCustomizationNetwork::ChunkSizeBytes,
		ActiveClientCustomizationUpload.Bytes.Num() - Offset);
	TArray<uint8> Chunk;
	Chunk.Append(
		ActiveClientCustomizationUpload.Bytes.GetData() + Offset,
		BytesThisChunk);
	++ActiveClientCustomizationUpload.NextChunkIndex;
	ServerUploadCustomizationChunk(UploadId, ChunkIndex, Chunk);
}

void UKCCustomizationNetworkComponent::ClientFinishCustomizationUpload_Implementation(
	const int32 UploadId,
	const bool bSucceeded)
{
	if (ActiveClientCustomizationUpload.UploadId != UploadId)
	{
		return;
	}

	if (bSucceeded)
	{
		UE_LOG(LogKCCustomizationNetwork, Log,
			TEXT("Customization upload finished: Owner=%s, UploadId=%d"),
			*GetNameSafe(GetOwner()),
			UploadId);
	}
	else
	{
		UE_LOG(LogKCCustomizationNetwork, Warning,
			TEXT("Customization upload failed: Owner=%s, UploadId=%d"),
			*GetNameSafe(GetOwner()),
			UploadId);
	}
	ActiveClientCustomizationUpload.Reset();

	if (!PendingClientCustomizationUpload.IsEmpty())
	{
		TArray<uint8> PendingPayload =
			MoveTemp(PendingClientCustomizationUpload);
		UploadCustomizationPayload(PendingPayload);
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
		ClientAbortCustomizationDownload(TargetPlayerState, Revision);
		return;
	}

	ActiveServerCustomizationDownload.Reset();
	ActiveServerCustomizationDownload.PlayerState = TargetPlayerState;
	ActiveServerCustomizationDownload.Revision = Revision;
	ActiveServerCustomizationDownload.ContentHash = ContentHash;
	ActiveServerCustomizationDownload.Bytes = MoveTemp(Payload);

	const int32 TotalChunks = FMath::DivideAndRoundUp(
		ActiveServerCustomizationDownload.Bytes.Num(),
		KCCustomizationNetwork::ChunkSizeBytes);
	ClientBeginCustomizationDownload(
		TargetPlayerState,
		Revision,
		ContentHash,
		ActiveServerCustomizationDownload.Bytes.Num(),
		TotalChunks);
}

void UKCCustomizationNetworkComponent::ServerAcknowledgeCustomizationDownloadChunk_Implementation(
	AKCPlayerState* TargetPlayerState,
	const uint32 Revision,
	const int32 NextChunkIndex)
{
	if (ActiveServerCustomizationDownload.PlayerState.Get() != TargetPlayerState ||
		ActiveServerCustomizationDownload.Revision != Revision ||
		ActiveServerCustomizationDownload.NextChunkIndex != NextChunkIndex)
	{
		ActiveServerCustomizationDownload.Reset();
		ClientAbortCustomizationDownload(TargetPlayerState, Revision);
		return;
	}

	const int32 Offset =
		NextChunkIndex * KCCustomizationNetwork::ChunkSizeBytes;
	if (Offset >= ActiveServerCustomizationDownload.Bytes.Num())
	{
		ActiveServerCustomizationDownload.Reset();
		ClientCompleteCustomizationDownload(TargetPlayerState, Revision);
		return;
	}

	const int32 BytesThisChunk = FMath::Min(
		KCCustomizationNetwork::ChunkSizeBytes,
		ActiveServerCustomizationDownload.Bytes.Num() - Offset);
	TArray<uint8> Chunk;
	Chunk.Append(
		ActiveServerCustomizationDownload.Bytes.GetData() + Offset,
		BytesThisChunk);
	++ActiveServerCustomizationDownload.NextChunkIndex;
	ClientReceiveCustomizationChunk(
		TargetPlayerState,
		Revision,
		NextChunkIndex,
		Chunk);
}

void UKCCustomizationNetworkComponent::ServerCancelCustomizationDownload_Implementation(
	AKCPlayerState* TargetPlayerState,
	const uint32 Revision)
{
	if (ActiveServerCustomizationDownload.PlayerState.Get() == TargetPlayerState &&
		ActiveServerCustomizationDownload.Revision == Revision)
	{
		ActiveServerCustomizationDownload.Reset();
	}
}

void UKCCustomizationNetworkComponent::ClientBeginCustomizationDownload_Implementation(
	AKCPlayerState* TargetPlayerState,
	const uint32 Revision,
	const uint32 ContentHash,
	const int32 TotalBytes,
	const int32 TotalChunks)
{
	ActiveCustomizationDownload.Reset();
	if (ActiveCustomizationRequestPlayerState.Get() != TargetPlayerState ||
		ActiveCustomizationRequestRevision != Revision ||
		!TargetPlayerState ||
		Revision == 0 ||
		TotalBytes <= 0 ||
		TotalBytes > KCCustomizationNetwork::MaxPayloadBytes ||
		TotalChunks != FMath::DivideAndRoundUp(
			TotalBytes,
			KCCustomizationNetwork::ChunkSizeBytes))
	{
		ServerCancelCustomizationDownload(TargetPlayerState, Revision);
		FinishCustomizationDownload(TargetPlayerState, Revision);
		return;
	}

	ActiveCustomizationDownload.PlayerState = TargetPlayerState;
	ActiveCustomizationDownload.Revision = Revision;
	ActiveCustomizationDownload.ExpectedHash = ContentHash;
	ActiveCustomizationDownload.ExpectedBytes = TotalBytes;
	ActiveCustomizationDownload.ExpectedChunks = TotalChunks;
	ActiveCustomizationDownload.Bytes.Reserve(TotalBytes);
	ServerAcknowledgeCustomizationDownloadChunk(
		TargetPlayerState,
		Revision,
		0);
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
		ServerCancelCustomizationDownload(TargetPlayerState, Revision);
		FinishCustomizationDownload(TargetPlayerState, Revision);
		return;
	}

	ActiveCustomizationDownload.Bytes.Append(ChunkBytes);
	++ActiveCustomizationDownload.NextChunkIndex;
	ServerAcknowledgeCustomizationDownloadChunk(
		TargetPlayerState,
		Revision,
		ActiveCustomizationDownload.NextChunkIndex);
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
		FinishCustomizationDownload(TargetPlayerState, Revision);
		return;
	}

	TArray<uint8> CompletedPayload = MoveTemp(ActiveCustomizationDownload.Bytes);
	ActiveCustomizationDownload.Reset();
	ApplyReceivedCustomization(TargetPlayerState, CompletedPayload);
	FinishCustomizationDownload(TargetPlayerState, Revision);
}

void UKCCustomizationNetworkComponent::ClientAbortCustomizationDownload_Implementation(
	AKCPlayerState* TargetPlayerState,
	const uint32 Revision)
{
	if (ActiveCustomizationDownload.PlayerState.Get() == TargetPlayerState &&
		ActiveCustomizationDownload.Revision == Revision)
	{
		ActiveCustomizationDownload.Reset();
	}
	FinishCustomizationDownload(TargetPlayerState, Revision);
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

void UKCCustomizationNetworkComponent::TryStartNextCustomizationDownload()
{
	if (ActiveCustomizationRequestRevision != 0)
	{
		return;
	}

	APlayerController* OwnerController = GetOwningPlayerController();
	if (!OwnerController ||
		!OwnerController->IsLocalController() ||
		OwnerController->HasAuthority())
	{
		return;
	}

	TArray<TWeakObjectPtr<AKCPlayerState>> PendingPlayerStates;
	PendingCustomizationRevisions.GetKeys(PendingPlayerStates);
	for (const TWeakObjectPtr<AKCPlayerState>& PlayerStatePtr : PendingPlayerStates)
	{
		AKCPlayerState* PlayerState = PlayerStatePtr.Get();
		const uint32* PendingRevision =
			PendingCustomizationRevisions.Find(PlayerStatePtr);
		UKCPlayerCustomizationComponent* TargetComponent = nullptr;
		if (const TWeakObjectPtr<UKCPlayerCustomizationComponent>* TargetPtr =
			PendingCustomizationTargets.Find(PlayerStatePtr))
		{
			TargetComponent = TargetPtr->Get();
		}

		if (!PlayerState || !PendingRevision || !TargetComponent)
		{
			PendingCustomizationTargets.Remove(PlayerStatePtr);
			PendingCustomizationRevisions.Remove(PlayerStatePtr);
			continue;
		}

		const FKCCustomizationDescriptor& Descriptor =
			PlayerState->GetCustomizationDescriptor();
		if (!Descriptor.IsPublished() ||
			Descriptor.Revision != *PendingRevision ||
			Descriptor.bUseDefaultAppearance)
		{
			PendingCustomizationTargets.Remove(PlayerStatePtr);
			PendingCustomizationRevisions.Remove(PlayerStatePtr);
			continue;
		}

		ActiveCustomizationRequestPlayerState = PlayerState;
		ActiveCustomizationRequestRevision = Descriptor.Revision;
		ServerRequestCustomizationPayload(
			PlayerState,
			Descriptor.Revision,
			Descriptor.ContentHash);
		return;
	}
}

void UKCCustomizationNetworkComponent::FinishCustomizationDownload(
	AKCPlayerState* TargetPlayerState,
	const uint32 Revision)
{
	if (ActiveCustomizationRequestPlayerState.Get() == TargetPlayerState &&
		ActiveCustomizationRequestRevision == Revision)
	{
		ActiveCustomizationRequestPlayerState.Reset();
		ActiveCustomizationRequestRevision = 0;
	}

	if (const uint32* PendingRevision =
		PendingCustomizationRevisions.Find(TargetPlayerState);
		PendingRevision && *PendingRevision == Revision)
	{
		PendingCustomizationTargets.Remove(TargetPlayerState);
		PendingCustomizationRevisions.Remove(TargetPlayerState);
	}

	TryStartNextCustomizationDownload();
}
