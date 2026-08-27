#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/UI/Common/Core/KCViewModelBase.h"
#include "KCLoadingViewModel.generated.h"

UCLASS(BlueprintType, Blueprintable, meta = (MVVMAllowedContextCreationType = "Manual|CreateInstance"))
class PROJECTKC_API UKCLoadingViewModel : public UKCViewModelBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "KC|UI")
	float GetProgress() const { return Progress; }

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetProgress(float NewProgress);

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	FText GetLoadingText() const { return LoadingText; }

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetLoadingText(const FText& NewLoadingText);

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	FText GetTipText() const { return TipText; }

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetTipText(const FText& NewTipText);

	UFUNCTION(BlueprintCallable, Category = "KC|UI|Preview")
	void SetPreviewData(float NewProgress, const FText& NewLoadingText, const FText& NewTipText);

private:
	UPROPERTY(BlueprintReadWrite, FieldNotify, Getter, Setter, Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	float Progress = 0.0f;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Getter, Setter, Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	FText LoadingText;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Getter, Setter, Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	FText TipText;
};
