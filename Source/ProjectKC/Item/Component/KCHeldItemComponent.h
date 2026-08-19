#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "KCHeldItemComponent.generated.h"

class AKCWorldItemActor;
class USceneComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FKCHeldItemChangedSignature,
	AKCWorldItemActor*,
	NewHeldItem);

/** 한 Actor가 한 번에 하나의 월드 아이템만 들도록 보장하는 Holder 측 컴포넌트다. */
UCLASS(BlueprintType, ClassGroup = (KC), meta = (BlueprintSpawnableComponent))
class PROJECTKC_API UKCHeldItemComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UKCHeldItemComponent();

	/** 런타임에 기본 AttachmentComponent 설정을 덮어써야 할 때 사용한다. */
	bool ConfigureAttachment(
		USceneComponent* NewAttachmentComponent,
		FName NewHandSocketName);

	/** 입력 계층에서 호출한다. 서버가 현재 위치에서 가장 가까운 아이템을 찾는다. */
	UFUNCTION(BlueprintCallable, Category = "KC|Item")
	void TryInteract();

	/** 입력 계층에서 호출한다. 서버가 컴포넌트 설정으로 Drop Transform을 계산한다. */
	UFUNCTION(BlueprintCallable, Category = "KC|Item")
	void TryDropHeldItem();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "KC|Item")
	bool TryPickUp(AKCWorldItemActor* Item);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "KC|Item")
	bool DropHeldItem(
		const FTransform& DropTransform,
		FVector DropImpulse = FVector::ZeroVector);

	UFUNCTION(BlueprintCallable, Category = "KC|Item")
	bool UseHeldItem();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "KC|Item")
	bool UseHeldItemWithTarget(AActor* TargetActor);

	UFUNCTION(BlueprintPure, Category = "KC|Item")
	AKCWorldItemActor* GetHeldItem() const;

	UFUNCTION(BlueprintPure, Category = "KC|Item")
	bool HasHeldItem() const;

	/** 명시적 설정 또는 HandSocketName을 가진 Owner 컴포넌트를 반환한다. */
	UFUNCTION(BlueprintPure, Category = "KC|Item|Attachment")
	USceneComponent* GetAttachmentComponent() const;

	UFUNCTION(BlueprintPure, Category = "KC|Item|Attachment")
	FName GetHandSocketName() const;

	UPROPERTY(BlueprintAssignable, Category = "KC|Item")
	FKCHeldItemChangedSignature OnHeldItemChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(Server, Reliable)
	void ServerTryInteract();

	UFUNCTION(Server, Reliable)
	void ServerDropHeldItem();

	UFUNCTION()
	void OnRep_HeldItem();

	/**
	 * Holder의 SkeletalMesh 등 손 소켓을 제공하는 컴포넌트다.
	 * 비어 있으면 Owner에서 HandSocketName을 가진 Scene Component를 자동 탐색한다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KC|Item|Attachment")
	FComponentReference AttachmentComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KC|Item|Attachment")
	FName HandSocketName = TEXT("HandItem");

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "KC|Item|Interaction",
		meta = (ClampMin = "0.0"))
	float InteractionRadius = 180.0f;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "KC|Item|Drop",
		meta = (ClampMin = "0.0"))
	float DropForwardDistance = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KC|Item|Drop")
	float DropHeightOffset = 30.0f;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "KC|Item|Drop",
		meta = (ClampMin = "0.0"))
	float DropForwardImpulse = 0.0f;

private:
	friend class AKCWorldItemActor;

	USceneComponent* ResolveAttachmentComponent() const;
	void TryInteractAuthority();
	void DropHeldItemAuthority();
	AKCWorldItemActor* FindBestPickupCandidate() const;
	FTransform MakeHeldItemDropTransform() const;
	void BroadcastHeldItemChanged();

	UPROPERTY(ReplicatedUsing = OnRep_HeldItem)
	TObjectPtr<AKCWorldItemActor> HeldItem;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> RuntimeAttachmentComponent;
};
