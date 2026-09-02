#include "Player/KCPlayerController.h"

#include "Math/RotationMatrix.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Customization/KCCustomizationNetworkTypes.h"
#include "GameFramework/Pawn.h"
#include "Player/KCPlayerCharacter.h"
#include "Player/KCPlayerState.h"
#include "Player/Component/KCPlayerCustomizationComponent.h"
#include "ProjectKC/UI/Common/Core/KCLocalPlayerUISubsystem.h"
#include "ProjectKC/UI/Common/Core/KCUISettings.h"
#include "ProjectKC/UI/HUD/Widget/KCHUDWidget.h"

AKCPlayerController::AKCPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;
}

void AKCPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
			LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (PlayerMappingContext)
			{
				InputSubsystem->AddMappingContext(PlayerMappingContext, 0);
			}
		}
	}

	InitializeInGameHUD();
}

void AKCPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearInGameHUD();

	Super::EndPlay(EndPlayReason);
}

void AKCPlayerController::InitializeInGameHUD()
{
	if (!IsLocalController())
	{
		return;
	}

	FInputModeGameOnly InputMode;
	// 마우스 커서를 표시한 상태에서는 기본값(true)이 첫 클릭을 뷰포트 캡처에
	// 소비한다. 공격 입력이 첫 클릭부터 전달되도록 캡처 클릭도 게임에 넘긴다.
	InputMode.SetConsumeCaptureMouseDown(false);
	SetInputMode(InputMode);

	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!LocalPlayer)
	{
		UE_LOG(LogTemp, Warning, TEXT("KC HUD failed: LocalPlayer is null on %s."), *GetName());
		return;
	}

	UKCLocalPlayerUISubsystem* UISubsystem = LocalPlayer->GetSubsystem<UKCLocalPlayerUISubsystem>();
	if (!UISubsystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("KC HUD failed: KCLocalPlayerUISubsystem is null on %s."), *GetName());
		return;
	}

	const UKCUISettings* UISettings = GetDefault<UKCUISettings>();
	const TSubclassOf<UKCHUDWidget> HUDWidgetClass = UISettings ? UISettings->HUDWidgetClass.LoadSynchronous() : nullptr;
	if (!HUDWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("KC HUD failed: HUDWidgetClass is not configured in ProjectKC UI settings."));
		return;
	}

	if (!UISubsystem->SetHUDWidget(HUDWidgetClass))
	{
		UE_LOG(LogTemp, Warning, TEXT("KC HUD failed: SetHUDWidget returned null for %s."), *GetNameSafe(HUDWidgetClass));
	}
}

void AKCPlayerController::ClearInGameHUD()
{
	if (!IsLocalController())
	{
		return;
	}

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UKCLocalPlayerUISubsystem* UISubsystem = LocalPlayer->GetSubsystem<UKCLocalPlayerUISubsystem>())
		{
			UISubsystem->ClearHUDWidget();
		}
	}
}

void AKCPlayerController::BeginUseHeldItem(const FInputActionValue& InputValue)
{
	if (!InputValue.Get<bool>())
	{
		return;
	}

	if (AKCPlayerCharacter* PlayerCharacter = Cast<AKCPlayerCharacter>(GetPawn()))
	{
		PlayerCharacter->BeginUseHeldItem();
	}
}

void AKCPlayerController::EndUseHeldItem(const FInputActionValue& InputValue)
{
	if (AKCPlayerCharacter* PlayerCharacter = Cast<AKCPlayerCharacter>(GetPawn()))
	{
		PlayerCharacter->EndUseHeldItem();
	}
}

void AKCPlayerController::Interact(const FInputActionValue& InputValue)
{
	if (!InputValue.Get<bool>())
	{
		return;
	}

	if (AKCPlayerCharacter* PlayerCharacter = Cast<AKCPlayerCharacter>(GetPawn()))
	{
		PlayerCharacter->RequestInteract();
	}
}

void AKCPlayerController::DropHeldItem(const FInputActionValue& InputValue)
{
	if (!InputValue.Get<bool>())
	{
		return;
	}

	if (AKCPlayerCharacter* PlayerCharacter = Cast<AKCPlayerCharacter>(GetPawn()))
	{
		PlayerCharacter->RequestDropHeldItem();
	}
}

void AKCPlayerController::Dash(const FInputActionValue& InputValue)
{
	if (!InputValue.Get<bool>())
	{
		return;
	}

	if (AKCPlayerCharacter* PlayerCharacter = Cast<AKCPlayerCharacter>(GetPawn()))
	{
		PlayerCharacter->RequestDash();
	}
}

void AKCPlayerController::Emote(const FInputActionValue& InputValue)
{
	if (!InputValue.Get<bool>())
	{
		return;
	}

	if (AKCPlayerCharacter* PlayerCharacter = Cast<AKCPlayerCharacter>(GetPawn()))
	{
		PlayerCharacter->RequestPlayNextEmote();
	}
}

void AKCPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent);
	if (MoveAction)
	{
		EnhancedInputComponent->BindAction(
			MoveAction, ETriggerEvent::Triggered, this, &AKCPlayerController::Move);
	}

	if (DashAction)
	{
		EnhancedInputComponent->BindAction(
			DashAction,
			ETriggerEvent::Started,
			this,
			&AKCPlayerController::Dash);
	}

	if (EmoteAction)
	{
		EnhancedInputComponent->BindAction(
			EmoteAction,
			ETriggerEvent::Started,
			this,
			&AKCPlayerController::Emote);
	}

	if (AttackAction)
	{
		EnhancedInputComponent->BindAction(
			AttackAction,
			ETriggerEvent::Started,
			this,
			&AKCPlayerController::BeginUseHeldItem);
		EnhancedInputComponent->BindAction(
			AttackAction,
			ETriggerEvent::Completed,
			this,
			&AKCPlayerController::EndUseHeldItem);
		EnhancedInputComponent->BindAction(
			AttackAction,
			ETriggerEvent::Canceled,
			this,
			&AKCPlayerController::EndUseHeldItem);
	}

	if (InteractAction)
	{
		EnhancedInputComponent->BindAction(
			InteractAction, ETriggerEvent::Started, this, &AKCPlayerController::Interact);
	}

	if (DropHeldItemAction)
	{
		EnhancedInputComponent->BindAction(
			DropHeldItemAction,
			ETriggerEvent::Started,
			this,
			&AKCPlayerController::DropHeldItem);
	}
}

void AKCPlayerController::PlayerTick(const float DeltaSeconds)
{
	Super::PlayerTick(DeltaSeconds);

	if (IsLocalController())
	{
		UpdateCharacterFacing(DeltaSeconds);
		
		// 서버 시간 주기적으로 재동기화
		if (!HasAuthority())
		{
			TimeSinceLastServerTimeSync += DeltaSeconds;
			if (TimeSinceLastServerTimeSync > 5.0f)
			{
				ServerRequestServerTime(GetWorld()->GetTimeSeconds());
				TimeSinceLastServerTimeSync = 0.0f;
			}
		}
	}
}

void AKCPlayerController::Move(const FInputActionValue& InputValue)
{
	if (AKCPlayerCharacter* PlayerCharacter = Cast<AKCPlayerCharacter>(GetPawn()))
	{
		const FVector2D MovementInput = InputValue.Get<FVector2D>();

		// 카메라가 회전해도 WASD는 항상 화면의 위/오른쪽을 기준으로 움직인다.
		FVector CameraLocation;
		FRotator CameraRotation;
		GetPlayerViewPoint(CameraLocation, CameraRotation);
		const FRotationMatrix CameraYawRotation(
			FRotator(0.0f, CameraRotation.Yaw, 0.0f));
		PlayerCharacter->MoveInWorldDirection(
			CameraYawRotation.GetUnitAxis(EAxis::X), MovementInput.Y);
		PlayerCharacter->MoveInWorldDirection(
			CameraYawRotation.GetUnitAxis(EAxis::Y), MovementInput.X);
	}
}

void AKCPlayerController::UpdateCharacterFacing(const float DeltaSeconds)
{
	AKCPlayerCharacter* PlayerCharacter = Cast<AKCPlayerCharacter>(GetPawn());
	if (!PlayerCharacter)
	{
		return;
	}

	FVector MouseWorldLocation;
	FVector MouseWorldDirection;
	if (!DeprojectMousePositionToWorld(MouseWorldLocation, MouseWorldDirection))
	{
		return;
	}

	if (FMath::IsNearlyZero(MouseWorldDirection.Z))
	{
		return;
	}

	const FVector CharacterLocation = PlayerCharacter->GetActorLocation();
	const float DistanceToCharacterPlane =
		(CharacterLocation.Z - MouseWorldLocation.Z) / MouseWorldDirection.Z;
	if (DistanceToCharacterPlane <= 0.0f)
	{
		return;
	}

	const FVector MousePlaneLocation =
		MouseWorldLocation + MouseWorldDirection * DistanceToCharacterPlane;
	PlayerCharacter->UpdateFacingDirection(MousePlaneLocation - CharacterLocation, DeltaSeconds);
}

void AKCPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	if (IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
}

void AKCPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	const float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void AKCPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest, float TimeServerReceivedClientRequest)
{
	const float RTT = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
	const float CurrentServerTime = TimeServerReceivedClientRequest - RTT / 2.0f;
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

float AKCPlayerController::GetServerTime() const
{
	return HasAuthority() ? GetWorld()->GetTimeSeconds() : GetWorld()->GetTimeSeconds() + ClientServerDelta;
}

void AKCPlayerController::UploadCustomizationPayload(const TArray<uint8>& Payload)
{
	if (!IsLocalController() ||
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
	for (int32 Offset = 0; Offset < Payload.Num(); Offset += KCCustomizationNetwork::ChunkSizeBytes)
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

void AKCPlayerController::RequestCustomizationPayload(
	AKCPlayerState* TargetPlayerState,
	UKCPlayerCustomizationComponent* TargetComponent)
{
	if (!IsLocalController() || !TargetPlayerState || !TargetComponent)
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

	if (const FKCCachedCustomizationData* CachedData = CustomizationCache.Find(TargetPlayerState);
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
	if (const uint32* PendingRevision = PendingCustomizationRevisions.Find(TargetPlayerState);
		PendingRevision && *PendingRevision == Descriptor.Revision)
	{
		return;
	}
	PendingCustomizationRevisions.Add(TargetPlayerState, Descriptor.Revision);

	if (HasAuthority())
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

void AKCPlayerController::ServerBeginCustomizationUpload_Implementation(
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

void AKCPlayerController::ServerUploadCustomizationChunk_Implementation(
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

void AKCPlayerController::ServerCommitCustomizationUpload_Implementation(const int32 UploadId)
{
	if (ActiveCustomizationUpload.UploadId != UploadId ||
		ActiveCustomizationUpload.Bytes.Num() != ActiveCustomizationUpload.ExpectedBytes ||
		KCCustomizationNetwork::ComputePayloadHash(ActiveCustomizationUpload.Bytes) !=
			ActiveCustomizationUpload.ExpectedHash)
	{
		ActiveCustomizationUpload.Reset();
		return;
	}

	TArray<uint8> CompletedPayload = MoveTemp(ActiveCustomizationUpload.Bytes);
	ActiveCustomizationUpload.Reset();
	if (AKCPlayerState* KCPlayerState = GetPlayerState<AKCPlayerState>())
	{
		KCPlayerState->PublishCustomizationPayload(CompletedPayload);
	}
}

void AKCPlayerController::ServerRequestCustomizationPayload_Implementation(
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
	for (int32 Offset = 0; Offset < Payload.Num(); Offset += KCCustomizationNetwork::ChunkSizeBytes)
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

void AKCPlayerController::ClientBeginCustomizationDownload_Implementation(
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

void AKCPlayerController::ClientReceiveCustomizationChunk_Implementation(
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

void AKCPlayerController::ClientCompleteCustomizationDownload_Implementation(
	AKCPlayerState* TargetPlayerState,
	const uint32 Revision)
{
	if (ActiveCustomizationDownload.PlayerState.Get() != TargetPlayerState ||
		ActiveCustomizationDownload.Revision != Revision ||
		ActiveCustomizationDownload.NextChunkIndex != ActiveCustomizationDownload.ExpectedChunks ||
		ActiveCustomizationDownload.Bytes.Num() != ActiveCustomizationDownload.ExpectedBytes ||
		KCCustomizationNetwork::ComputePayloadHash(ActiveCustomizationDownload.Bytes) !=
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

bool AKCPlayerController::ApplyReceivedCustomization(
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
		Descriptor.ContentHash != KCCustomizationNetwork::ComputePayloadHash(Payload))
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

	FKCCachedCustomizationData& CachedData = CustomizationCache.FindOrAdd(TargetPlayerState);
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

UKCPlayerCustomizationComponent* AKCPlayerController::ResolveCustomizationComponent(
	AKCPlayerState* TargetPlayerState) const
{
	APawn* TargetPawn = TargetPlayerState ? TargetPlayerState->GetPawn() : nullptr;
	return TargetPawn
		? TargetPawn->FindComponentByClass<UKCPlayerCustomizationComponent>()
		: nullptr;
}

void AKCPlayerController::ResetCustomizationDownload(AKCPlayerState* TargetPlayerState)
{
	if (TargetPlayerState)
	{
		PendingCustomizationTargets.Remove(TargetPlayerState);
		PendingCustomizationRevisions.Remove(TargetPlayerState);
	}
}
