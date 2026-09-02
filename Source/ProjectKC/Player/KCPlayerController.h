#pragma once

#include "CoreMinimal.h"
#include "Customization/KCCustomizationNetworkTypes.h"
#include "GameFramework/PlayerController.h"
#include "KCPlayerController.generated.h"

struct FInputActionValue;
class UInputAction;
class UInputMappingContext;
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

UCLASS()
class PROJECTKC_API AKCPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AKCPlayerController();

	virtual void ReceivedPlayer() override;
	UFUNCTION(BlueprintPure, Category = "KC|Network")
	float GetServerTime() const; 

	/** 로컬 외형 페이로드를 서버로 분할 업로드합니다. */
	void UploadCustomizationPayload(const TArray<uint8>& Payload);

	/** 특정 PlayerState의 최신 외형을 서버에 요청하고 대상 컴포넌트에 적용합니다. */
	void RequestCustomizationPayload(
		AKCPlayerState* TargetPlayerState,
		UKCPlayerCustomizationComponent* TargetComponent);
	
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaSeconds) override;

private:
	void InitializeInGameHUD();
	void ClearInGameHUD();
	void Move(const FInputActionValue& InputValue);
	void Dash(const FInputActionValue& InputValue);
	void Emote(const FInputActionValue& InputValue);
	void BeginUseHeldItem(const FInputActionValue& InputValue);
	void EndUseHeldItem(const FInputActionValue& InputValue);
	void Interact(const FInputActionValue& InputValue);
	void DropHeldItem(const FInputActionValue& InputValue);
	void UpdateCharacterFacing(float DeltaSeconds);
	
	// 서버 시간 동기화
	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);
	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceivedClientRequest);

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

	float ClientServerDelta = 0.0f;
	float TimeSinceLastServerTimeSync = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> PlayerMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> DashAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> EmoteAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> AttackAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> DropHeldItemAction;
};
