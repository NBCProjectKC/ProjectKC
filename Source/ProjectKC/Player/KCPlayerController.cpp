#include "Player/KCPlayerController.h"

#include "Math/RotationMatrix.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputCoreTypes.h"
#include "Core/LoadingScreen/KCLoadingScreenSubsystem.h"
#include "Customization/KCCustomizationNetworkComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameSystem/KCGameState.h"
#include "GameSystem/KCGameMode.h"
#include "Player/KCPlayerCharacter.h"
#include "ProjectKC/UI/Common/Core/KCLocalPlayerUISubsystem.h"
#include "ProjectKC/UI/Common/Core/KCUISettings.h"
#include "ProjectKC/UI/HUD/Widget/KCHUDWidget.h"
#include "ProjectKC/UI/Result/Screen/KCResultScreen.h"
#include "ProjectKC/Player/KCPlayerState.h"
#include "ProjectKC/Player/Component/KCTeamFeedbackAudioComponent.h"
#include "Messages/KCGameplayTags.h"
#include "Messages/Struct/KCEmptyMessageStruct.h"
#include "Messages/Struct/KCGamePhaseChangedStruct.h"
#include "Engine/Engine.h"
#include "ProjectKC/ProjectKC.h"

static FString GetControllerNetPrefix(const APlayerController* PC)
{
	if (!PC) return TEXT("[Unknown]");
	if (PC->IsLocalController())
	{
		const FWorldContext* Context = (GEngine && PC->GetWorld()) ? GEngine->GetWorldContextFromWorld(PC->GetWorld()) : nullptr;
		// PIE에서 세션 연결 전(NM_Standalone) 구간도 PIEInstance로 클라이언트를 정확히 식별한다.
		if (PC->GetNetMode() == NM_Client || (Context && Context->PIEInstance > 0))
		{
			const int32 PieInstance = Context ? Context->PIEInstance : 1;
			return FString::Printf(TEXT("[Client PIE_%d]"), PieInstance);
		}
		return TEXT("[Server (Host)]");
	}
	return TEXT("[Remote Client on Server]");
}

AKCPlayerController::AKCPlayerController()
{
	CustomizationNetworkComponent = CreateDefaultSubobject<UKCCustomizationNetworkComponent>(
		TEXT("CustomizationNetwork"));
	TeamFeedbackAudioComponent = CreateDefaultSubobject<UKCTeamFeedbackAudioComponent>(
		TEXT("TeamFeedbackAudio"));
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
	
	// 카운트다운/대기 중에는 이동 및 시점 조작 잠금 (Playing 페이즈 진입 시 해제)
	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);

	if (UKCLoadingScreenSubsystem* LSS = GetGameInstance()->GetSubsystem<UKCLoadingScreenSubsystem>())
	{
		// 월드 감지 및 프리로드 완료 보장을 위해 등록 (HUD 생성은 Playing 페이즈로 이관)
		LSS->RunAfterLoadingScreenHidden(this, FSimpleDelegate());
	}

	LoadingScreenHiddenListenerHandle =
		UGameplayMessageSubsystem::Get(this).RegisterListener<FKCEmptyMessageStruct>(
			KCGameplayTags::Message_LoadingScreen_Hidden,
			this,
			&ThisClass::HandleLoadingScreenHidden);

	GamePhaseChangedListenerHandle =
		UGameplayMessageSubsystem::Get(this).RegisterListener<FKCGamePhaseChangedStruct>(
			KCGameplayTags::Message_Game_PhaseChanged,
			this,
			&ThisClass::HandleGamePhaseChanged);
	
	if (const UWorld* World = GetWorld())
	{
		if (const AKCGameState* GameState = World->GetGameState<AKCGameState>())
		{
			if (GameState->GetGamePhase() == EKCGamePhaseType::Playing)
			{
				InitializeInGameHUD();
				SetIgnoreMoveInput(false);
				SetIgnoreLookInput(false);
			}
			else if (GameState->GetGamePhase() == EKCGamePhaseType::Ending)
			{
				ShowResultScreen();
			}
		}
	}
}

void AKCPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	MessageSubsystem.UnregisterListener(LoadingScreenHiddenListenerHandle);
	MessageSubsystem.UnregisterListener(GamePhaseChangedListenerHandle);

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

void AKCPlayerController::ShowResultScreen()
{
	if (!IsLocalController())
	{
		return;
	}

	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!LocalPlayer)
	{
		UE_LOG(LogTemp, Warning, TEXT("KC Result failed: LocalPlayer is null on %s."), *GetName());
		return;
	}

	UKCLocalPlayerUISubsystem* UISubsystem = LocalPlayer->GetSubsystem<UKCLocalPlayerUISubsystem>();
	if (!UISubsystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("KC Result failed: KCLocalPlayerUISubsystem is null on %s."), *GetName());
		return;
	}

	const UKCUISettings* UISettings = GetDefault<UKCUISettings>();
	const TSubclassOf<UKCUserWidget> ResultScreenClass = UISettings ? UISettings->ResultScreenClass.LoadSynchronous() : nullptr;
	if (!ResultScreenClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("KC Result failed: ResultScreenClass is not configured in ProjectKC UI settings."));
		return;
	}

	UKCUserWidget* ResultWidget = UISubsystem->SetScreenWidget(ResultScreenClass);
	if (UKCResultScreen* ResultScreen = Cast<UKCResultScreen>(ResultWidget))
	{
		ResultScreen->RefreshResultScreen();
	}

	FInputModeUIOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor = true;
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

	if (InputComponent)
	{
		InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AKCPlayerController::ToggleEscMenu);
	}
}

void AKCPlayerController::ToggleEscMenu()
{
	if (!IsLocalController())
	{
		return;
	}

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UKCLocalPlayerUISubsystem* UISubsystem = LocalPlayer->GetSubsystem<UKCLocalPlayerUISubsystem>())
		{
			UISubsystem->ToggleEscMenu(true);
		}
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
		PlayerCharacter->UpdateCameraLookAhead(FVector::ZeroVector, DeltaSeconds);
		return;
	}

	if (FMath::IsNearlyZero(MouseWorldDirection.Z))
	{
		PlayerCharacter->UpdateCameraLookAhead(FVector::ZeroVector, DeltaSeconds);
		return;
	}

	const FVector CharacterLocation = PlayerCharacter->GetActorLocation();
	const float DistanceToCharacterPlane =
		(CharacterLocation.Z - MouseWorldLocation.Z) / MouseWorldDirection.Z;
	if (DistanceToCharacterPlane <= 0.0f)
	{
		PlayerCharacter->UpdateCameraLookAhead(FVector::ZeroVector, DeltaSeconds);
		return;
	}

	const FVector MousePlaneLocation =
		MouseWorldLocation + MouseWorldDirection * DistanceToCharacterPlane;
	const FVector CursorWorldOffset = MousePlaneLocation - CharacterLocation;
	PlayerCharacter->UpdateFacingDirection(CursorWorldOffset, DeltaSeconds);
	PlayerCharacter->UpdateCameraLookAhead(CursorWorldOffset, DeltaSeconds);
}

void AKCPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	if (IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
}

void AKCPlayerController::AcknowledgePossession(APawn* P)
{
	Super::AcknowledgePossession(P);

	if (IsLocalController())
	{
		// 카운트다운 전까지는 이동 및 시점 조작 잠금 유지
		SetIgnoreMoveInput(true);
		SetIgnoreLookInput(true);

		UE_LOG(LogKCGameSystem, Warning, TEXT("%s [PlayerController] AcknowledgePossession 완료 (Pawn: %s) -> 로딩화면에 3프레임 렌더링 웜업 요청"),
			*GetControllerNetPrefix(this), *GetNameSafe(P));
		if (UKCLoadingScreenSubsystem* LSS = GetGameInstance()->GetSubsystem<UKCLoadingScreenSubsystem>())
		{
			LSS->NotifyPlayerReady(this);
		}
	}
}

void AKCPlayerController::NotifyLocalLoadingAndWarmupComplete()
{
	UE_LOG(LogKCGameSystem, Warning, TEXT("%s [PlayerController] 3프레임 렌더링 웜업 완료 수신 -> Server_ReportLoadingComplete() 전송"),
		*GetControllerNetPrefix(this));
	Server_ReportLoadingComplete();
}

void AKCPlayerController::Server_ReportLoadingComplete_Implementation()
{
	UE_LOG(LogKCGameSystem, Warning, TEXT("[Server] [PlayerController] Server_ReportLoadingComplete RPC 수신됨 (From: %s)"), *GetName());
	if (AKCGameMode* GM = GetWorld()->GetAuthGameMode<AKCGameMode>())
	{
		GM->ReportPlayerLoadingComplete(this);
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

void AKCPlayerController::Client_NotifyAllPlayersReady_Implementation(float DisplayDuration)
{
	UE_LOG(LogKCGameSystem, Warning, TEXT("%s [PlayerController] Client_NotifyAllPlayersReady 수신 (노출 시간: %.2f초) -> 로딩화면에 '준비 완료!' 및 화면 닫기 지시"),
		*GetControllerNetPrefix(this), DisplayDuration);
	if (UKCLoadingScreenSubsystem* LSS = GetGameInstance()->GetSubsystem<UKCLoadingScreenSubsystem>())
	{
		LSS->NotifyAllPlayersReady(DisplayDuration);
	}
}

void AKCPlayerController::HandleLoadingScreenHidden(FGameplayTag Channel, const FKCEmptyMessageStruct& Message)
{
	UE_LOG(LogKCGameSystem, Warning, TEXT("%s [PlayerController] 로딩 화면 완전히 닫힘 (Message_LoadingScreen_Hidden 수신)"),
		*GetControllerNetPrefix(this));
}

void AKCPlayerController::HandleGamePhaseChanged(FGameplayTag Channel, const FKCGamePhaseChangedStruct& Message)
{
	UE_LOG(LogKCGameSystem, Warning, TEXT("%s [PlayerController] HandleGamePhaseChanged 수신: %d"),
		*GetControllerNetPrefix(this), static_cast<int32>(Message.NewPhase));

	if (Message.NewPhase == EKCGamePhaseType::Playing)
	{
		InitializeInGameHUD();
		SetIgnoreMoveInput(false);
		SetIgnoreLookInput(false);
		UE_LOG(LogKCGameSystem, Warning, TEXT("%s [PlayerController] Game Start! (Playing 페이즈) -> HUD 활성화 및 이동 조작 잠금 해제"),
			*GetControllerNetPrefix(this));
	}
	else if (Message.NewPhase == EKCGamePhaseType::Ending)
	{
		ShowResultScreen();
	}
}

void AKCPlayerController::RequestSkipResultScreen()
{
	Server_RequestSkipResultScreen();
}

void AKCPlayerController::Server_RequestSkipResultScreen_Implementation()
{
	if (AKCGameMode* GM = GetWorld()->GetAuthGameMode<AKCGameMode>())
	{
		GM->RequestEarlyTravelToLobby(GetPlayerState<AKCPlayerState>());
	}
}

void AKCPlayerController::Client_ShowResultToLobbyLoadingScreen_Implementation()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UKCLoadingScreenSubsystem* LoadingScreenSubsystem = GI->GetSubsystem<UKCLoadingScreenSubsystem>())
		{
			LoadingScreenSubsystem->BeginPreload(EKCLevelType::LobbyLevel);
		}
	}
}
