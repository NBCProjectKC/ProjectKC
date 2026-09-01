#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayAbilitySpecHandle.h"
#include "ProjectKC/Interaction/Interface/KCInteractableInterface.h"
#include "ProjectKC/Item/Struct/KCItemDurabilityStruct.h"
#include "KCWorldItemActor.generated.h"

class AActor;
class USceneComponent;
class UStaticMeshComponent;
class UKCAbilitySourceComponent;
class UKCHeldItemComponent;
class UKCItemDefinition;

UENUM(BlueprintType)
enum class EKCWorldItemState : uint8
{
	World,
	Held
};

/** 상태와 보유자는 하나의 복제 단위로 다뤄 서로 어긋난 프레임을 만들지 않는다. */
USTRUCT(BlueprintType)
struct PROJECTKC_API FKCWorldItemRuntimeState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "KC|Item")
	EKCWorldItemState State = EKCWorldItemState::World;

	UPROPERTY(BlueprintReadOnly, Category = "KC|Item")
	TObjectPtr<AActor> Holder;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FKCWorldItemStateChangedSignature,
	EKCWorldItemState,
	NewState,
	AActor*,
	NewHolder);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FKCItemDurabilityChangedSignature,
	float,
	PreviousDurability,
	float,
	CurrentDurability);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FKCItemBrokenSignature);

/**
 * 인벤토리 없이 월드와 손 사이를 오가는 아이템 기반 Actor다.
 * 플레이어 입력을 소유하지 않으며 Pickup/Drop/Use 계약만 제공한다.
 */
UCLASS(Blueprintable)
class PROJECTKC_API AKCWorldItemActor
	: public AActor
	, public IKCInteractableInterface
{
	GENERATED_BODY()

public:
	AKCWorldItemActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Interact_Implementation(AActor* Interactor) override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(
		FDataValidationContext& Context) const override;
#endif

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "KC|Item")
	bool InitializeItem(UKCItemDefinition* NewDefinition);

	UFUNCTION(BlueprintPure, Category = "KC|Item")
	bool CanBePickedUp() const;

	UFUNCTION(BlueprintPure, Category = "KC|Item")
	bool IsUsable() const;

	UFUNCTION(BlueprintPure, Category = "KC|Item|Durability")
	bool UsesDurability() const;

	UFUNCTION(BlueprintPure, Category = "KC|Item|Durability")
	bool IsBroken() const;

	UFUNCTION(BlueprintPure, Category = "KC|Item|Durability")
	float GetCurrentDurability() const;

	UFUNCTION(BlueprintPure, Category = "KC|Item|Durability")
	float GetMaximumDurability() const;

	UFUNCTION(BlueprintPure, Category = "KC|Item|Durability")
	float GetDurabilityNormalized() const;

	/** 성공 실행 뒤 소비가 확정되어 추가 사용을 막고 있는지 반환한다. */
	UFUNCTION(BlueprintPure, Category = "KC|Item|Use")
	bool IsUseConsumptionPending() const;

	/** Action 런타임이 성공한 Execute를 정산할 때 호출하는 서버 전용 진입점이다. */
	bool TryBeginUseConsumption();

	/** 활성 Action 정리가 끝난 뒤 예약된 소비 파괴를 다음 틱에 확정한다. */
	bool FinalizePendingUseConsumption();

	/** 서버의 실제 사용 수명주기가 정확한 소모 시점에 호출한다. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "KC|Item|Durability")
	bool TryConsumeDurability(
		EKCItemDurabilityConsumeMode ConsumeMode,
		float ConsumptionScale = 1.0f);

	UFUNCTION(BlueprintPure, Category = "KC|Item")
	EKCWorldItemState GetItemState() const;

	UFUNCTION(BlueprintPure, Category = "KC|Item")
	AActor* GetHolder() const;

	UFUNCTION(BlueprintPure, Category = "KC|Item")
	UKCItemDefinition* GetItemDefinition() const;

	UFUNCTION(BlueprintPure, Category = "KC|Item")
	UStaticMeshComponent* GetItemMesh() const;

	UPROPERTY(BlueprintAssignable, Category = "KC|Item")
	FKCWorldItemStateChangedSignature OnItemStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "KC|Item|Durability")
	FKCItemDurabilityChangedSignature OnDurabilityChanged;

	UPROPERTY(BlueprintAssignable, Category = "KC|Item|Durability")
	FKCItemBrokenSignature OnItemBroken;

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_RuntimeState();

	UFUNCTION()
	void OnRep_ItemDefinition();

	UFUNCTION()
	void OnRep_CurrentDurability(float PreviousDurability);

	UFUNCTION()
	void OnRep_UseConsumptionPending();

	/** 서버가 파괴 직전에 호출하며 현재 관련 클라이언트에서 연출을 재생한다. */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayBreakEffects(FVector_NetQuantize BreakLocation, FRotator BreakRotation);

	UPROPERTY(
		EditInstanceOnly,
		BlueprintReadOnly,
		ReplicatedUsing = OnRep_ItemDefinition,
		Category = "KC|Item",
		meta = (ExposeOnSpawn = true))
	TObjectPtr<UKCItemDefinition> ItemDefinition;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|Item")
	TObjectPtr<UStaticMeshComponent> ItemMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|Item|Use")
	TObjectPtr<UKCAbilitySourceComponent> AbilitySourceComponent;

private:
	friend class UKCHeldItemComponent;

	bool EnterHeldState(
		AActor* NewHolder,
		USceneComponent* AttachParent,
		FName AttachSocket);
	bool ExitHeldState(
		const FTransform& DropTransform,
		const FVector& DropImpulse);
	bool PressUse(FGameplayAbilitySpecHandle& OutPressedHandle);
	bool ReleaseUse(FGameplayAbilitySpecHandle PressedHandle);
	bool ActivateUseWithTarget(AActor* TargetActor);
	bool RefreshDefinition(FString* OutError = nullptr);
	void AlignGripToAttachmentSocket();
	void RefreshReplicatedAttachment();
	void ApplyStatePresentation();
	void BroadcastStateChanged();
	void ResetDurability();
	void SetCurrentDurability(float NewDurability);
	void BroadcastDurabilityChanged(float PreviousDurability);
	void HandleBroken();
	void DestroyBrokenItem();
	void DestroyConsumedItem();
	void DestroyItemActor();
	bool ShouldDestroyWhenBroken() const;

	UPROPERTY(ReplicatedUsing = OnRep_RuntimeState)
	FKCWorldItemRuntimeState RuntimeState;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentDurability)
	float CurrentDurability = FKCItemDurabilityStruct::MaximumDurability;

	UPROPERTY(ReplicatedUsing = OnRep_UseConsumptionPending)
	bool bUseConsumptionPending = false;

	bool bDefinitionValid = false;
	bool bBreakDestructionScheduled = false;
	bool bUseConsumptionDestructionScheduled = false;
};
