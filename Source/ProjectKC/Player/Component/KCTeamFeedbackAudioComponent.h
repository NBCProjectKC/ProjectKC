#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "KCTeamFeedbackAudioComponent.generated.h"

struct FKCDishRuinedStruct;

UCLASS(ClassGroup = (KC), meta = (BlueprintSpawnableComponent))
class PROJECTKC_API UKCTeamFeedbackAudioComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UKCTeamFeedbackAudioComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void HandleDishRuined(FGameplayTag Channel, const FKCDishRuinedStruct& Message);
	void PlayRecipeFailedSound() const;
	int32 GetOwnerTeamId() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Audio|SFX", meta = (AllowPrivateAccess = "true"))
	bool bPlayRecipeFailedSound = true;

	FGameplayMessageListenerHandle DishRuinedListenerHandle;
};