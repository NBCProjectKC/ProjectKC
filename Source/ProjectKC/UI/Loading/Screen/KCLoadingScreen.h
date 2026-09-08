#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/UI/Common/Widget/KCUserWidget.h"
#include "KCLoadingScreen.generated.h"

class UProgressBar;
class UTextBlock;
class UKCLoadingViewModel;

UCLASS(Abstract, Blueprintable)
class PROJECTKC_API UKCLoadingScreen : public UKCUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "KC|Loading")
	void SetLoadingViewModel(UKCLoadingViewModel* InLoadingViewModel);

	UFUNCTION(BlueprintCallable, Category = "KC|Loading")
	void RefreshFromViewModel();

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|Loading")
	TObjectPtr<UProgressBar> ProgressBar_286;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|Loading")
	TObjectPtr<UTextBlock> Tip_Text;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|Loading")
	TObjectPtr<UTextBlock> TextBlock_72;

private:
	UPROPERTY(Transient)
	TObjectPtr<UKCLoadingViewModel> LoadingViewModel;
};
