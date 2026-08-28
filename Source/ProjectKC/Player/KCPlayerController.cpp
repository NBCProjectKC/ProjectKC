#include "Player/KCPlayerController.h"

#include "Math/RotationMatrix.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Player/KCPlayerCharacter.h"
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
