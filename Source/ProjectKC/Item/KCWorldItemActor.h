#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectKC/Interaction/Interface/KCInteractableInterface.h"
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

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_RuntimeState();

	UFUNCTION()
	void OnRep_ItemDefinition();

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
	bool ActivateUse();
	bool ActivateUseWithTarget(AActor* TargetActor);
	bool RefreshDefinition(FString* OutError = nullptr);
	bool AlignGripToAttachmentSocket();
	void RefreshReplicatedAttachment();
	void ApplyStatePresentation();
	void BroadcastStateChanged();

	UPROPERTY(ReplicatedUsing = OnRep_RuntimeState)
	FKCWorldItemRuntimeState RuntimeState;

	bool bDefinitionValid = false;
};
