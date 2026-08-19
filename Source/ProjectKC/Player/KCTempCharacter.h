#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "KCTempCharacter.generated.h"

class UAbilitySystemComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class UKCAbilitySystemComponent;
class UKCHeldItemComponent;
class UKCItemDefinition;
class UKCKnockbackComponent;
class USpringArmComponent;
class UStaticMeshComponent;
struct FInputActionValue;
#if WITH_EDITOR
struct FPropertyChangedEvent;
#endif

/** GAS와 단일 HeldItem 흐름을 검증하기 위한 임시 탑다운 캐릭터다. */
UCLASS(Blueprintable)
class PROJECTKC_API AKCTempCharacter
	: public ACharacter
	, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AKCTempCharacter();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintPure, Category = "KC|Ability")
	UKCAbilitySystemComponent* GetKCAbilitySystemComponent() const;

	UFUNCTION(BlueprintPure, Category = "KC|Item")
	UKCHeldItemComponent* GetHeldItemComponent() const;

	UFUNCTION(BlueprintCallable, Category = "KC|Item")
	void TryInteract();

	UFUNCTION(BlueprintCallable, Category = "KC|Item")
	void TryDropHeldItem();

	UFUNCTION(BlueprintCallable, Category = "KC|Item")
	bool TryUseHeldItem();

	/** 선택한 Item Definition을 실제 장착 계산으로 에디터에 다시 표시한다. */
	UFUNCTION(CallInEditor, Category = "KC|Item|Preview")
	void RefreshHeldItemPreview();

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_Controller() override;
	virtual void PawnClientRestart() override;
	virtual void SetupPlayerInputComponent(
		UInputComponent* PlayerInputComponent) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(
		FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	void HandleMoveInput(const FInputActionValue& InputValue);
	void HandleInteractInput(const FInputActionValue& InputValue);
	void HandleUseInput(const FInputActionValue& InputValue);
	void HandleDropInput(const FInputActionValue& InputValue);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|Ability")
	TObjectPtr<UKCAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|Item")
	TObjectPtr<UKCHeldItemComponent> HeldItemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|Knockback")
	TObjectPtr<UKCKnockbackComponent> KnockbackComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|Camera")
	TObjectPtr<UCameraComponent> TopDownCamera;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Input")
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Input")
	TObjectPtr<UInputAction> UseAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Input")
	TObjectPtr<UInputAction> DropAction;

#if WITH_EDITORONLY_DATA
	/**
	 * 제작 중 손 장착 결과를 확인할 Item Definition이다.
	 * 에디터 전용이며 빌드된 게임과 실제 HeldItem 상태에는 포함되지 않는다.
	 */
	UPROPERTY(
		EditAnywhere,
		Category = "KC|Item|Preview",
		meta = (DisplayThumbnail = true))
	TObjectPtr<UKCItemDefinition> PreviewItemDefinition;

	/** HandItem과 Grip의 실제 정렬 결과를 보여주는 에디터 전용 Mesh다. */
	UPROPERTY(VisibleAnywhere, Category = "KC|Item|Preview")
	TObjectPtr<UStaticMeshComponent> HeldItemPreviewMesh;
#endif

private:
	void InitializeAbilityActorInfo();
	void InitializeInputMapping();
};
