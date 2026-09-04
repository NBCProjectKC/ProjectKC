#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/UI/Common/Widget/KCUserWidget.h"
#include "KCPlayerOverHeadWidget.generated.h"

class UProgressBar;
class UTextBlock;
class UKCColorStyle;
class UKCPlayerOverHeadViewModel;

UCLASS(Abstract, Blueprintable)
class PROJECTKC_API UKCPlayerOverHeadWidget : public UKCUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetViewModel(UKCPlayerOverHeadViewModel* InViewModel);

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	UKCPlayerOverHeadViewModel* GetViewModel() const { return PlayerOverHeadViewModel; }

protected:
	virtual void NativeApplyColorStyle(const UKCColorStyle* InColorStyle) override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UTextBlock> PlayerNameText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UProgressBar> SteminaProgressBar;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "KC|UI")
	TObjectPtr<UKCPlayerOverHeadViewModel> PlayerOverHeadViewModel;

private:
	void BindViewModel();
	void UnbindViewModel();
	void RefreshFromViewModel();
	void RefreshTeamColor();
	void RefreshStamina();
	void HandlePlayerNameChanged(const FText& NewPlayerName);
	void HandleTeamIdChanged(int32 NewTeamId);
	void HandleVisibilityChanged(bool bNewVisible);
	void HandleStaminaChanged(float NewStaminaPercent, bool bNewVisible);
};