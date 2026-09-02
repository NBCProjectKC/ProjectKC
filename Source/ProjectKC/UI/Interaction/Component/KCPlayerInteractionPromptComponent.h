#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KCPlayerInteractionPromptComponent.generated.h"

class AActor;
class UWidgetComponent;
class AKCWorldItemActor;
class UKCHeldItemComponent;
class UKCInteractionPromptRegistry;
class UKCInteractionPromptViewModel;
class UKCInteractionPromptWidget;
class UKCPlayerInteractionComponent;

UCLASS(ClassGroup = (KC), meta = (BlueprintSpawnableComponent))
class PROJECTKC_API UKCPlayerInteractionPromptComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UKCPlayerInteractionPromptComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	UKCInteractionPromptViewModel* GetViewModel() const
	{
		return InteractionPromptViewModel;
	}

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	UWidgetComponent* GetWidgetComponent() const
	{
		return InteractionWidgetComponent;
	}

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|UI")
	TSubclassOf<UKCInteractionPromptWidget> InteractionPromptWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|UI")
	TObjectPtr<UKCInteractionPromptRegistry> InteractionPromptRegistry;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|UI")
	FVector WidgetWorldLocationOffset = FVector(0.0f, 0.0f, 80.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|UI")
	FVector2D WidgetDrawSize = FVector2D(180.0f, 48.0f);

private:
	bool ShouldCreateLocalPrompt() const;
	bool InitializeLocalPrompt();
	void BindInteractionComponent();
	void UnbindInteractionComponent();
	void BindHeldItemComponent();
	void UnbindHeldItemComponent();
	void BindObservedHeldItem(AKCWorldItemActor* NewHeldItem);
	void UnbindObservedHeldItem();
	void EnsureViewModel();
	void EnsureWidgetComponent();
	void EnsurePromptAssets();
	void HandleBestInteractableChanged(AActor* NewTargetActor);

	UFUNCTION()
	void HandleHeldItemChanged(AKCWorldItemActor* NewHeldItem);

	UFUNCTION()
	void HandleObservedHeldItemBroken();

	UFUNCTION()
	void HandleObservedHeldItemDestroyed(AActor* DestroyedActor);

	void SetTargetActor(AActor* NewTargetActor);
	void ClearTargetActor();
	void RefreshPrompt();
	void ApplyViewModelToWidget();
	void UpdateWidgetLocation();

	UPROPERTY(Transient)
	TObjectPtr<UWidgetComponent> InteractionWidgetComponent;

	UPROPERTY(Transient)
	TObjectPtr<UKCInteractionPromptViewModel> InteractionPromptViewModel;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> CurrentTargetActor;

	UPROPERTY(Transient)
	TWeakObjectPtr<UKCPlayerInteractionComponent> BoundInteractionComponent;

	UPROPERTY(Transient)
	TWeakObjectPtr<UKCHeldItemComponent> BoundHeldItemComponent;

	UPROPERTY(Transient)
	TWeakObjectPtr<AKCWorldItemActor> ObservedHeldItem;

	bool bLocalPromptInitialized = false;
};
