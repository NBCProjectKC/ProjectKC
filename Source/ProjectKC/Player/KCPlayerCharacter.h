#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffectTypes.h"
#include "GameFramework/Character.h"
#include "Player/Struct/KCAvatarTeamAppearanceStruct.h"
#include "TimerManager.h"
#include "KCPlayerCharacter.generated.h"

class UAbilitySystemComponent;
class UCameraComponent;
class UGameplayAbility;
class UGameplayEffect;
class AKCPlayerState;
class UKCAbilitySystemComponent;
class UKCCharacterAttributeSet;
class UKCEmoteComponent;
class UKCHeldItemComponent;
class UKCItemDefinition;
class UKCKnockbackComponent;
class UKCPlayerCustomizationComponent;
class UKCPlayerOverHeadComponent;
class UKCPlayerInteractionPromptComponent;
class UKCPlayerInteractionComponent;
class UKCProjectileTrajectoryPreviewComponent;
class USceneComponent;
class USpringArmComponent;
class UStaticMeshComponent;
#if WITH_EDITOR
struct FPropertyChangedEvent;
#endif
struct FOnAttributeChangeData;

UCLASS()
class PROJECTKC_API AKCPlayerCharacter
	: public ACharacter
	, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AKCPlayerCharacter();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void LaunchCharacter(
		FVector LaunchVelocity,
		bool bXYOverride,
		bool bZOverride) override;

	void MoveInWorldDirection(const FVector& WorldDirection, float ScaleValue);
	void UpdateFacingDirection(const FVector& WorldDirection, float DeltaSeconds);
	void UpdateCameraLookAhead(const FVector& CursorWorldOffset, float DeltaSeconds);
	void TriggerDashCameraPunch();
	bool RequestDash();
	bool RequestPlayEmote(int32 EmoteIndex = 0);
	bool RequestPlayNextEmote();
	void RequestStopEmote(float BlendOutTime = 0.2f);
	bool BeginUseHeldItem();
	void EndUseHeldItem();
	void RequestInteract();
	void RequestDropHeldItem();

	UFUNCTION(BlueprintPure, Category = "KC|Ability")
	UKCAbilitySystemComponent* GetKCAbilitySystemComponent() const;

	UFUNCTION(BlueprintPure, Category = "KC|Attributes")
	UKCCharacterAttributeSet* GetCharacterAttributes() const;

	UFUNCTION(BlueprintPure, Category = "KC|Emote")
	UKCEmoteComponent* GetEmoteComponent() const;

	UFUNCTION(BlueprintPure, Category = "KC|Item")
	UKCHeldItemComponent* GetHeldItemComponent() const;

	UFUNCTION(BlueprintPure, Category = "KC|Knockback")
	UKCKnockbackComponent* GetKnockbackComponent() const;

	UFUNCTION(BlueprintPure, Category = "KC|Projectile|Preview")
	UKCProjectileTrajectoryPreviewComponent*
	GetProjectileTrajectoryPreviewComponent() const;
	
	UFUNCTION(BlueprintPure, Category = "KC|Customization")
	UKCPlayerCustomizationComponent* GetPlayerCustomizationComponent() const;

	UFUNCTION(BlueprintPure, Category = "Interaction")
	UKCPlayerInteractionComponent* GetInteractionComponent() const;

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	UKCPlayerInteractionPromptComponent* GetInteractionPromptComponent() const;

	/** 선택한 Item Definition을 실제 HandItem/Grip 계산으로 에디터에 표시한다. */
	UFUNCTION(CallInEditor, Category = "KC|Item|Preview")
	void RefreshHeldItemPreview();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_Controller() override;
	virtual void OnRep_Owner() override;
	virtual void OnRep_PlayerState() override;
	virtual void PawnClientRestart() override;

	void BindTeamAppearanceToPlayerState(AKCPlayerState* InPlayerState);
	void ApplyTeamAppearance(int32 TeamId);

#if WITH_EDITOR
	virtual void PostEditChangeProperty(
		FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	void ConfigureDriverMesh();
	void RefreshTeamAppearanceBinding();
	void UnbindTeamAppearanceFromPlayerState();
	void RefreshPlayerOverHead();

	UFUNCTION()
	void HandleTeamIdChanged(int32 NewTeamId);

	UFUNCTION()
	void HandleGamePlayerNameChanged(const FString& NewPlayerName);

	void InitializeAbilityActorInfo();
	void GrantDefaultAbilities();
	void EnsureStaminaRegenEffect();
	void BindAttributeDelegates();
	void HandleMoveSpeedChanged(const FOnAttributeChangeData& ChangeData);
	void HandleHealthChanged(const FOnAttributeChangeData& ChangeData);
	void ApplyMoveSpeed(float MoveSpeed);
	void InterruptEmote();
	void ApplyFacingYaw(float FacingYaw);
	void ApplyAcceptedServerFacingYaw(
		float FacingYaw,
		double CurrentTimeSeconds);
	void FlushPendingServerFacingYaw();

	UFUNCTION(Server, Unreliable)
	void ServerSetFacingYaw(float FacingYaw);

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoomComponent;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> TopDownCameraComponent;

	/** 커서의 월드 거리 중 카메라 선행 오프셋으로 반영할 비율이다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Camera|Look Ahead",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "1.0"))
	float CameraLookAheadStrength = 0.25f;

	/** 카메라 중심이 캐릭터로부터 이동할 수 있는 최대 월드 거리다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Camera|Look Ahead",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float CameraLookAheadMaxDistance = 220.0f;

	/** 커서가 캐릭터 중심 근처에 있을 때 미세 진동을 막는 월드 거리다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Camera|Look Ahead",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float CameraLookAheadDeadZone = 80.0f;

	/** 목표 오프셋을 따라가고 중앙으로 복귀하는 보간 속도다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Camera|Look Ahead",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float CameraLookAheadInterpSpeed = 7.5f;

	/** 대시 시작 순간 기본 시야각에 더할 값이다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Camera|Dash",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "30.0"))
	float DashCameraFOVKick = 8.0f;

	/** 대시 카메라 시야각이 최대치까지 부드럽게 커지는 시간이다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Camera|Dash",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float DashCameraFOVAttackDuration = 0.15f;

	/** 최대 시야각을 유지해 대시 속도감을 읽을 수 있게 하는 시간이다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Camera|Dash",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float DashCameraFOVHoldDuration = 0.18f;

	/** 최대 시야각에서 기본값까지 부드럽게 돌아오는 시간이다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Camera|Dash",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float DashCameraFOVReturnDuration = 0.42f;

	FVector BaseCameraTargetOffset = FVector::ZeroVector;
	FVector CurrentCameraLookAheadOffset = FVector::ZeroVector;
	float BaseCameraFieldOfView = 90.0f;
	float CurrentDashCameraFOVOffset = 0.0f;
	float DashCameraFOVElapsed = -1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|Ability",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UKCAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|Attributes",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UKCCharacterAttributeSet> CharacterAttributes;

	UPROPERTY(EditDefaultsOnly, Category = "KC|Dash")
	TSubclassOf<UGameplayAbility> DashAbilityClass;

	UPROPERTY(EditDefaultsOnly, Category = "KC|Attributes")
	TSubclassOf<UGameplayEffect> StaminaRegenEffectClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|Emote",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UKCEmoteComponent> EmoteComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|Item",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UKCHeldItemComponent> HeldItemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|Knockback",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UKCKnockbackComponent> KnockbackComponent;

	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "KC|Projectile|Preview",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UKCProjectileTrajectoryPreviewComponent>
		ProjectileTrajectoryPreviewComponent;
		
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|Customization",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UKCPlayerCustomizationComponent> PlayerCustomizationComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UKCPlayerInteractionComponent> InteractionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|UI",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UKCPlayerOverHeadComponent> PlayerOverHeadComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|UI",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UKCPlayerInteractionPromptComponent> InteractionPromptComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|Avatar",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> AvatarBody;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|Avatar",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> AvatarHandLeft;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|Avatar",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> AvatarHandRight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|Avatar",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> AvatarFootLeft;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|Avatar",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> AvatarFootRight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|Avatar",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> FaceAnchor;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Avatar|Team",
		meta = (AllowPrivateAccess = "true", TitleProperty = "TeamId"))
	TArray<FKCAvatarTeamAppearanceStruct> TeamAppearances;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = "KC|Avatar|Team|Preview",
		meta = (ClampMin = "0"))
	int32 PreviewTeamId = 0;

	UPROPERTY(
		EditAnywhere,
		Category = "KC|Item|Preview",
		meta = (DisplayThumbnail = true))
	TObjectPtr<UKCItemDefinition> PreviewItemDefinition;

	UPROPERTY(VisibleAnywhere, Category = "KC|Item|Preview")
	TObjectPtr<UStaticMeshComponent> HeldItemPreviewMesh;
#endif

	float FacingReplicationElapsed = 0.0f;
	float LastSentFacingYaw = 0.0f;
	double LastServerFacingUpdateTimeSeconds = -1.0;
	float PendingServerFacingYaw = 0.0f;
	bool bHasPendingServerFacingYaw = false;
	FTimerHandle ServerFacingUpdateTimer;
	FDelegateHandle MoveSpeedChangedDelegateHandle;
	FDelegateHandle HealthChangedDelegateHandle;
	FActiveGameplayEffectHandle StaminaRegenEffectHandle;
	TWeakObjectPtr<AKCPlayerState> BoundTeamPlayerState;
};
