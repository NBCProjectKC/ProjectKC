#include "ProjectKC/UI/HUD/Widget/KCHUDPotProgressWidget.h"

#include "Animation/WidgetAnimation.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProjectKC/UI/Common/Style/KCColorStyle.h"
#include "TimerManager.h"

void UKCHUDPotProgressWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	ApplyTeamColor();
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

void UKCHUDPotProgressWidget::SetTeamId(int32 NewTeamId)
{
	NewTeamId = FMath::Max(0, NewTeamId);
	if (TeamId == NewTeamId)
	{
		return;
	}

	TeamId = NewTeamId;
	ApplyTeamColor();
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
	ApplyTeamColor();
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

void UKCHUDPotProgressWidget::NativeApplyColorStyle(const UKCColorStyle* InColorStyle)
{
	Super::NativeApplyColorStyle(InColorStyle);

	ApplyTeamColor();
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

void UKCHUDPotProgressWidget::ApplyTeamColor()
{
	const UKCColorStyle* CurrentColorStyle = GetColorStyle();
	if (!CurrentColorStyle || !CurrentColorStyle->TeamColors.IsValidIndex(TeamId))
	{
		return;
	}

	const FLinearColor TeamColor = CurrentColorStyle->TeamColors[TeamId];
	if (Team1PotProgress)
	{
		Team1PotProgress->SetColorAndOpacity(TeamColor);
	}

	if (Team1PotProgressText)
	{
		Team1PotProgressText->SetColorAndOpacity(FSlateColor(TeamColor));
	}
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