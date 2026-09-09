#include "ProjectKC/UI/Splash/Screen/KCSplashScreen.h"

#include "Animation/WidgetAnimation.h"
#include "Components/Image.h"
#include "Components/Widget.h"

void UKCSplashScreen::NativeConstruct()
{
	Super::NativeConstruct();

	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	SetRenderOpacity(1.0f);

	if (UWidget* RootWidget = GetRootWidget())
	{
		RootWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		RootWidget->SetRenderOpacity(1.0f);
	}

	if (UnrealLogo)
	{
		UnrealLogo->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		UnrealLogo->SetRenderOpacity(1.0f);
	}

	UE_LOG(LogTemp, Log, TEXT("KC Splash widget constructed: Widget=%s UnrealLogo=%s LogoFadeInOut=%s."),
		*GetNameSafe(this),
		*GetNameSafe(UnrealLogo),
		*GetNameSafe(LogoFadeInOut));

	if (LogoFadeInOut)
	{
		FWidgetAnimationDynamicEvent FinishedDelegate;
		FinishedDelegate.BindDynamic(this, &ThisClass::HandleLogoFadeInOutFinished);
		BindToAnimationFinished(LogoFadeInOut, FinishedDelegate);
		PlayAnimation(LogoFadeInOut);
	}
}

void UKCSplashScreen::NativeDestruct()
{
	if (LogoFadeInOut)
	{
		UnbindAllFromAnimationFinished(LogoFadeInOut);
	}

	Super::NativeDestruct();
}

void UKCSplashScreen::HandleLogoFadeInOutFinished()
{
	UE_LOG(LogTemp, Log, TEXT("KC Splash animation finished: Widget=%s."), *GetNameSafe(this));
	SetRenderOpacity(0.0f);
	SetVisibility(ESlateVisibility::Hidden);
	RemoveFromParent();
}
