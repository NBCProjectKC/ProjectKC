#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "ProjectKC/UI/Common/Core/KCViewModelBase.h"
#include "ProjectKC/UI/Interaction/Data/KCInteractionPromptRegistry.h"
#include "KCInteractionPromptViewModel.generated.h"

class UTexture2D;

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
	bool UsesInputKey() const { return bUsesInputKey; }

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetUsesInputKey(bool bNewUsesInputKey);

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	FKey GetInputKey() const { return InputKey; }

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetInputKey(FKey NewInputKey);

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	FText GetActionText() const { return ActionText; }

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetActionText(const FText& NewActionText);

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	AActor* GetTargetActor() const { return TargetActor; }

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetTargetActor(AActor* NewTargetActor);

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	TSoftObjectPtr<UTexture2D> GetIcon() const { return Icon; }

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetIcon(TSoftObjectPtr<UTexture2D> NewIcon);

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetPromptData(AActor* NewTargetActor, const FKCInteractionPromptEntry& PromptEntry);

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void ClearPrompt();

	UFUNCTION(BlueprintCallable, Category = "KC|UI|Preview")
	void SetPreviewData(bool bNewVisible, const FText& NewInputText, const FText& NewActionText);

private:
	UPROPERTY(BlueprintReadWrite, FieldNotify, Getter = "IsVisible", Setter = "SetVisible", Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	bool bVisible = false;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Getter, Setter, Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	FText InputText;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Getter = "UsesInputKey", Setter = "SetUsesInputKey", Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	bool bUsesInputKey = false;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Getter, Setter, Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	FKey InputKey;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Getter, Setter, Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	FText ActionText;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Getter, Setter, Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AActor> TargetActor;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Getter, Setter, Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	TSoftObjectPtr<UTexture2D> Icon;
};
