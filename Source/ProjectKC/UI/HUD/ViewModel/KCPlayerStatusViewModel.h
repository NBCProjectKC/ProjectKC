#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/UI/Common/Core/KCViewModelBase.h"
#include "KCPlayerStatusViewModel.generated.h"

UCLASS(BlueprintType, Blueprintable, meta = (MVVMAllowedContextCreationType = "Manual|CreateInstance"))
class PROJECTKC_API UKCPlayerStatusViewModel : public UKCViewModelBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "KC|UI")
	FText GetDisplayName() const { return DisplayName; }

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetDisplayName(const FText& NewDisplayName);

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	float GetHealthRatio() const { return HealthRatio; }

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetHealthRatio(float NewHealthRatio);

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	bool IsDowned() const { return bDowned; }

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetDowned(bool bNewDowned);

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	FText GetHeldItemName() const { return HeldItemName; }

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetHeldItemName(const FText& NewHeldItemName);

	UFUNCTION(BlueprintCallable, Category = "KC|UI|Preview")
	void SetPreviewData(const FText& NewDisplayName, float NewHealthRatio, bool bNewDowned, const FText& NewHeldItemName);

private:
	UPROPERTY(BlueprintReadWrite, FieldNotify, Getter, Setter, Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	FText DisplayName;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Getter, Setter, Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	float HealthRatio = 1.0f;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Getter = "IsDowned", Setter = "SetDowned", Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	bool bDowned = false;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Getter, Setter, Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	FText HeldItemName;
};
