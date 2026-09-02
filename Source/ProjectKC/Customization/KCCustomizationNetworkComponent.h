#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Customization/KCCustomizationNetworkTypes.h"
#include "KCCustomizationNetworkComponent.generated.h"

class AKCPlayerState;
class UKCPlayerCustomizationComponent;

struct FKCServerCustomizationUpload
{
	int32 UploadId = INDEX_NONE;
	int32 ExpectedBytes = 0;
	int32 NextChunkIndex = 0;
	uint32 ExpectedHash = 0;
	TArray<uint8> Bytes;

	void Reset() { *this = FKCServerCustomizationUpload(); }
};

struct FKCClientCustomizationDownload
{
	TWeakObjectPtr<AKCPlayerState> PlayerState;
	uint32 Revision = 0;
	uint32 ExpectedHash = 0;
	int32 ExpectedBytes = 0;
	int32 ExpectedChunks = 0;
	int32 NextChunkIndex = 0;
	TArray<uint8> Bytes;

	void Reset() { *this = FKCClientCustomizationDownload(); }
};

struct FKCCachedCustomizationData
{
	uint32 Revision = 0;
	uint32 ContentHash = 0;
	bool bUseDefaultAppearance = true;
	FRuntimeMeshPaintPatchHistory PaintHistory;
};

/**
 * 로비와 인게임 PlayerController가 공유하는 외형 페이로드 전송 컴포넌트입니다.
 * PlayerState에는 작은 Descriptor만 복제하고 실제 데이터는 소유 Controller의
 * 신뢰성 있는 청크 RPC로 요청/전송합니다.
 */
UCLASS(ClassGroup = (KC), meta = (BlueprintSpawnableComponent))
class PROJECTKC_API UKCCustomizationNetworkComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UKCCustomizationNetworkComponent();

	/** 로컬 외형 페이로드를 서버로 분할 업로드합니다. */
	void UploadCustomizationPayload(const TArray<uint8>& Payload);

	/** 특정 PlayerState의 최신 외형을 요청하고 대상 컴포넌트에 적용합니다. */
	void RequestCustomizationPayload(
		AKCPlayerState* TargetPlayerState,
		UKCPlayerCustomizationComponent* TargetComponent);

private:
	UFUNCTION(Server, Reliable)
	void ServerBeginCustomizationUpload(
		int32 UploadId,
		int32 TotalBytes,
		uint32 ExpectedHash);

	UFUNCTION(Server, Reliable)
	void ServerUploadCustomizationChunk(
		int32 UploadId,
		int32 ChunkIndex,
		const TArray<uint8>& ChunkBytes);

	UFUNCTION(Server, Reliable)
	void ServerCommitCustomizationUpload(int32 UploadId);

	UFUNCTION(Server, Reliable)
	void ServerRequestCustomizationPayload(
		AKCPlayerState* TargetPlayerState,
		uint32 Revision,
		uint32 ContentHash);

	UFUNCTION(Client, Reliable)
	void ClientBeginCustomizationDownload(
		AKCPlayerState* TargetPlayerState,
		uint32 Revision,
		uint32 ContentHash,
		int32 TotalBytes,
		int32 TotalChunks);

	UFUNCTION(Client, Reliable)
	void ClientReceiveCustomizationChunk(
		AKCPlayerState* TargetPlayerState,
		uint32 Revision,
		int32 ChunkIndex,
		const TArray<uint8>& ChunkBytes);

	UFUNCTION(Client, Reliable)
	void ClientCompleteCustomizationDownload(
		AKCPlayerState* TargetPlayerState,
		uint32 Revision);

	class APlayerController* GetOwningPlayerController() const;
	bool ApplyReceivedCustomization(
		AKCPlayerState* TargetPlayerState,
		const TArray<uint8>& Payload);
	UKCPlayerCustomizationComponent* ResolveCustomizationComponent(
		AKCPlayerState* TargetPlayerState) const;
	void ResetCustomizationDownload(AKCPlayerState* TargetPlayerState);

	int32 NextCustomizationUploadId = 1;
	FKCServerCustomizationUpload ActiveCustomizationUpload;
	FKCClientCustomizationDownload ActiveCustomizationDownload;
	TMap<TWeakObjectPtr<AKCPlayerState>, FKCCachedCustomizationData> CustomizationCache;
	TMap<TWeakObjectPtr<AKCPlayerState>, TWeakObjectPtr<UKCPlayerCustomizationComponent>> PendingCustomizationTargets;
	TMap<TWeakObjectPtr<AKCPlayerState>, uint32> PendingCustomizationRevisions;
};
