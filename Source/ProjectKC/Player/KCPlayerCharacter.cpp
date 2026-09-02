#include "Player/KCPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameplayAbilitySpec.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/MaterialInterface.h"
#include "ProjectKC/AbilitySystem/Ability/KCGA_PlayerDash.h"
#include "ProjectKC/AbilitySystem/Attribute/KCCharacterAttributeSet.h"
#include "ProjectKC/AbilitySystem/Component/KCAbilitySystemComponent.h"
#include "ProjectKC/AbilitySystem/Component/KCKnockbackComponent.h"
#include "ProjectKC/AbilitySystem/Effect/KCGE_StaminaRegen.h"
#include "ProjectKC/AbilitySystem/Tag/KCAbilityGameplayTags.h"
#include "ProjectKC/Item/Component/KCHeldItemComponent.h"
#include "ProjectKC/Item/Definition/KCItemDefinition.h"
#include "ProjectKC/Lobby/KCPlayerSlotActor.h"
#include "ProjectKC/Player/KCPlayerState.h"
#include "ProjectKC/Player/Component/KCEmoteComponent.h"
#include "ProjectKC/Player/Component/KCPlayerCustomizationComponent.h"
#include "ProjectKC/UI/Interaction/Component/KCPlayerInteractionPromptComponent.h"
#include "ProjectKC/UI/World/Player/Component/KCPlayerOverHeadComponent.h"
#include "ProjectKC/UI/World/Player/Struct/KCPlayerDisplayInfoStruct.h"
#include "Player/Interaction/KCPlayerInteractionComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float FacingReplicationInterval = 1.0f / 30.0f;
	constexpr float MinimumFacingReplicationAngle = 0.5f;
	constexpr double MinimumServerFacingUpdateInterval = 1.0 / 60.0;
	constexpr float MinimumFacingYaw = -180.0f;
	constexpr float MaximumFacingYaw = 180.0f;

	bool IsValidFacingYaw(const float FacingYaw)
	{
		return FMath::IsFinite(FacingYaw) &&
			FacingYaw >= MinimumFacingYaw &&
			FacingYaw <= MaximumFacingYaw;
	}

	void ConfigureAvatarPart(UStaticMeshComponent* Component, UStaticMesh* Mesh)
	{
		if (!Component)
		{
			return;
		}

		Component->SetStaticMesh(Mesh);
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Component->SetGenerateOverlapEvents(false);
		Component->SetCanEverAffectNavigation(false);
		Component->SetReceivesDecals(false);
	}
}

AKCPlayerCharacter::AKCPlayerCharacter()
{
	bReplicates = true;
	SetReplicateMovement(true);
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);
	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -96.0f));
	GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshFinder(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	UStaticMesh* SphereMesh = SphereMeshFinder.Succeeded()
		? SphereMeshFinder.Object
		: nullptr;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PillBodyMeshFinder(
		TEXT("/Game/KC/Player/Avatar/SM_PillBody.SM_PillBody"));
	UStaticMesh* PillBodyMesh = SphereMesh;
	if (PillBodyMeshFinder.Succeeded())
	{
		PillBodyMesh = PillBodyMeshFinder.Object;
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> RedMaterialFinder(
		TEXT("/Game/KC/Player/Avatar/MI_Red.MI_Red"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BlueMaterialFinder(
		TEXT("/Game/KC/Player/Avatar/MI_Blue.MI_Blue"));
	if (RedMaterialFinder.Succeeded())
	{
		FKCAvatarTeamAppearanceStruct& Team0Appearance =
			TeamAppearances.AddDefaulted_GetRef();
		Team0Appearance.TeamId = 0;
		Team0Appearance.BodyMaterial = RedMaterialFinder.Object;
		Team0Appearance.HandMaterial = RedMaterialFinder.Object;
		Team0Appearance.FootMaterial = RedMaterialFinder.Object;
	}
	if (BlueMaterialFinder.Succeeded())
	{
		FKCAvatarTeamAppearanceStruct& Team1Appearance =
			TeamAppearances.AddDefaulted_GetRef();
		Team1Appearance.TeamId = 1;
		Team1Appearance.BodyMaterial = BlueMaterialFinder.Object;
		Team1Appearance.HandMaterial = BlueMaterialFinder.Object;
		Team1Appearance.FootMaterial = BlueMaterialFinder.Object;
	}

	AvatarBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AvatarBody"));
	AvatarBody->SetupAttachment(GetMesh(), TEXT("pelvis"));
	AvatarBody->SetRelativeRotation(FRotator(-22.8f, 92.5f, -91.4f));
	AvatarBody->SetRelativeScale3D(FVector(1.0f, 1.0f, 0.86f));
	ConfigureAvatarPart(AvatarBody, PillBodyMesh);

	AvatarHandLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AvatarHandLeft"));
	AvatarHandLeft->SetupAttachment(GetMesh(), TEXT("hand_l"));
	AvatarHandLeft->SetRelativeLocation(FVector(0.0f, 30.0f, 0.0f));
	AvatarHandLeft->SetRelativeScale3D(FVector(0.26f));
	ConfigureAvatarPart(AvatarHandLeft, SphereMesh);

	AvatarHandRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AvatarHandRight"));
	// 스켈레톤의 표준 그립 소켓은 유지하고, 보이는 손이 그 기준을 따른다.
	// 아이템도 같은 소켓에 붙으므로 Grip이 손 중심과 자연스럽게 일치한다.
	AvatarHandRight->SetupAttachment(GetMesh(), TEXT("HandGrip_R"));
	AvatarHandRight->SetRelativeScale3D(FVector(0.26f));
	ConfigureAvatarPart(AvatarHandRight, SphereMesh);

	AvatarFootLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AvatarFootLeft"));
	AvatarFootLeft->SetupAttachment(GetMesh(), TEXT("foot_l"));
	AvatarFootLeft->SetRelativeLocation(FVector(0.0f, -12.0f, -14.0f));
	AvatarFootLeft->SetRelativeScale3D(FVector(0.30f));
	ConfigureAvatarPart(AvatarFootLeft, SphereMesh);

	AvatarFootRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AvatarFootRight"));
	AvatarFootRight->SetupAttachment(GetMesh(), TEXT("foot_r"));
	AvatarFootRight->SetRelativeLocation(FVector(0.0f, -28.0f, -2.0f));
	AvatarFootRight->SetRelativeScale3D(FVector(0.30f));
	ConfigureAvatarPart(AvatarFootRight, SphereMesh);

	FaceAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("FaceAnchor"));
	FaceAnchor->SetupAttachment(AvatarBody);
	FaceAnchor->SetRelativeLocation(FVector(49.0f, 0.0f, 25.0f));

	ConfigureDriverMesh();

	UCharacterMovementComponent* CharacterMovementComponent = GetCharacterMovement();
	CharacterMovementComponent->bOrientRotationToMovement = false;
	CharacterMovementComponent->bUseControllerDesiredRotation = false;

	AbilitySystemComponent =
		CreateDefaultSubobject<UKCAbilitySystemComponent>(TEXT("AbilitySystem"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	CharacterAttributes =
		CreateDefaultSubobject<UKCCharacterAttributeSet>(TEXT("CharacterAttributes"));
	CharacterMovementComponent->MaxWalkSpeed = CharacterAttributes->GetMoveSpeed();
	DashAbilityClass = UKCGA_PlayerDash::StaticClass();
	StaminaRegenEffectClass = UKCGE_StaminaRegen::StaticClass();
	EmoteComponent = CreateDefaultSubobject<UKCEmoteComponent>(TEXT("Emote"));

	HeldItemComponent =
		CreateDefaultSubobject<UKCHeldItemComponent>(TEXT("HeldItem"));
	KnockbackComponent =
		CreateDefaultSubobject<UKCKnockbackComponent>(TEXT("Knockback"));
	PlayerCustomizationComponent =
		CreateDefaultSubobject<UKCPlayerCustomizationComponent>(TEXT("PlayerCustomization"));

	CameraBoomComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoomComponent"));
	CameraBoomComponent->SetupAttachment(RootComponent);
	CameraBoomComponent->SetUsingAbsoluteRotation(true);
	// 탑다운 구도는 유지한 채, 화면의 전방을 월드 Y+ 방향으로 90도 회전한다.
	CameraBoomComponent->SetRelativeRotation(FRotator(-60.0f, 90.0f, 0.0f));
	CameraBoomComponent->TargetArmLength = 1200.0f;
	CameraBoomComponent->bDoCollisionTest = false;
	CameraBoomComponent->bUsePawnControlRotation = false;

	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCameraComponent"));
	TopDownCameraComponent->SetupAttachment(CameraBoomComponent, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false;

	InteractionComponent = CreateDefaultSubobject<UKCPlayerInteractionComponent>(
		TEXT("InteractionComponent"));
	InteractionComponent->SetupAttachment(RootComponent);
	PlayerOverHeadComponent = CreateDefaultSubobject<UKCPlayerOverHeadComponent>(
		TEXT("PlayerOverHead"));
	InteractionPromptComponent =
		CreateDefaultSubobject<UKCPlayerInteractionPromptComponent>(
			TEXT("InteractionPrompt"));

#if WITH_EDITORONLY_DATA
	HeldItemPreviewMesh =
		CreateEditorOnlyDefaultSubobject<UStaticMeshComponent>(
			TEXT("HeldItemPreview"));
	if (HeldItemPreviewMesh)
	{
		HeldItemPreviewMesh->SetupAttachment(
			GetMesh(),
			HeldItemComponent->GetHandSocketName());
		HeldItemPreviewMesh->SetMobility(EComponentMobility::Movable);
		HeldItemPreviewMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		HeldItemPreviewMesh->SetGenerateOverlapEvents(false);
		HeldItemPreviewMesh->SetSimulatePhysics(false);
		HeldItemPreviewMesh->SetCanEverAffectNavigation(false);
		HeldItemPreviewMesh->SetHiddenInGame(true);
		HeldItemPreviewMesh->SetVisibility(false);
	}
#endif
}

UAbilitySystemComponent* AKCPlayerCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AKCPlayerCharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ConfigureDriverMesh();

#if WITH_EDITOR
	ApplyTeamAppearance(PreviewTeamId);
#endif

	RefreshHeldItemPreview();
}

void AKCPlayerCharacter::ConfigureDriverMesh()
{
	USkeletalMeshComponent* DriverMesh = GetMesh();
	if (!DriverMesh)
	{
		return;
	}

	DriverMesh->VisibilityBasedAnimTickOption =
		EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	DriverMesh->SetVisibility(false, false);
	DriverMesh->SetHiddenInGame(true, false);
	DriverMesh->SetCastShadow(false);
}

bool AKCPlayerCharacter::BeginUseHeldItem()
{
	if (!IsLocallyControlled() || !HeldItemComponent)
	{
		return false;
	}

	const bool bUseStarted = HeldItemComponent->PressHeldItemUse();
	if (bUseStarted)
	{
		InterruptEmote();
	}
	return bUseStarted;
}

void AKCPlayerCharacter::EndUseHeldItem()
{
	if (IsLocallyControlled() && HeldItemComponent)
	{
		HeldItemComponent->ReleaseHeldItemUse();
	}
}

void AKCPlayerCharacter::RequestInteract()
{
	if (IsLocallyControlled())
	{
		if (InteractionComponent)
		{
			InteractionComponent->TryInteract();
		}
	}
}

void AKCPlayerCharacter::RequestDropHeldItem()
{
	if (IsLocallyControlled() && HeldItemComponent)
	{
		HeldItemComponent->TryDropHeldItem();
	}
}

UKCAbilitySystemComponent* AKCPlayerCharacter::GetKCAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UKCCharacterAttributeSet* AKCPlayerCharacter::GetCharacterAttributes() const
{
	return CharacterAttributes;
}

UKCEmoteComponent* AKCPlayerCharacter::GetEmoteComponent() const
{
	return EmoteComponent;
}

UKCHeldItemComponent* AKCPlayerCharacter::GetHeldItemComponent() const
{
	return HeldItemComponent;
}

UKCKnockbackComponent* AKCPlayerCharacter::GetKnockbackComponent() const
{
	return KnockbackComponent;
}

UKCPlayerCustomizationComponent* AKCPlayerCharacter::GetPlayerCustomizationComponent() const
{
	return PlayerCustomizationComponent;
}

UKCPlayerInteractionComponent* AKCPlayerCharacter::GetInteractionComponent() const
{
	return InteractionComponent;
}

UKCPlayerInteractionPromptComponent*
AKCPlayerCharacter::GetInteractionPromptComponent() const
{
	return InteractionPromptComponent;
}

void AKCPlayerCharacter::RefreshHeldItemPreview()
{
#if WITH_EDITORONLY_DATA
	if (!HeldItemPreviewMesh)
	{
		return;
	}

	HeldItemPreviewMesh->SetVisibility(false);
	HeldItemPreviewMesh->SetStaticMesh(nullptr);
	HeldItemPreviewMesh->SetRelativeTransform(FTransform::Identity);

	USceneComponent* Attachment = HeldItemComponent
		? HeldItemComponent->GetAttachmentComponent()
		: nullptr;
	const FName HandSocketName = HeldItemComponent
		? HeldItemComponent->GetHandSocketName()
		: NAME_None;
	if (!PreviewItemDefinition || !Attachment || HandSocketName.IsNone() ||
		!Attachment->DoesSocketExist(HandSocketName))
	{
		return;
	}

	FTransform AlignmentTransform;
	if (!PreviewItemDefinition->Presentation.TryGetGripAlignmentTransform(
		AlignmentTransform))
	{
		return;
	}

	const bool bAttached = HeldItemPreviewMesh->IsRegistered()
		? HeldItemPreviewMesh->AttachToComponent(
			Attachment,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			HandSocketName)
		: true;
	if (!HeldItemPreviewMesh->IsRegistered())
	{
		HeldItemPreviewMesh->SetupAttachment(Attachment, HandSocketName);
	}
	if (!bAttached)
	{
		return;
	}

	HeldItemPreviewMesh->SetStaticMesh(
		PreviewItemDefinition->Presentation.StaticMesh);
	HeldItemPreviewMesh->SetRelativeTransform(AlignmentTransform);
	HeldItemPreviewMesh->SetVisibility(true);
#endif
}

#if WITH_EDITOR
void AKCPlayerCharacter::PostEditChangeProperty(
	FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	ApplyTeamAppearance(PreviewTeamId);
	RefreshHeldItemPreview();
}
#endif

void AKCPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	InitializeAbilityActorInfo();
	RefreshTeamAppearanceBinding();
	if (PlayerCustomizationComponent)
	{
		PlayerCustomizationComponent->InitializeForPawn();
	}
}

void AKCPlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindTeamAppearanceFromPlayerState();
	Super::EndPlay(EndPlayReason);
}

void AKCPlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	InitializeAbilityActorInfo();
	RefreshTeamAppearanceBinding();
	if (PlayerCustomizationComponent)
	{
		PlayerCustomizationComponent->InitializeForPawn();
	}
}

void AKCPlayerCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();
	InitializeAbilityActorInfo();
	RefreshTeamAppearanceBinding();
	if (PlayerCustomizationComponent)
	{
		PlayerCustomizationComponent->InitializeForPawn();
	}
}

void AKCPlayerCharacter::OnRep_Owner()
{
	Super::OnRep_Owner();
	RefreshTeamAppearanceBinding();
}

void AKCPlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	RefreshTeamAppearanceBinding();
}

void AKCPlayerCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();
	InitializeAbilityActorInfo();
	RefreshTeamAppearanceBinding();
	if (PlayerCustomizationComponent)
	{
		PlayerCustomizationComponent->InitializeForPawn();
	}
}

void AKCPlayerCharacter::RefreshTeamAppearanceBinding()
{
	AKCPlayerState* TeamPlayerState =
		GetPlayerState<AKCPlayerState>();
	BindTeamAppearanceToPlayerState(TeamPlayerState);

	if (!TeamPlayerState)
	{
		if (const AKCPlayerSlotActor* PlayerSlot =
			Cast<AKCPlayerSlotActor>(GetOwner()))
		{
			ApplyTeamAppearance(PlayerSlot->GetSlotTeamId());
		}
	}
}

void AKCPlayerCharacter::BindTeamAppearanceToPlayerState(
	AKCPlayerState* InPlayerState)
{
	if (BoundTeamPlayerState.Get() != InPlayerState)
	{
		UnbindTeamAppearanceFromPlayerState();
		BoundTeamPlayerState = InPlayerState;

		if (InPlayerState)
		{
			InPlayerState->OnTeamIdChanged.AddUniqueDynamic(
				this,
				&AKCPlayerCharacter::HandleTeamIdChanged);
			InPlayerState->OnGamePlayerNameChanged.AddUniqueDynamic(
				this,
				&AKCPlayerCharacter::HandleGamePlayerNameChanged);
		}
	}

	if (InPlayerState)
	{
		ApplyTeamAppearance(InPlayerState->GetTeamId());
	}

	RefreshPlayerOverHead();
}

void AKCPlayerCharacter::UnbindTeamAppearanceFromPlayerState()
{
	if (AKCPlayerState* TeamPlayerState = BoundTeamPlayerState.Get())
	{
		TeamPlayerState->OnTeamIdChanged.RemoveDynamic(
			this,
			&AKCPlayerCharacter::HandleTeamIdChanged);
		TeamPlayerState->OnGamePlayerNameChanged.RemoveDynamic(
			this,
			&AKCPlayerCharacter::HandleGamePlayerNameChanged);
	}

	BoundTeamPlayerState.Reset();
}

void AKCPlayerCharacter::HandleTeamIdChanged(const int32 NewTeamId)
{
	ApplyTeamAppearance(NewTeamId);
	RefreshPlayerOverHead();
}

void AKCPlayerCharacter::HandleGamePlayerNameChanged(const FString& NewPlayerName)
{
	RefreshPlayerOverHead();
}

void AKCPlayerCharacter::RefreshPlayerOverHead()
{
	if (!PlayerOverHeadComponent)
	{
		return;
	}

	const AKCPlayerState* KCPlayerState = GetPlayerState<AKCPlayerState>();
	if (!KCPlayerState)
	{
		PlayerOverHeadComponent->ClearPlayerDisplayInfo();
		return;
	}

	PlayerOverHeadComponent->SetPlayerDisplayInfo(FKCPlayerDisplayInfoStruct(
		FText::FromString(KCPlayerState->GetGamePlayerName()),
		KCPlayerState->GetTeamId(),
		KCPlayerState->GetUniquePlayerIdString(),
		true));
}

void AKCPlayerCharacter::ApplyTeamAppearance(const int32 TeamId)
{
	const FKCAvatarTeamAppearanceStruct* Appearance =
		TeamAppearances.FindByPredicate(
			[TeamId](const FKCAvatarTeamAppearanceStruct& Candidate)
			{
				return Candidate.TeamId == TeamId;
			});
	if (!Appearance)
	{
		return;
	}

	const auto ApplyMaterial = [](UStaticMeshComponent* Component,
		UMaterialInterface* Material)
	{
		if (Component && Material)
		{
			Component->SetMaterial(0, Material);
		}
	};

	ApplyMaterial(AvatarBody, Appearance->BodyMaterial);
	ApplyMaterial(AvatarHandLeft, Appearance->HandMaterial);
	ApplyMaterial(AvatarHandRight, Appearance->HandMaterial);
	ApplyMaterial(AvatarFootLeft, Appearance->FootMaterial);
	ApplyMaterial(AvatarFootRight, Appearance->FootMaterial);
}

void AKCPlayerCharacter::InitializeAbilityActorInfo()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	BindAttributeDelegates();
	GrantDefaultAbilities();
	EnsureStaminaRegenEffect();
}

void AKCPlayerCharacter::GrantDefaultAbilities()
{
	if (!HasAuthority() || !AbilitySystemComponent || !DashAbilityClass ||
		AbilitySystemComponent->FindAbilitySpecFromClass(DashAbilityClass))
	{
		return;
	}

	AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(
		DashAbilityClass,
		1,
		INDEX_NONE,
		this));
}

void AKCPlayerCharacter::EnsureStaminaRegenEffect()
{
	if (!HasAuthority() || !AbilitySystemComponent ||
		!StaminaRegenEffectClass)
	{
		return;
	}

	if (StaminaRegenEffectHandle.IsValid() &&
		AbilitySystemComponent->GetActiveGameplayEffect(
			StaminaRegenEffectHandle))
	{
		return;
	}

	const UGameplayEffect* RegenEffect =
		StaminaRegenEffectClass->GetDefaultObject<UGameplayEffect>();
	if (!RegenEffect)
	{
		return;
	}

	StaminaRegenEffectHandle =
		AbilitySystemComponent->ApplyGameplayEffectToSelf(
			RegenEffect,
			1.0f,
			AbilitySystemComponent->MakeEffectContext());
}

void AKCPlayerCharacter::BindAttributeDelegates()
{
	if (!AbilitySystemComponent || !CharacterAttributes)
	{
		return;
	}

	FOnGameplayAttributeValueChange& MoveSpeedDelegate =
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			UKCCharacterAttributeSet::GetMoveSpeedAttribute());
	if (MoveSpeedChangedDelegateHandle.IsValid())
	{
		MoveSpeedDelegate.Remove(MoveSpeedChangedDelegateHandle);
		MoveSpeedChangedDelegateHandle.Reset();
	}
	MoveSpeedChangedDelegateHandle = MoveSpeedDelegate.AddUObject(
		this,
		&AKCPlayerCharacter::HandleMoveSpeedChanged);
	ApplyMoveSpeed(CharacterAttributes->GetMoveSpeed());

	FOnGameplayAttributeValueChange& HealthDelegate =
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			UKCCharacterAttributeSet::GetHealthAttribute());
	if (HealthChangedDelegateHandle.IsValid())
	{
		HealthDelegate.Remove(HealthChangedDelegateHandle);
		HealthChangedDelegateHandle.Reset();
	}
	HealthChangedDelegateHandle = HealthDelegate.AddUObject(
		this,
		&AKCPlayerCharacter::HandleHealthChanged);
}

void AKCPlayerCharacter::HandleMoveSpeedChanged(
	const FOnAttributeChangeData& ChangeData)
{
	ApplyMoveSpeed(ChangeData.NewValue);
}

void AKCPlayerCharacter::HandleHealthChanged(
	const FOnAttributeChangeData& ChangeData)
{
	if (HasAuthority() && ChangeData.NewValue < ChangeData.OldValue)
	{
		InterruptEmote();
	}
}

void AKCPlayerCharacter::ApplyMoveSpeed(const float MoveSpeed)
{
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = FMath::IsFinite(MoveSpeed)
			? FMath::Max(MoveSpeed, 0.0f)
			: 0.0f;
	}
}

void AKCPlayerCharacter::MoveInWorldDirection(const FVector& WorldDirection, const float ScaleValue)
{
	if (!FMath::IsNearlyZero(ScaleValue))
	{
		InterruptEmote();
		AddMovementInput(WorldDirection, ScaleValue);
	}
}

bool AKCPlayerCharacter::RequestDash()
{
	if (!IsLocallyControlled() || !AbilitySystemComponent || !DashAbilityClass)
	{
		return false;
	}

	FGameplayAbilitySpec* DashSpec =
		AbilitySystemComponent->FindAbilitySpecFromClass(DashAbilityClass);
	if (!DashSpec)
	{
		return false;
	}

	FVector DashDirection = GetLastMovementInputVector().GetSafeNormal2D();
	if (DashDirection.IsNearlyZero())
	{
		DashDirection = GetActorForwardVector().GetSafeNormal2D();
	}
	if (DashDirection.IsNearlyZero())
	{
		return false;
	}

	FGameplayEventData EventData;
	EventData.Instigator = this;
	EventData.Target = this;
	EventData.EventMagnitude = FMath::UnwindDegrees(
		DashDirection.Rotation().Yaw);
	const bool bDashActivated =
		AbilitySystemComponent->TryActivateGrantedAbilityWithEvent(
		DashSpec->Handle,
		TAG_KC_GameplayEvent_Player_Dash,
		EventData);
	if (bDashActivated)
	{
		InterruptEmote();
	}
	return bDashActivated;
}

bool AKCPlayerCharacter::RequestPlayEmote(const int32 EmoteIndex)
{
	return EmoteComponent && EmoteComponent->RequestPlayEmote(EmoteIndex);
}

bool AKCPlayerCharacter::RequestPlayNextEmote()
{
	return EmoteComponent && EmoteComponent->RequestPlayNextEmote();
}

void AKCPlayerCharacter::RequestStopEmote(const float BlendOutTime)
{
	if (EmoteComponent)
	{
		EmoteComponent->RequestStopEmote(BlendOutTime);
	}
}

void AKCPlayerCharacter::LaunchCharacter(
	const FVector LaunchVelocity,
	const bool bXYOverride,
	const bool bZOverride)
{
	InterruptEmote();
	Super::LaunchCharacter(LaunchVelocity, bXYOverride, bZOverride);
}

void AKCPlayerCharacter::InterruptEmote()
{
	if (EmoteComponent)
	{
		EmoteComponent->RequestInterruptEmote();
	}
}

void AKCPlayerCharacter::UpdateFacingDirection(const FVector& WorldDirection, const float DeltaSeconds)
{
	const FVector FlatDirection = FVector(WorldDirection.X, WorldDirection.Y, 0.0f).GetSafeNormal();
	if (FlatDirection.IsNearlyZero())
	{
		return;
	}

	const float FacingYaw = FlatDirection.Rotation().Yaw;
	ApplyFacingYaw(FacingYaw);

	if (HasAuthority())
	{
		return;
	}

	FacingReplicationElapsed += DeltaSeconds;
	const float FacingDelta = FMath::Abs(FMath::FindDeltaAngleDegrees(LastSentFacingYaw, FacingYaw));
	if (FacingReplicationElapsed >= FacingReplicationInterval
		&& FacingDelta >= MinimumFacingReplicationAngle)
	{
		ServerSetFacingYaw(FacingYaw);
		LastSentFacingYaw = FacingYaw;
		FacingReplicationElapsed = 0.0f;
	}
}

void AKCPlayerCharacter::ApplyFacingYaw(const float FacingYaw)
{
	if (!IsValidFacingYaw(FacingYaw))
	{
		return;
	}

	SetActorRotation(FRotator(0.0f, FMath::UnwindDegrees(FacingYaw), 0.0f));
}

void AKCPlayerCharacter::ServerSetFacingYaw_Implementation(const float FacingYaw)
{
	if (!IsValidFacingYaw(FacingYaw))
	{
		return;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const double CurrentTimeSeconds = World->GetTimeSeconds();
	const double ElapsedSinceLastUpdate =
		CurrentTimeSeconds - LastServerFacingUpdateTimeSeconds;
	if (LastServerFacingUpdateTimeSeconds < 0.0 ||
		CurrentTimeSeconds < LastServerFacingUpdateTimeSeconds ||
		ElapsedSinceLastUpdate >= MinimumServerFacingUpdateInterval)
	{
		GetWorldTimerManager().ClearTimer(ServerFacingUpdateTimer);
		bHasPendingServerFacingYaw = false;
		ApplyAcceptedServerFacingYaw(FacingYaw, CurrentTimeSeconds);
		return;
	}

	// 지연으로 같은 서버 틱에 RPC가 몰리면 오래된 중간값은 버리고
	// 제한 구간에서 받은 가장 최신 방향 하나만 다음 슬롯에 적용한다.
	PendingServerFacingYaw = FacingYaw;
	bHasPendingServerFacingYaw = true;
	if (!GetWorldTimerManager().IsTimerActive(ServerFacingUpdateTimer))
	{
		GetWorldTimerManager().SetTimer(
			ServerFacingUpdateTimer,
			this,
			&AKCPlayerCharacter::FlushPendingServerFacingYaw,
			MinimumServerFacingUpdateInterval - ElapsedSinceLastUpdate,
			false);
	}
}

void AKCPlayerCharacter::ApplyAcceptedServerFacingYaw(
	const float FacingYaw,
	const double CurrentTimeSeconds)
{
	LastServerFacingUpdateTimeSeconds = CurrentTimeSeconds;
	ApplyFacingYaw(FacingYaw);
}

void AKCPlayerCharacter::FlushPendingServerFacingYaw()
{
	if (!HasAuthority() || !bHasPendingServerFacingYaw)
	{
		return;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		bHasPendingServerFacingYaw = false;
		return;
	}

	const float FacingYaw = PendingServerFacingYaw;
	bHasPendingServerFacingYaw = false;
	ApplyAcceptedServerFacingYaw(FacingYaw, World->GetTimeSeconds());
}
