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
class UPrimitiveComponent;
class UKCAbilitySystemComponent;
class UKCCookingProgressAttributeSet;
class UKCPotProgressBroadcasterComponent;
struct FOnAttributeChangeData;
struct FKCDishRuinedStruct;
struct FKCRecipeCompletedStruct;

/** 솥의 수명 동안 누적되는 플레이어별 재료 투입 기록이다. */
struct FKCPotIngredientSubmissionStats
{
	FString PlayerName;
	int32 SubmissionCount = 0;
};

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
	virtual FGameplayTag GetInteractionPromptTag_Implementation(
		AActor* Interactor) const override;
	virtual FVector GetInteractionPromptWorldLocation_Implementation(
		AActor* Interactor) const override;

	UFUNCTION(BlueprintPure, Category = "KC|Pot")
	EKCPotStateType GetPotState() const { return PotState; }

	int32 GetAssignedTeamId() const { return AssignedTeamId; }
	FName GetActiveRecipeRowName() const { return ActiveRecipeRowName; }
	const UKCCookingProgressAttributeSet* GetCookingProgressAttributes() const { return CookingProgressAttributes; }
	float GetActiveProgressSpeedPerSecond() const { return ActiveProgressSpeedPerSecond; }

	/** 서버에서 레벨 종료 전에 수집한다. 고유 ID별 복사본이며 ResetPot으로 지워지지 않는다.
	 * 온라인 ID가 없는 경우 Local:<PlayerId> 키를 사용하며 재접속 식별은 보장하지 않는다. */
	TMap<FString, FKCPotIngredientSubmissionStats> GetIngredientSubmissionStats() const;

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
	// 서버 전용 누적 통계. 플레이어가 나가더라도 ID와 이름을 유지한다.
	TMap<FString, FKCPotIngredientSubmissionStats> IngredientSubmissionStats;
	FGameplayMessageListenerHandle DishRuinedListenerHandle;
	FTimerHandle CookingTimerHandle;
	FDelegateHandle CookingProgressChangedDelegateHandle;
	FName ActiveRecipeRowName;
	float ActiveProgressSpeedPerSecond = 0.0f;
	bool bRestoringCookingProgress = false;
};
