#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/UI/Common/Core/KCViewModelBase.h"
#include "KCInteractionPromptViewModel.generated.h"

UCLASS(BlueprintType, Blueprintable, meta = (MVVMAllowedContextCreationType = "Manual|CreateInstance"))
class PROJECTKC_API UKCInteractionPromptViewModel : public UKCViewModelBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "KC|UI")
	bool IsVisible() const { return bVisible; }

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetVisible(bool bNewVisible);

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	FText GetInputText() const { return InputText; }

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetInputText(const FText& NewInputText);

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	FText GetActionText() const { return ActionText; }

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetActionText(const FText& NewActionText);

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	AActor* GetTargetActor() const { return TargetActor; }

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetTargetActor(AActor* NewTargetActor);

	UFUNCTION(BlueprintCallable, Category = "KC|UI|Preview")
	void SetPreviewData(bool bNewVisible, const FText& NewInputText, const FText& NewActionText);

private:
	UPROPERTY(BlueprintReadWrite, FieldNotify, Getter = "IsVisible", Setter = "SetVisible", Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	bool bVisible = false;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Getter, Setter, Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	FText InputText;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Getter, Setter, Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	FText ActionText;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Getter, Setter, Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AActor> TargetActor;
};
