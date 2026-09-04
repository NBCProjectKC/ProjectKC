#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/UI/Common/Widget/KCUserWidget.h"
#include "ProjectKC/UI/HUD/ViewModel/KCHUDViewModel.h"
#include "KCHUDPotProgressWidget.generated.h"

class UImage;
class UTextBlock;
class UWidgetAnimation;
class UKCColorStyle;

UCLASS(Abstract, Blueprintable)
class PROJECTKC_API UKCHUDPotProgressWidget : public UKCUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativePreConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetTeamId(int32 NewTeamId);

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetPotProgress(const FKCPotProgressViewData& PotProgress);

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void HidePotProgress();

protected:
	virtual void NativeApplyColorStyle(const UKCColorStyle* InColorStyle) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KC|UI|Preview")
	FKCPotProgressViewData PreviewPotProgress;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KC|UI|Preview")
	int32 TeamId = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|UI|Material")
	FName ProgressParameterName = TEXT("Radial_wipe");

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
	void ShowPotProgress();
	void ApplyProgressMaterial(float ProgressPercent);
	void ApplyRemainingSeconds(int32 RemainingSeconds);
	void ApplyTeamColor();
	void PlayCompletedAnimationThenHide();
	void PlayHideAnimationThenHide();

	FTimerHandle HideAfterCompletedAnimationTimerHandle;
	bool bCompletedAnimationPlayed = false;
	bool bShowing = false;
	bool bHideAnimationPlaying = false;
};