/**
 * @file KCLobbyPlayerController.cpp
 * @brief AKCLobbyPlayerController 구현부
 */

#include "ProjectKC/Lobby/KCLobbyPlayerController.h"
#include "ProjectKC/Customization/KCCustomizationNetworkComponent.h"
#include "ProjectKC/Customization/KCCustomizationSaveSubsystem.h"
#include "ProjectKC/Lobby/KCLobbyCharacter.h"
#include "ProjectKC/Lobby/UI/KCLobbyWidget.h"
#include "ProjectKC/Player/Component/KCPlayerCustomizationComponent.h"
#include "ProjectKC/Player/KCPlayerState.h"
#include "ProjectKC/GameSystem/KCLobbyGameMode.h"
#include "ProjectKC/Lobby/KCSessionSubsystem.h"
#include "UI/Common/Core/KCUISettings.h"
#include "ProjectKC/ProjectKC.h"
#include "Blueprint/UserWidget.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Core/LoadingScreen/KCLoadingScreenSubsystem.h"
#include "GameSystem/KCLevelTypeLibrary.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "Messages/KCGameplayTags.h"
#include "Messages/Struct/KCEmptyMessageStruct.h"
#include "Painting/PaintingModeControllerComponent.h"
#include "Painting/RuntimeMeshPaintTargetComponent.h"
#include "Widgets/ColorPickerPanelWidget.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"

AKCLobbyPlayerController::AKCLobbyPlayerController()
{
	CustomizationNetworkComponent = CreateDefaultSubobject<UKCCustomizationNetworkComponent>(
		TEXT("CustomizationNetwork"));
	CustomizationPaintingController = CreateDefaultSubobject<UPaintingModeControllerComponent>(
		TEXT("CustomizationPaintingController"));

	const UPaintingModeControllerComponent* PluginPaintingDefaults =
		GetDefault<UPaintingModeControllerComponent>();
	if (PluginPaintingDefaults)
	{
		CustomizationPaintActionAsset = PluginPaintingDefaults->PaintAction;
		CustomizationMouseDeltaActionAsset =
			PluginPaintingDefaults->MouseDeltaAction;
		CustomizationAdjustBrushSizeActionAsset =
			PluginPaintingDefaults->AdjustBrushSizeAction;
	}

	CustomizationPaintingController->ControlMode =
		EPaintingModeControllerControlMode::Simple;
	CustomizationPaintingController->bAutoRegister = false;
	CustomizationPaintingController->ColorPickerWidgetZOrder = 30;
	ConfigureCustomizationPaintingController();
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void AKCLobbyPlayerController::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	ConfigureCustomizationPaintingController();
}

void AKCLobbyPlayerController::ConfigureCustomizationPaintingController()
{
	if (!CustomizationPaintingController)
	{
		return;
	}

	// 로비 커스터마이징은 UI 버튼으로만 시작합니다. 플러그인의 전체 기본 입력
	// 로드는 끄되, Simple 모드에 필요한 입력만 프로젝트 소유 참조로 공급합니다.
	CustomizationPaintingController->bAutoCreateColorPickerWidget = true;
	CustomizationPaintingController->bLoadDefaultInputAssets = false;
	CustomizationPaintingController->PaintAction = CustomizationPaintActionAsset;
	CustomizationPaintingController->MouseDeltaAction =
		CustomizationMouseDeltaActionAsset;
	CustomizationPaintingController->AdjustBrushSizeAction =
		CustomizationAdjustBrushSizeActionAsset;
	CustomizationPaintingController->TogglePaintingModeAction = nullptr;
	CustomizationPaintingController->PaintingToggleInputMappingContext = nullptr;
}

bool AKCLobbyPlayerController::ValidateCustomizationPaintingSetup() const
{
	if (!CustomizationPaintingController)
	{
		UE_LOG(LogKCLobby, Error,
			TEXT("[KCLobbyPlayerController] Customization painting setup invalid: Controller is null"));
		return false;
	}

	const bool bHasRequiredAssets =
		IsValid(CustomizationPaintActionAsset) &&
		IsValid(CustomizationMouseDeltaActionAsset) &&
		IsValid(CustomizationAdjustBrushSizeActionAsset);
	const bool bControllerUsesRequiredAssets =
		CustomizationPaintingController->PaintAction ==
			CustomizationPaintActionAsset &&
		CustomizationPaintingController->MouseDeltaAction ==
			CustomizationMouseDeltaActionAsset &&
		CustomizationPaintingController->AdjustBrushSizeAction ==
			CustomizationAdjustBrushSizeActionAsset;
	if (!bHasRequiredAssets || !bControllerUsesRequiredAssets)
	{
		UE_LOG(LogKCLobby, Error,
			TEXT("[KCLobbyPlayerController] Customization painting setup invalid: "
				"Paint=%s, MouseDelta=%s, AdjustBrushSize=%s, Bound=%s"),
			*GetNameSafe(CustomizationPaintActionAsset),
			*GetNameSafe(CustomizationMouseDeltaActionAsset),
			*GetNameSafe(CustomizationAdjustBrushSizeActionAsset),
			bControllerUsesRequiredAssets ? TEXT("true") : TEXT("false"));
		return false;
	}

	if (!CustomizationPaintingController->bAutoCreateColorPickerWidget ||
		!CustomizationPaintingController->ColorPickerWidgetClass.Get())
	{
		UE_LOG(LogKCLobby, Error,
			TEXT("[KCLobbyPlayerController] Customization painting setup invalid: "
				"ColorPicker auto creation or widget class is missing"));
		return false;
	}

	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!LocalPlayer ||
		!LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
	{
		UE_LOG(LogKCLobby, Error,
			TEXT("[KCLobbyPlayerController] Customization painting setup invalid: "
				"Enhanced Input local player subsystem is unavailable"));
		return false;
	}

	return true;
}

void AKCLobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	UE_LOG(LogKCLobby, Warning, TEXT("[KCLobbyPlayerController] BeginPlay 진입 -> RunAfterLoadingScreenHidden에 SetupLobbyUI 전달"));

	if (UKCLoadingScreenSubsystem* LSS = GetGameInstance()->GetSubsystem<UKCLoadingScreenSubsystem>())
	{
		LSS->RunAfterLoadingScreenHidden(this, FSimpleDelegate::CreateUObject(this, &AKCLobbyPlayerController::SetupLobbyUI));
	}
	
	/*
	// 리스너 등록 - 로딩화면이 아직 안 끝났으면 이 경로로 로비 UI 표시
	UGameplayMessageSubsystem::Get(this).RegisterListener<FKCEmptyMessageStruct>(
		KCGameplayTags::Message_LoadingScreen_Hidden, this, &AKCLobbyPlayerController::HandleLoadingScreenHidden);

	// 메시지가 이미 지나갔을 수 있으니, 로딩화면이 이미 없으면 즉시 로비 UI 표시
	bool bShowedImmediately = false;
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UKCLoadingScreenSubsystem* LoadingScreenSubsystem = GI->GetSubsystem<UKCLoadingScreenSubsystem>())
		{
			if (!LoadingScreenSubsystem->ActiveLoadingWidget)
			{
				SetupLobbyUI();
				bShowedImmediately = true;
			}
		}
	}
*/
	
//	UE_LOG(LogKCLobby, Warning, TEXT("[KC_DEBUG12] BeginPlay - 즉시 표시 여부: %s (false면 로딩화면 종료 대기 중)"),
//		bShowedImmediately ? TEXT("TRUE") : TEXT("FALSE"));

	RefreshLobbyCustomizationPresentations();
	RefreshLobbyCustomizationPresentations();
}

void AKCLobbyPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent)
	{
		InputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &AKCLobbyPlayerController::HandleEnterKey);
	}
}

void AKCLobbyPlayerController::HandleEnterKey()
{
	if (!IsLocalController())
	{
		return;
	}

	if (LobbyWidgetInstance)
	{
		LobbyWidgetInstance->FocusChatInput();
	}
}

void AKCLobbyPlayerController::ResetFocusToGame()
{
	if (!IsLocalController())
	{
		return;
	}

	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	if (LobbyWidgetInstance)
	{
		if (TSharedPtr<SWidget> SafeWidget = LobbyWidgetInstance->GetCachedWidget())
		{
			InputMode.SetWidgetToFocus(SafeWidget);
		}
		LobbyWidgetInstance->SetFocus();
	}
	SetInputMode(InputMode);
}

void AKCLobbyPlayerController::PlayerTick(const float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if (!bCustomizationEditing || !CustomizationCameraActor)
	{
		return;
	}

	if (IsInputKeyDown(EKeys::RightMouseButton))
	{
		float MouseDeltaX = 0.0f;
		float MouseDeltaY = 0.0f;
		GetInputMouseDelta(MouseDeltaX, MouseDeltaY);
		OrbitCustomizationCamera(MouseDeltaX, MouseDeltaY);
	}

	const float MouseWheelDelta =
		GetInputAnalogKeyState(EKeys::MouseWheelAxis);
	if (!FMath::IsNearlyZero(MouseWheelDelta))
	{
		ZoomCustomizationCamera(MouseWheelDelta);
	}
}

void AKCLobbyPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CloseCustomizationEditingSession();
	if (CustomizationNetworkComponent)
	{
		CustomizationNetworkComponent->ResetTransientCustomizationData();
	}
	Super::EndPlay(EndPlayReason);
}

void AKCLobbyPlayerController::PostSeamlessTravel()
{
	CloseCustomizationEditingSession();
	if (CustomizationNetworkComponent)
	{
		CustomizationNetworkComponent->ResetTransientCustomizationData();
	}
	Super::PostSeamlessTravel();
	SetupLobbyUI();
	RefreshLobbyCustomizationPresentations();
}

void AKCLobbyPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	if (IsLocalPlayerController() && LobbyWidgetInstance)
	{
		LobbyWidgetInstance->TryBindPlayerState();
	}

	RefreshLobbyCustomizationPresentations();
}

bool AKCLobbyPlayerController::BeginCustomizationEditing()
{
	if (bCustomizationEditing)
	{
		return true;
	}

	if (!IsLocalPlayerController() ||
		UKCLevelTypeLibrary::GetLevelTypeFromWorld(GetWorld()) !=
			EKCLevelType::LobbyLevel ||
		!CustomizationPaintingController)
	{
		LastCustomizationEditingResult =
			EKCCustomizationSaveResult::InvalidPaintTarget;
		return false;
	}

	ConfigureCustomizationPaintingController();
	if (!ValidateCustomizationPaintingSetup())
	{
		LastCustomizationEditingResult =
			EKCCustomizationSaveResult::InvalidPaintTarget;
		return false;
	}

	AKCLobbyCharacter* TargetCharacter = ResolveLocalCustomizationCharacter();
	UKCPlayerCustomizationComponent* TargetComponent = TargetCharacter
		? TargetCharacter->GetPlayerCustomizationComponent()
		: nullptr;
	if (!TargetComponent || !TargetComponent->BeginLocalCustomizationEditing())
	{
		LastCustomizationEditingResult =
			EKCCustomizationSaveResult::InvalidPaintTarget;
		return false;
	}

	URuntimeMeshPaintTargetComponent* PaintTarget =
		TargetComponent->GetRuntimePaintTarget();
	UKCCustomizationSaveSubsystem* SaveSubsystem =
		GetCustomizationSaveSubsystem();
	if (!PaintTarget || !SaveSubsystem)
	{
		TargetComponent->EndLocalCustomizationEditing();
		LastCustomizationEditingResult =
			EKCCustomizationSaveResult::InvalidPaintTarget;
		return false;
	}

	bool bSaveFound = false;
	bool bUseDefaultAppearance = true;
	if (!SaveSubsystem->LoadCustomization(
		PaintTarget,
		bSaveFound,
		bUseDefaultAppearance,
		LastCustomizationEditingResult))
	{
		TargetComponent->EndLocalCustomizationEditing();
		return false;
	}

	CustomizationEditingComponent = TargetComponent;
	CustomizationEditingPaintTarget = PaintTarget;
	if (!OpenCustomizationCamera(TargetCharacter))
	{
		CloseCustomizationEditingSession();
		LastCustomizationEditingResult =
			EKCCustomizationSaveResult::ApplyFailed;
		return false;
	}

	CustomizationPaintingController->SetPaintTargetComponent(PaintTarget);
	if (!CustomizationPaintingController->EnterPaintingMode())
	{
		CloseCustomizationEditingSession();
		LastCustomizationEditingResult =
			EKCCustomizationSaveResult::ApplyFailed;
		return false;
	}

	bCustomizationEditing = true;
	if (UWidgetComponent* PlayerInfoWidget =
		TargetCharacter->FindComponentByClass<UWidgetComponent>())
	{
		PlayerInfoWidget->SetVisibility(false, true);
	}
	ServerSetCustomizationEditing(true);
	UE_LOG(LogKCLobby, Log,
		TEXT("[KCLobbyPlayerController] Customization editing started: Target=%s, SaveFound=%s"),
		*GetNameSafe(PaintTarget),
		bSaveFound ? TEXT("true") : TEXT("false"));
	return true;
}

bool AKCLobbyPlayerController::SaveCustomizationEditing()
{
	if (!bCustomizationEditing ||
		!CustomizationEditingComponent ||
		!CustomizationEditingPaintTarget ||
		!CustomizationPaintingController)
	{
		LastCustomizationEditingResult =
			EKCCustomizationSaveResult::InvalidPaintTarget;
		return false;
	}

	CustomizationPaintingController->ExitPaintingMode();
	CustomizationEditingPaintTarget->FlushPendingPaintPatchCaptures();
	const bool bUseDefaultAppearance =
		CustomizationEditingPaintTarget->GetPaintPatchHistoryEntryCount() == 0;
	UKCCustomizationSaveSubsystem* SaveSubsystem =
		GetCustomizationSaveSubsystem();
	if (!SaveSubsystem || !SaveSubsystem->SaveCustomization(
		CustomizationEditingPaintTarget,
		bUseDefaultAppearance,
		LastCustomizationEditingResult))
	{
		if (!CustomizationPaintingController->EnterPaintingMode())
		{
			CloseCustomizationEditingSession();
		}
		return false;
	}

	UKCPlayerCustomizationComponent* SavedComponent =
		CustomizationEditingComponent;
	CloseCustomizationEditingSession();
	const bool bApplied =
		SavedComponent && SavedComponent->ApplyLocalSavedCustomization();
	if (!bApplied && SavedComponent)
	{
		LastCustomizationEditingResult = SavedComponent->LastApplyResult;
	}

	UE_LOG(LogKCLobby, Log,
		TEXT("[KCLobbyPlayerController] Customization editing saved: Applied=%s, Default=%s"),
		bApplied ? TEXT("true") : TEXT("false"),
		bUseDefaultAppearance ? TEXT("true") : TEXT("false"));
	return bApplied;
}

bool AKCLobbyPlayerController::ResetCustomizationEditing()
{
	if (!bCustomizationEditing ||
		!CustomizationEditingPaintTarget ||
		!CustomizationPaintingController)
	{
		LastCustomizationEditingResult =
			EKCCustomizationSaveResult::InvalidPaintTarget;
		return false;
	}

	CustomizationPaintingController->ExitPaintingMode();
	UKCCustomizationSaveSubsystem* SaveSubsystem =
		GetCustomizationSaveSubsystem();
	const bool bReset = SaveSubsystem && SaveSubsystem->ResetCustomization(
		CustomizationEditingPaintTarget,
		LastCustomizationEditingResult);
	if (!bReset)
	{
		if (!CustomizationPaintingController->EnterPaintingMode())
		{
			CloseCustomizationEditingSession();
		}
		return false;
	}

	if (!CustomizationPaintingController->EnterPaintingMode())
	{
		LastCustomizationEditingResult =
			EKCCustomizationSaveResult::ApplyFailed;
		CloseCustomizationEditingSession();
		return false;
	}

	return true;
}

bool AKCLobbyPlayerController::CancelCustomizationEditing()
{
	if (!bCustomizationEditing || !CustomizationEditingPaintTarget)
	{
		LastCustomizationEditingResult =
			EKCCustomizationSaveResult::InvalidPaintTarget;
		return false;
	}

	CustomizationPaintingController->ExitPaintingMode();
	bool bSaveFound = false;
	bool bUseDefaultAppearance = true;
	UKCCustomizationSaveSubsystem* SaveSubsystem =
		GetCustomizationSaveSubsystem();
	const bool bRestored = SaveSubsystem && SaveSubsystem->LoadCustomization(
		CustomizationEditingPaintTarget,
		bSaveFound,
		bUseDefaultAppearance,
		LastCustomizationEditingResult);
	CloseCustomizationEditingSession();
	return bRestored;
}

void AKCLobbyPlayerController::OrbitCustomizationCamera(
	const float DeltaYaw,
	const float DeltaPitch)
{
	if (!bCustomizationEditing || !CustomizationCameraActor)
	{
		return;
	}

	CustomizationCameraYaw += DeltaYaw * CustomizationCameraOrbitSensitivity;
	const float SafeMinimumPitch = FMath::Min(
		CustomizationCameraMinimumPitch,
		CustomizationCameraMaximumPitch);
	const float SafeMaximumPitch = FMath::Max(
		CustomizationCameraMinimumPitch,
		CustomizationCameraMaximumPitch);
	CustomizationCameraPitch = FMath::Clamp(
		CustomizationCameraPitch +
			DeltaPitch * CustomizationCameraOrbitSensitivity,
		SafeMinimumPitch,
		SafeMaximumPitch);
	UpdateCustomizationCameraTransform();
}

void AKCLobbyPlayerController::ZoomCustomizationCamera(const float ZoomDelta)
{
	if (!bCustomizationEditing || !CustomizationCameraActor)
	{
		return;
	}

	const float SafeMinimumDistance = FMath::Min(
		CustomizationCameraMinimumDistance,
		CustomizationCameraMaximumDistance);
	const float SafeMaximumDistance = FMath::Max(
		CustomizationCameraMinimumDistance,
		CustomizationCameraMaximumDistance);
	CustomizationCameraDistance = FMath::Clamp(
		CustomizationCameraDistance -
			ZoomDelta * CustomizationCameraZoomSensitivity,
		SafeMinimumDistance,
		SafeMaximumDistance);
	UpdateCustomizationCameraTransform();
}

void AKCLobbyPlayerController::ResetCustomizationCamera()
{
	if (!CustomizationCameraActor || !CustomizationCameraTarget)
	{
		return;
	}

	CustomizationCameraYaw =
		CustomizationCameraTarget->GetActorRotation().Yaw + 180.0f;
	CustomizationCameraPitch = FMath::Clamp(
		CustomizationCameraInitialPitch,
		FMath::Min(
			CustomizationCameraMinimumPitch,
			CustomizationCameraMaximumPitch),
		FMath::Max(
			CustomizationCameraMinimumPitch,
			CustomizationCameraMaximumPitch));
	CustomizationCameraDistance = FMath::Clamp(
		CustomizationCameraInitialDistance,
		FMath::Min(
			CustomizationCameraMinimumDistance,
			CustomizationCameraMaximumDistance),
		FMath::Max(
			CustomizationCameraMinimumDistance,
			CustomizationCameraMaximumDistance));
	UpdateCustomizationCameraTransform();
}

AKCLobbyCharacter*
AKCLobbyPlayerController::ResolveLocalCustomizationCharacter() const
{
	if (!PlayerState)
	{
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<AKCLobbyCharacter> CharacterIterator(World);
		CharacterIterator;
		++CharacterIterator)
	{
		if (CharacterIterator->GetPlayerInfo().PlayerState.Get() == PlayerState)
		{
			return *CharacterIterator;
		}
	}

	return nullptr;
}

UKCCustomizationSaveSubsystem*
AKCLobbyPlayerController::GetCustomizationSaveSubsystem() const
{
	const UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance
		? GameInstance->GetSubsystem<UKCCustomizationSaveSubsystem>()
		: nullptr;
}

bool AKCLobbyPlayerController::OpenCustomizationCamera(
	AKCLobbyCharacter* TargetCharacter)
{
	if (!IsLocalPlayerController() || !TargetCharacter)
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	PreviousCustomizationViewTarget = GetViewTarget();
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.ObjectFlags = RF_Transient;
	SpawnParameters.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	CustomizationCameraActor = World->SpawnActor<ACameraActor>(
		ACameraActor::StaticClass(),
		FTransform::Identity,
		SpawnParameters);
	if (!CustomizationCameraActor)
	{
		PreviousCustomizationViewTarget = nullptr;
		return false;
	}

	CustomizationCameraActor->SetReplicates(false);
	CustomizationCameraActor->SetActorEnableCollision(false);
	if (UCameraComponent* CameraComponent =
		CustomizationCameraActor->GetCameraComponent())
	{
		CameraComponent->SetFieldOfView(CustomizationCameraFieldOfView);
	}

	CustomizationCameraTarget = TargetCharacter;
	ResetCustomizationCamera();
	SetViewTargetWithBlend(
		CustomizationCameraActor,
		CustomizationCameraBlendTime,
		EViewTargetBlendFunction::VTBlend_EaseInOut);
	return true;
}

void AKCLobbyPlayerController::UpdateCustomizationCameraTransform()
{
	if (!CustomizationCameraActor || !CustomizationCameraTarget)
	{
		return;
	}

	FVector BoundsOrigin = CustomizationCameraTarget->GetActorLocation();
	TArray<UStaticMeshComponent*> StaticMeshComponents;
	CustomizationCameraTarget->GetComponents(StaticMeshComponents);
	for (const UStaticMeshComponent* StaticMeshComponent : StaticMeshComponents)
	{
		if (StaticMeshComponent &&
			StaticMeshComponent->GetFName() == TEXT("AvatarBody"))
		{
			BoundsOrigin = StaticMeshComponent->Bounds.Origin;
			break;
		}
	}
	const FVector FocusLocation =
		BoundsOrigin + CustomizationCameraFocusOffset;
	const FRotator ViewRotation(
		CustomizationCameraPitch,
		CustomizationCameraYaw,
		0.0f);
	const FVector CameraLocation =
		FocusLocation - ViewRotation.Vector() * CustomizationCameraDistance;
	CustomizationCameraActor->SetActorLocationAndRotation(
		CameraLocation,
		(FocusLocation - CameraLocation).Rotation());
}

void AKCLobbyPlayerController::CloseCustomizationCamera()
{
	if (IsLocalPlayerController() && PreviousCustomizationViewTarget)
	{
		SetViewTarget(PreviousCustomizationViewTarget);
	}

	if (CustomizationCameraActor)
	{
		CustomizationCameraActor->Destroy();
	}

	CustomizationCameraActor = nullptr;
	CustomizationCameraTarget = nullptr;
	PreviousCustomizationViewTarget = nullptr;
}

void AKCLobbyPlayerController::CloseCustomizationEditingSession()
{
	const bool bWasCustomizationEditing = bCustomizationEditing;
	if (CustomizationCameraTarget)
	{
		if (UWidgetComponent* PlayerInfoWidget =
			CustomizationCameraTarget->FindComponentByClass<UWidgetComponent>())
		{
			PlayerInfoWidget->SetVisibility(true, true);
		}
	}

	if (CustomizationPaintingController)
	{
		CustomizationPaintingController->ExitPaintingMode();
		CustomizationPaintingController->ClearPaintTargetComponents();
	}
	if (CustomizationEditingComponent)
	{
		CustomizationEditingComponent->EndLocalCustomizationEditing();
	}
	CloseCustomizationCamera();

	CustomizationEditingComponent = nullptr;
	CustomizationEditingPaintTarget = nullptr;
	bCustomizationEditing = false;
	if (bWasCustomizationEditing && IsLocalPlayerController())
	{
		ServerSetCustomizationEditing(false);
	}

	// 페인팅 플러그인이 GameOnly로 바꾼 입력 모드를 로비 전용 GameAndUI로 복구
	if (IsLocalPlayerController())
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);
		bShowMouseCursor = true;
		bEnableClickEvents = true;
		bEnableMouseOverEvents = true;
	}
}

void AKCLobbyPlayerController::RefreshLobbyCustomizationPresentations()
{
	if (!IsLocalPlayerController())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<AKCLobbyCharacter> CharacterIterator(World);
		CharacterIterator;
		++CharacterIterator)
	{
		CharacterIterator->RefreshCustomizationPresentation();
	}
}

void AKCLobbyPlayerController::SetupLobbyUI()
{
	if (!IsLocalPlayerController())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UE_LOG(LogKCLobby, Warning, TEXT("[KCLobbyPlayerController] SetupLobbyUI 진입 (콜백 실행됨)"));

	// L_LobbyLevel 레벨에 있을 때만 로비 UI 생성
	const FString MapName = World->GetMapName();
	if (UKCLevelTypeLibrary::GetLevelTypeFromWorld(World) != EKCLevelType::LobbyLevel)
	{
		UE_LOG(LogKCLobby, Verbose, TEXT("[KCLobbyPlayerController] SetupLobbyUI skipped: Not in L_LobbyLevel (Current: %s)"), *MapName);
		return;
	}

	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	bShowMouseCursor = true;

	if (!LobbyWidgetClass)
	{
		if (const UKCUISettings* UISettings = GetDefault<UKCUISettings>())
		{
			LobbyWidgetClass = UISettings->LobbyWidgetClass.LoadSynchronous();
		}

		if (!LobbyWidgetClass)
		{
			UE_LOG(LogKCLobby, Error, TEXT("[KCLobbyPlayerController] SetupLobbyUI Failed: LobbyWidgetClass is not set in BP_PC_Lobby or KCUISettings."));
			return;
		}
	}

	if (LobbyWidgetClass && (!LobbyWidgetInstance || !LobbyWidgetInstance->IsInViewport()))
	{
		LobbyWidgetInstance = CreateWidget<UKCLobbyWidget>(this, LobbyWidgetClass);
		if (LobbyWidgetInstance)
		{
			LobbyWidgetInstance->AddToViewport();
			UE_LOG(LogKCLobby, Log, TEXT("[KCLobbyPlayerController] SetupLobbyUI: Created and added WBP_LobbyUI to viewport"));
		}
		else
		{
			UE_LOG(LogKCLobby, Error, TEXT("[KCLobbyPlayerController] SetupLobbyUI Failed: Failed to create LobbyWidgetInstance"));
		}
	}
}

void AKCLobbyPlayerController::ROS_ToggleReadyStatus_Implementation()
{
	const FString PlayerName = PlayerState ? PlayerState->GetPlayerName() : GetName();
	UE_LOG(LogKCLobby, Log, TEXT("[KCLobbyPlayerController] ROS_ToggleReadyStatus received from Player '%s'"), *PlayerName);

	if (UWorld* World = GetWorld())
	{
		if (AKCLobbyGameMode* GM = World->GetAuthGameMode<AKCLobbyGameMode>())
		{
			GM->HandlePlayerReadyToggled(this);
		}
	}
}

void AKCLobbyPlayerController::ROS_RequestMoveToSlot_Implementation(int32 TargetSlotIndex)
{
	const FString PlayerName = PlayerState ? PlayerState->GetPlayerName() : GetName();
	if (bCustomizationEditing)
	{
		UE_LOG(LogKCLobby, Warning,
			TEXT("[KCLobbyPlayerController] ROS_RequestMoveToSlot rejected while Player '%s' is customizing"),
			*PlayerName);
		return;
	}

	if (TargetSlotIndex < 0 || TargetSlotIndex >= AKCLobbyGameMode::MAX_LOBBY_SLOTS)
	{
		UE_LOG(LogKCLobby, Warning, TEXT("[KCLobbyPlayerController] ROS_RequestMoveToSlot: Invalid SlotIndex %d from Player '%s' (Valid: 0-%d)"),
			TargetSlotIndex, *PlayerName, AKCLobbyGameMode::MAX_LOBBY_SLOTS - 1);
		return;
	}

	UE_LOG(LogKCLobby, Log, TEXT("[KCLobbyPlayerController] ROS_RequestMoveToSlot: Player '%s' requested move to Slot %d"),
		*PlayerName, TargetSlotIndex);

	if (UWorld* World = GetWorld())
	{
		if (AKCLobbyGameMode* GM = World->GetAuthGameMode<AKCLobbyGameMode>())
		{
			GM->MovePlayerToSlot(this, TargetSlotIndex);
		}
	}
}

void AKCLobbyPlayerController::ServerSetCustomizationEditing_Implementation(
	const bool bEditing)
{
	bCustomizationEditing = bEditing;
}

void AKCLobbyPlayerController::MoveSlot(int32 TargetSlotIndex)
{
	ROS_RequestMoveToSlot(TargetSlotIndex);
}

void AKCLobbyPlayerController::ROS_UpdatePlayerInfo_Implementation()
{
	UE_LOG(LogKCLobby, Verbose, TEXT("[KCLobbyPlayerController] ROS_UpdatePlayerInfo received from %s"), *GetName());

	if (UWorld* World = GetWorld())
	{
		if (AKCLobbyGameMode* GM = World->GetAuthGameMode<AKCLobbyGameMode>())
		{
			GM->UpdateLobbyReadyState();
		}
	}
}

void AKCLobbyPlayerController::ROS_ApplyGameSettings_Implementation(int32 InPlayerCount, EKCLevelType InMapType, float InMatchDurationSeconds)
{
	UE_LOG(LogKCLobby, Log, TEXT("[KCLobbyPlayerController] ROS_ApplyGameSettings received from %s: Players=%d, Map=%s, Duration=%.0fs"),
		*GetName(), InPlayerCount, *UKCLevelTypeLibrary::GetLevelName(InMapType).ToString(), InMatchDurationSeconds);

	if (UWorld* World = GetWorld())
	{
		if (AKCLobbyGameMode* GM = World->GetAuthGameMode<AKCLobbyGameMode>())
		{
			GM->ApplyGameSettings(InPlayerCount, InMapType, InMatchDurationSeconds);
		}
	}
}

void AKCLobbyPlayerController::Client_OnMatchBegin_Implementation(EKCLevelType TargetMap)
{
	UE_LOG(LogKCLobby, Log, TEXT("[KCLobbyPlayerController] Client_OnMatchBegin received. Playing match start animation and locking inputs."));

	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);

	if (LobbyWidgetInstance)
	{
		LobbyWidgetInstance->PlayMatchStartAnim();
		LobbyWidgetInstance->RemoveFromParent();
	}
	
	// GasRange 진입 준비: 로딩화면 표시 + 에셋 프리로드 시작점
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UKCLoadingScreenSubsystem* LoadingScreenSubsystem = GI->GetSubsystem<UKCLoadingScreenSubsystem>())
		{
			LoadingScreenSubsystem->BeginPreload(TargetMap);
		}
	}
}

void AKCLobbyPlayerController::Client_SetStartGameButtonEnabled_Implementation(bool bEnabled)
{
	UE_LOG(LogKCLobby, Verbose, TEXT("[KCLobbyPlayerController] Client_SetStartGameButtonEnabled: %s"), bEnabled ? TEXT("TRUE") : TEXT("FALSE"));

	if (LobbyWidgetInstance)
	{
		LobbyWidgetInstance->SetStartGameButtonEnabled(bEnabled);
	}
}

/* =========================================================================
 *  로비 채팅 시스템 구현부 (Lobby Chat System Implementation)
 * ========================================================================= */

void AKCLobbyPlayerController::SendChatMessage(const FString& Message)
{
	// 1. 공백 및 빈 문자열 로컬 사전 차단
	const FString TrimmedMessage = Message.TrimStartAndEnd();
	if (TrimmedMessage.IsEmpty())
	{
		return;
	}

	// 2. 글자 수 제한 초과 검사
	if (TrimmedMessage.Len() > MaxChatMessageLength)
	{
		UE_LOG(LogKCLobby, Warning, TEXT("[KCLobbyPlayerController] SendChatMessage Failed: Message exceeds max length (%d > %d)"),
			TrimmedMessage.Len(), MaxChatMessageLength);
		return;
	}

	// 3. 빠른 전송 허용 (쿨타임 제한 해제)
	LastChatMessageTimeSeconds = FPlatformTime::Seconds();

	// 4. 서버로 전송
	Server_SendChatMessage(TrimmedMessage);
}

void AKCLobbyPlayerController::SendChat(const FString& Message)
{
	SendChatMessage(Message);
}

bool AKCLobbyPlayerController::Server_SendChatMessage_Validate(const FString& Message)
{
	// 서버 측 유효성 검증
	const FString Trimmed = Message.TrimStartAndEnd();
	return !Trimmed.IsEmpty() && Trimmed.Len() <= MaxChatMessageLength;
}

void AKCLobbyPlayerController::Server_SendChatMessage_Implementation(const FString& Message)
{
	const FString TrimmedMessage = Message.TrimStartAndEnd();

	// 1. 발신자 닉네임 가져오기 (스팀 프로필 닉네임 자동 반영)
	FString SenderName = TEXT("Unknown");
	if (PlayerState)
	{
		SenderName = PlayerState->GetPlayerName();
	}

	UE_LOG(LogKCLobby, Log, TEXT("[Server Chat] Broadcast from '%s': %s"), *SenderName, *TrimmedMessage);

	// 2. 현재 월드에 접속 중인 모든 PlayerController를 찾아 Client RPC 브로드캐스트
	if (UWorld* World = GetWorld())
	{
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			if (AKCLobbyPlayerController* LobbyPC = Cast<AKCLobbyPlayerController>(It->Get()))
			{
				LobbyPC->Client_ReceiveChatMessage(SenderName, TrimmedMessage);
			}
		}
	}
}

void AKCLobbyPlayerController::Client_ReceiveChatMessage_Implementation(const FString& SenderName, const FString& Message)
{
	// 1. 출력 로그창(Output Log) 출력
	UE_LOG(LogKCLobby, Log, TEXT("[Chat] [%s]: %s"), *SenderName, *Message);

	// 2. [추후 UI 연동용] 승재님의 위젯이 수신할 수 있도록 델리게이트 브로드캐스트
	OnChatMessageReceived.Broadcast(SenderName, Message);
}

void AKCLobbyPlayerController::Client_NotifySessionTerminated_Implementation(const FString& Reason)
{
	UE_LOG(LogKCLobby, Warning, TEXT("[KCLobbyPlayerController] Client_NotifySessionTerminated received: %s"), *Reason);

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UKCSessionSubsystem* SessionSubsystem = GI->GetSubsystem<UKCSessionSubsystem>())
		{
			SessionSubsystem->NotifySessionTerminatedByHost(Reason);
		}
	}
}

void AKCLobbyPlayerController::EndSession()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UKCSessionSubsystem* SessionSubsystem = GI->GetSubsystem<UKCSessionSubsystem>())
		{
			SessionSubsystem->EndSession();
		}
	}
}

void AKCLobbyPlayerController::HandleLoadingScreenHidden(FGameplayTag Channel, const FKCEmptyMessageStruct& Message)
{
	UE_LOG(LogKCLobby, Warning, TEXT("[KCLobbyPlayerController] HandleLoadingScreenHidden 수신 - 이제 로비 UI 표시"));
	SetupLobbyUI();
}

void AKCLobbyPlayerController::LeaveLobby()
{
	if (!IsLocalController())
	{
		return;
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UKCSessionSubsystem* SessionSubsystem = GI->GetSubsystem<UKCSessionSubsystem>())
		{
			if (HasAuthority())
			{
				SessionSubsystem->EndSession();
			}
			else
			{
				SessionSubsystem->ReturnToMainMenu();
			}
		}
	}
}
