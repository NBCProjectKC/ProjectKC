#include "ProjectKC/UI/HUD/Widget/KCHUDPotProgressWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "Animation/WidgetAnimation.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"

void UKCHUDPotProgressWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	SetPotProgress(PreviewPotProgress);
}

void UKCHUDPotProgressWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HideAfterCompletedAnimationTimerHandle);
	}

	Super::NativeDestruct();
}

void UKCHUDPotProgressWidget::SetPotProgress(const FKCPotProgressViewData& PotProgress)
{
	if (!PotProgress.bVisible && !PotProgress.bCompleted)
	{
		if (bShowing && GetVisibility() != ESlateVisibility::Hidden &&
			GetVisibility() != ESlateVisibility::Collapsed)
		{
			PlayHideAnimationThenHide();
		}
		else
		{
			HidePotProgress();
		}
		return;
	}

	ShowPotProgress();
	ApplyProgressMaterial(PotProgress.ProgressPercent);
	ApplyRemainingSeconds(PotProgress.RemainingSeconds);

	if (PotProgress.bCompleted)
	{
		if (!bCompletedAnimationPlayed)
		{
			bCompletedAnimationPlayed = true;
			PlayCompletedAnimationThenHide();
		}
		return;
	}

	bCompletedAnimationPlayed = false;
}

void UKCHUDPotProgressWidget::HidePotProgress()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HideAfterCompletedAnimationTimerHandle);
	}

	ApplyProgressMaterial(0.0f);
	ApplyRemainingSeconds(0);
	bCompletedAnimationPlayed = false;
	SetVisibility(ESlateVisibility::Hidden);
	bShowing = false;
	bHideAnimationPlaying = false;
}

void UKCHUDPotProgressWidget::ShowPotProgress()
{
	if (bShowing && GetVisibility() != ESlateVisibility::Hidden &&
		GetVisibility() != ESlateVisibility::Collapsed)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HideAfterCompletedAnimationTimerHandle);
	}

	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	bShowing = true;
	bHideAnimationPlaying = false;

	if (PopUp)
	{
		PlayAnimation(PopUp);
	}
}

void UKCHUDPotProgressWidget::ApplyProgressMaterial(float ProgressPercent)
{
	if (!Team1PotProgress || ProgressParameterName.IsNone())
	{
		return;
	}

	if (UMaterialInstanceDynamic* DynamicMaterial = Team1PotProgress->GetDynamicMaterial())
	{
		DynamicMaterial->SetScalarParameterValue(
			ProgressParameterName,
			FMath::Clamp(ProgressPercent, 0.0f, 1.0f));
	}
}

void UKCHUDPotProgressWidget::ApplyRemainingSeconds(int32 RemainingSeconds)
{
	if (!Team1PotProgressText)
	{
		return;
	}

	Team1PotProgressText->SetText(FText::FromString(FString::Printf(
		TEXT("%ds"),
		FMath::Max(0, RemainingSeconds))));
}

void UKCHUDPotProgressWidget::PlayCompletedAnimationThenHide()
{
	BP_OnPotProgressCompleted();

	PlayHideAnimationThenHide();
}

void UKCHUDPotProgressWidget::PlayHideAnimationThenHide()
{
	if (bHideAnimationPlaying)
	{
		return;
	}

	if (!Pop)
	{
		HidePotProgress();
		return;
	}

	bHideAnimationPlaying = true;
	PlayAnimation(Pop);

	if (UWorld* World = GetWorld())
	{
		const float AnimationDuration = FMath::Max(Pop->GetEndTime(), 0.01f);
		World->GetTimerManager().SetTimer(
			HideAfterCompletedAnimationTimerHandle,
			this,
			&ThisClass::HidePotProgress,
			AnimationDuration,
			false);
	}
}
