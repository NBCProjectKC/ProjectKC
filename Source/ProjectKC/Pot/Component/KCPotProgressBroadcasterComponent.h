#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KCPotProgressBroadcasterComponent.generated.h"

class UProgressBar;
class UUserWidget;
class UWidgetComponent;
class AKCPotActor;
class UKCCookingProgressAttributeSet;

UCLASS(ClassGroup = (KC), meta = (BlueprintSpawnableComponent))
class PROJECTKC_API UKCPotProgressBroadcasterComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UKCPotProgressBroadcasterComponent();

	virtual void BeginPlay() override;

	void NotifyCookingStarted();
	void NotifyCookingProgress();
	void NotifyCookingCompleted();
	void NotifyCookingHidden();

private:
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPotProgressStarted(int32 TeamId, int32 RemainingSeconds);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPotProgressUpdated(int32 TeamId, float ProgressPercent, int32 RemainingSeconds);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPotProgressCompleted(int32 TeamId);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPotProgressHidden(int32 TeamId);

	void BroadcastPotProgress(
		int32 TeamId,
		float ProgressPercent,
		int32 RemainingSeconds,
		bool bVisible,
		bool bCompleted);

	void EnsureWorldWidget();
	void ApplyWorldWidgetProgress(float ProgressPercent, bool bVisible);
	const AKCPotActor* GetPotActor() const;
	int32 GetRemainingCookingSeconds(
		const UKCCookingProgressAttributeSet* CookingProgressAttributes,
		float ProgressSpeedPerSecond) const;
	float GetCookingProgressPercent(
		const UKCCookingProgressAttributeSet* CookingProgressAttributes) const;

	UPROPERTY(EditDefaultsOnly, Category = "KC|Pot|UI")
	TSubclassOf<UUserWidget> PotProgressWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "KC|Pot|UI")
	FName ProgressBarWidgetName = TEXT("ProgressBar_34");

	UPROPERTY(EditDefaultsOnly, Category = "KC|Pot|UI")
	FVector WidgetRelativeLocation = FVector(0.0f, 0.0f, 160.0f);

	UPROPERTY(EditDefaultsOnly, Category = "KC|Pot|UI")
	FVector2D WidgetDrawSize = FVector2D(180.0f, 24.0f);

	UPROPERTY(Transient)
	TObjectPtr<UWidgetComponent> PotProgressWidgetComponent;

	UPROPERTY(Transient)
	TWeakObjectPtr<UProgressBar> CachedProgressBar;
};
