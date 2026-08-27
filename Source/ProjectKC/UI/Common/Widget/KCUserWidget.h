#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "KCUserWidget.generated.h"

class UKCColorStyle;

UCLASS(Abstract, Blueprintable)
class PROJECTKC_API UKCUserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativePreConstruct() override;

	UFUNCTION(BlueprintPure, Category = "KC|UI|Style")
	UKCColorStyle* GetColorStyle() const;

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "KC|UI|Style", meta = (DisplayName = "Apply Color Style"))
	void BP_ApplyColorStyle(UKCColorStyle* InColorStyle);

	virtual void NativeApplyColorStyle(const UKCColorStyle* InColorStyle);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|UI|Style")
	TObjectPtr<UKCColorStyle> ColorStyle;
};
