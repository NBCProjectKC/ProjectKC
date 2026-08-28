#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/UI/Common/Widget/KCUserWidget.h"
#include "ProjectKC/UI/HUD/ViewModel/KCHUDViewModel.h"
#include "KCHUDPotProgressWidget.generated.h"

class UImage;
class UTextBlock;
class UWidgetAnimation;

UCLASS(Abstract, Blueprintable)
class PROJECTKC_API UKCHUDPotProgressWidget : public UKCUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativePreConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetPotProgress(const FKCPotProgressViewData& PotProgress);

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void HidePotProgress();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KC|UI|Preview")
	FKCPotProgressViewData PreviewPotProgress;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|UI|Material")
	FName ProgressParameterName = TEXT("Progress");

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UImage> Team1PotProgress;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UTextBlock> Team1PotProgressText;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> PopUp;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> Pop;

	UFUNCTION(BlueprintImplementableEvent, Category = "KC|UI", meta = (DisplayName = "On Pot Progress Completed"))
	void BP_OnPotProgressCompleted();

private:
	void ApplyProgressMaterial(float ProgressPercent);
	void ApplyRemainingSeconds(int32 RemainingSeconds);
	void PlayCompletedAnimationThenHide();
	UWidgetAnimation* GetCompletedAnimation() const;

	FTimerHandle HideAfterCompletedAnimationTimerHandle;
	bool bCompletedAnimationPlayed = false;
};
