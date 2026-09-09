#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/UI/Common/Widget/KCUserWidget.h"
#include "KCSplashScreen.generated.h"

class UImage;
class UWidgetAnimation;

UCLASS(Abstract, Blueprintable)
class PROJECTKC_API UKCSplashScreen : public UKCUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional), BlueprintReadOnly, Category = "KC|Splash")
	TObjectPtr<UWidgetAnimation> LogoFadeInOut;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "KC|Splash")
	TObjectPtr<UImage> UnrealLogo;

private:
	UFUNCTION()
	void HandleLogoFadeInOutFinished();
};
