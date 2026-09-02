#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/UI/Common/Widget/KCUserWidget.h"
#include "KCInteractionPromptWidget.generated.h"

class UTextBlock;
class UKCInteractionPromptViewModel;

UCLASS(Abstract, Blueprintable)
class PROJECTKC_API UKCInteractionPromptWidget : public UKCUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativePreConstruct() override;

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "KC|UI")
	void SetInteractionPrompt(const FText& InputText, const FText& ActionText);

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetViewModel(UKCInteractionPromptViewModel* NewViewModel);

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	UKCInteractionPromptViewModel* GetViewModel() const { return ViewModel; }

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UTextBlock> InteractionKeyText;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UTextBlock> PromptText;
	
protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "KC|UI", meta = (DisplayName = "On View Model Changed"))
	void BP_OnViewModelChanged(UKCInteractionPromptViewModel* NewViewModel);

	void RefreshFromViewModel();

private:
	UPROPERTY(Transient, BlueprintReadOnly, Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UKCInteractionPromptViewModel> ViewModel;
};
