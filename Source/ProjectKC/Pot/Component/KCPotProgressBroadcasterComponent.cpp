#include "ProjectKC/Pot/Component/KCPotProgressBroadcasterComponent.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Components/ProgressBar.h"
#include "Components/WidgetComponent.h"
#include "ProjectKC/AbilitySystem/Attribute/KCCookingProgressAttributeSet.h"
#include "ProjectKC/Messages/KCGameplayTags.h"
#include "ProjectKC/Messages/Struct/KCPotProgressChangedStruct.h"
#include "ProjectKC/Pot/KCPotActor.h"

UKCPotProgressBroadcasterComponent::UKCPotProgressBroadcasterComponent()
{
	SetIsReplicatedByDefault(true);
}

void UKCPotProgressBroadcasterComponent::BeginPlay()
{
	Super::BeginPlay();

	EnsureWorldWidget();
	ApplyWorldWidgetProgress(0.0f, false);
}

void UKCPotProgressBroadcasterComponent::NotifyCookingStarted()
{
	const AKCPotActor* PotActor = GetPotActor();
	if (!PotActor)
	{
		return;
	}

	MulticastPotProgressStarted(
		PotActor->GetAssignedTeamId(),
		PotActor->GetActiveRecipeRowName(),
		GetRemainingCookingSeconds(
			PotActor->GetCookingProgressAttributes(),
			PotActor->GetActiveProgressSpeedPerSecond()));
}

void UKCPotProgressBroadcasterComponent::NotifyCookingProgress()
{
	const AKCPotActor* PotActor = GetPotActor();
	if (!PotActor)
	{
		return;
	}

	MulticastPotProgressUpdated(
		PotActor->GetAssignedTeamId(),
		PotActor->GetActiveRecipeRowName(),
		GetCookingProgressPercent(PotActor->GetCookingProgressAttributes()),
		GetRemainingCookingSeconds(
			PotActor->GetCookingProgressAttributes(),
			PotActor->GetActiveProgressSpeedPerSecond()));
}

void UKCPotProgressBroadcasterComponent::NotifyCookingCompleted()
{
	const AKCPotActor* PotActor = GetPotActor();
	if (!PotActor)
	{
		return;
	}

	MulticastPotProgressCompleted(PotActor->GetAssignedTeamId(), PotActor->GetActiveRecipeRowName());
}

void UKCPotProgressBroadcasterComponent::NotifyCookingHidden()
{
	const AKCPotActor* PotActor = GetPotActor();
	if (!PotActor)
	{
		return;
	}

	MulticastPotProgressHidden(PotActor->GetAssignedTeamId());
}

void UKCPotProgressBroadcasterComponent::MulticastPotProgressStarted_Implementation(
	int32 TeamId,
	FName RecipeRowName,
	int32 RemainingSeconds)
{
	BroadcastPotProgress(TeamId, RecipeRowName, 0.0f, RemainingSeconds, true, false);
}

void UKCPotProgressBroadcasterComponent::MulticastPotProgressUpdated_Implementation(
	int32 TeamId,
	FName RecipeRowName,
	float ProgressPercent,
	int32 RemainingSeconds)
{
	BroadcastPotProgress(TeamId, RecipeRowName, ProgressPercent, RemainingSeconds, true, false);
}

void UKCPotProgressBroadcasterComponent::MulticastPotProgressCompleted_Implementation(
	int32 TeamId,
	FName RecipeRowName)
{
	BroadcastPotProgress(TeamId, RecipeRowName, 1.0f, 0, true, true);
}

void UKCPotProgressBroadcasterComponent::MulticastPotProgressHidden_Implementation(
	int32 TeamId)
{
	BroadcastPotProgress(TeamId, NAME_None, 0.0f, 0, false, false);
}

void UKCPotProgressBroadcasterComponent::BroadcastPotProgress(
	int32 TeamId,
	FName RecipeRowName,
	float ProgressPercent,
	int32 RemainingSeconds,
	bool bVisible,
	bool bCompleted)
{
	FKCPotProgressChangedStruct Message;
	Message.TeamId = TeamId;
	Message.RecipeRowName = RecipeRowName;
	Message.ProgressPercent = FMath::Clamp(ProgressPercent, 0.0f, 1.0f);
	Message.RemainingSeconds = FMath::Max(0, RemainingSeconds);
	Message.bVisible = bVisible;
	Message.bCompleted = bCompleted;

	UGameplayMessageSubsystem::Get(this).BroadcastMessage(
		KCGameplayTags::Message_Game_PotProgressChanged,
		Message);

	ApplyWorldWidgetProgress(Message.ProgressPercent, Message.bVisible && !Message.bCompleted);
}

void UKCPotProgressBroadcasterComponent::EnsureWorldWidget()
{
	if (PotProgressWidgetComponent || !PotProgressWidgetClass)
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	PotProgressWidgetComponent = NewObject<UWidgetComponent>(
		Owner,
		TEXT("PotProgressWidget"));
	if (!PotProgressWidgetComponent)
	{
		return;
	}

	PotProgressWidgetComponent->SetupAttachment(Owner->GetRootComponent());
	PotProgressWidgetComponent->SetRelativeLocation(WidgetRelativeLocation);
	PotProgressWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	PotProgressWidgetComponent->SetDrawSize(WidgetDrawSize);
	PotProgressWidgetComponent->SetWidgetClass(PotProgressWidgetClass);
	PotProgressWidgetComponent->SetVisibility(false);
	PotProgressWidgetComponent->RegisterComponent();
}

void UKCPotProgressBroadcasterComponent::ApplyWorldWidgetProgress(
	float ProgressPercent,
	bool bVisible)
{
	EnsureWorldWidget();

	if (!PotProgressWidgetComponent)
	{
		return;
	}

	PotProgressWidgetComponent->SetVisibility(bVisible);
	if (!bVisible)
	{
		return;
	}

	UProgressBar* ProgressBar = CachedProgressBar.Get();
	if (!ProgressBar)
	{
		UUserWidget* UserWidget = PotProgressWidgetComponent->GetUserWidgetObject();
		ProgressBar = UserWidget && UserWidget->WidgetTree
			? UserWidget->WidgetTree->FindWidget<UProgressBar>(ProgressBarWidgetName)
			: nullptr;
		CachedProgressBar = ProgressBar;
	}

	if (ProgressBar)
	{
		ProgressBar->SetPercent(FMath::Clamp(ProgressPercent, 0.0f, 1.0f));
	}
}

const AKCPotActor* UKCPotProgressBroadcasterComponent::GetPotActor() const
{
	return Cast<AKCPotActor>(GetOwner());
}

int32 UKCPotProgressBroadcasterComponent::GetRemainingCookingSeconds(
	const UKCCookingProgressAttributeSet* CookingProgressAttributes,
	float ProgressSpeedPerSecond) const
{
	if (!CookingProgressAttributes || ProgressSpeedPerSecond <= 0.0f)
	{
		return 0;
	}

	const float RemainingProgress = FMath::Max(
		0.0f,
		CookingProgressAttributes->GetMaxCookingProgress() -
		CookingProgressAttributes->GetCookingProgress());
	return FMath::CeilToInt(RemainingProgress / ProgressSpeedPerSecond);
}

float UKCPotProgressBroadcasterComponent::GetCookingProgressPercent(
	const UKCCookingProgressAttributeSet* CookingProgressAttributes) const
{
	if (!CookingProgressAttributes ||
		CookingProgressAttributes->GetMaxCookingProgress() <= 0.0f)
	{
		return 0.0f;
	}

	return CookingProgressAttributes->GetCookingProgress() /
		CookingProgressAttributes->GetMaxCookingProgress();
}
