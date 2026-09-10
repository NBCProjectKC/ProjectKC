#pragma once

#include "CoreMinimal.h"
#include "Input/Events.h"
#include "Input/Reply.h"
#include "ProjectKC/UI/Common/Widget/KCUserWidget.h"
#include "TimerManager.h"
#include "KCResultScreen.generated.h"

class UTextBlock;
class UButton;
class UWidgetAnimation;
class UKCColorStyle;
class UKCResultViewModel;

UCLASS(Abstract, Blueprintable)
class PROJECTKC_API UKCResultScreen : public UKCUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void RefreshResultScreen();

	UFUNCTION(BlueprintImplementableEvent, Category = "KC|UI")
	void OnRestartRequested();

	UFUNCTION(BlueprintImplementableEvent, Category = "KC|UI")
	void OnReturnToLobbyRequested();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeApplyColorStyle(const UKCColorStyle* InColorStyle) override;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "KC|UI")
	TObjectPtr<UTextBlock> SecondCountText;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "KC|UI")
	TObjectPtr<UTextBlock> TeamText;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "KC|UI")
	TObjectPtr<UButton> BackToLobbyButton;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional), BlueprintReadOnly, Category = "KC|UI")
	TObjectPtr<UWidgetAnimation> ShowResult;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "KC|UI")
	TObjectPtr<UKCResultViewModel> ResultViewModel;

private:
	UFUNCTION()
	void HandleBackToLobbyButtonClicked();

	void StartBackToLobbyTimer();
	void StopBackToLobbyTimer();
	void UpdateBackToLobbyTimer();
	void ApplySecondCountText();
	void UpdateWinningTeam();
	void ApplyWinningTeamText();
	void ApplyWinningTeamColor();

	FTimerHandle BackToLobbyTimerHandle;
};
