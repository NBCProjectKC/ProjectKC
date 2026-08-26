#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffectTypes.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"
#include "KCPlayerCharacter.generated.h"

class UAbilitySystemComponent;
class UCameraComponent;
class UGameplayAbility;
class UGameplayEffect;
class UKCAbilitySystemComponent;
class UKCCharacterAttributeSet;
class UKCEmoteComponent;
class UKCHeldItemComponent;
class UKCItemDefinition;
class UKCKnockbackComponent;
class UKCPlayerInteractionComponent;
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

	void MoveInWorldDirection(const FVector& WorldDirection, float ScaleValue);
	void UpdateFacingDirection(const FVector& WorldDirection, float DeltaSeconds);
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

	UFUNCTION(BlueprintPure, Category = "Interaction")
	UKCPlayerInteractionComponent* GetInteractionComponent() const;

	/** 선택한 Item Definition을 실제 HandItem/Grip 계산으로 에디터에 표시한다. */
	UFUNCTION(CallInEditor, Category = "KC|Item|Preview")
	void RefreshHeldItemPreview();

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_Controller() override;
	virtual void PawnClientRestart() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(
		FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	void ConfigureDriverMesh();
	void InitializeAbilityActorInfo();
	void GrantDefaultAbilities();
	void EnsureStaminaRegenEffect();
	void BindAttributeDelegates();
	void HandleMoveSpeedChanged(const FOnAttributeChangeData& ChangeData);
	void ApplyMoveSpeed(float MoveSpeed);
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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UKCPlayerInteractionComponent> InteractionComponent;

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

#if WITH_EDITORONLY_DATA
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
	FActiveGameplayEffectHandle StaminaRegenEffectHandle;
};
