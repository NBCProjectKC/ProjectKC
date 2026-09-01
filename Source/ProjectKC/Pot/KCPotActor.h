#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "ProjectKC/Interaction/Interface/KCInteractableInterface.h"
#include "ProjectKC/Pot/Enum/KCPotStateType.h"
#include "KCPotActor.generated.h"

class UBoxComponent;
class UDataTable;
class UPrimitiveComponent;
class UKCAbilitySystemComponent;
class UKCCookingProgressAttributeSet;
class UKCPotProgressBroadcasterComponent;
struct FOnAttributeChangeData;
struct FKCDishRuinedStruct;
struct FKCRecipeCompletedStruct;

/** 팀 전용 재료 투입과 조리 진행도를 담당하는 월드 냄비다. */
UCLASS(Blueprintable)
class PROJECTKC_API AKCPotActor
	: public AActor
	, public IAbilitySystemInterface
	, public IKCInteractableInterface
{
	GENERATED_BODY()

public:
	AKCPotActor();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual void Interact_Implementation(AActor* Interactor) override;

	UFUNCTION(BlueprintPure, Category = "KC|Pot")
	EKCPotStateType GetPotState() const { return PotState; }

	int32 GetAssignedTeamId() const { return AssignedTeamId; }
	FName GetActiveRecipeRowName() const { return ActiveRecipeRowName; }
	const UKCCookingProgressAttributeSet* GetCookingProgressAttributes() const { return CookingProgressAttributes; }
	float GetActiveProgressSpeedPerSecond() const { return ActiveProgressSpeedPerSecond; }

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "KC|Pot")
	bool ResetPot();

	/** 뚜껑을 닫는 연출을 Blueprint에서 구현한다. */
	UFUNCTION(BlueprintImplementableEvent, Category = "KC|Pot|Presentation")
	void OnCookingStarted();

	/** 완성 음식 연출을 Blueprint에서 구현한다. */
	UFUNCTION(BlueprintImplementableEvent, Category = "KC|Pot|Presentation")
	void OnCookingCompleted();

	/** 잘못된 재료 투입 연출을 Blueprint에서 구현한다. */
	UFUNCTION(BlueprintImplementableEvent, Category = "KC|Pot|Presentation")
	void OnCookingRuined();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|Pot")
	TObjectPtr<UBoxComponent> InteractionVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|Pot")
	TObjectPtr<UKCAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|Pot")
	TObjectPtr<UKCCookingProgressAttributeSet> CookingProgressAttributes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|Pot")
	TObjectPtr<UKCPotProgressBroadcasterComponent> PotProgressBroadcaster;

	/** 0은 1팀, 1은 2팀이다. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "KC|Pot")
	int32 AssignedTeamId = 0;

	/** GameMode와 동일한 레시피 DataTable을 지정한다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Pot")
	TObjectPtr<UDataTable> RecipeDataTable;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "KC|Pot")
	EKCPotStateType PotState = EKCPotStateType::Idle;

private:
	UFUNCTION()
	void OnInteractionVolumeBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	void HandleRecipeCompleted(
		FGameplayTag Channel,
		const FKCRecipeCompletedStruct& Message);
	void HandleDishRuined(
		FGameplayTag Channel,
		const FKCDishRuinedStruct& Message);
	bool IsRegisteredIngredient(const FGameplayTag& IngredientId) const;
	bool TrySubmitHeldIngredient(AActor& Interactor);
	bool ConsumeHeldItem(AActor& Interactor);
	void StartCooking(FName RecipeRowName, float ProgressSpeedPerSecond);
	void AdvanceCookingProgress();
	void CompleteCooking();
	void ApplyCookingProgressIncrease(float Amount);
	void HandleCookingProgressChanged(const FOnAttributeChangeData& ChangeData);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastCookingStarted();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastCookingCompleted();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastCookingRuined();

	FGameplayMessageListenerHandle RecipeCompletedListenerHandle;
	FGameplayMessageListenerHandle DishRuinedListenerHandle;
	FTimerHandle CookingTimerHandle;
	FDelegateHandle CookingProgressChangedDelegateHandle;
	FName ActiveRecipeRowName;
	float ActiveProgressSpeedPerSecond = 0.0f;
	bool bRestoringCookingProgress = false;
};
