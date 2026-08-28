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
	// if (!PotProgress.bVisible)
	// {
	// 	HidePotProgress();
	// 	return;
	// }

	// SetVisibility(ESlateVisibility::SelfHitTestInvisible);
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

	UWidgetAnimation* CompletedAnimation = GetCompletedAnimation();
	if (!CompletedAnimation)
	{
		HidePotProgress();
		return;
	}

	PlayAnimation(CompletedAnimation);

	if (UWorld* World = GetWorld())
	{
		const float AnimationDuration = FMath::Max(CompletedAnimation->GetEndTime(), 0.01f);
		World->GetTimerManager().SetTimer(
			HideAfterCompletedAnimationTimerHandle,
			this,
			&ThisClass::HidePotProgress,
			AnimationDuration,
			false);
	}
}

UWidgetAnimation* UKCHUDPotProgressWidget::GetCompletedAnimation() const
{
	return PopUp ? PopUp.Get() : Pop.Get();
}
