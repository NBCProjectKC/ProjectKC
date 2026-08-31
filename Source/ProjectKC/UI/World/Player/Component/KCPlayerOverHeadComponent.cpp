#include "ProjectKC/UI/World/Player/Component/KCPlayerOverHeadComponent.h"

#include "Components/WidgetComponent.h"
#include "ProjectKC/UI/World/Player/ViewModel/KCPlayerOverHeadViewModel.h"
#include "ProjectKC/UI/World/Player/Widget/KCPlayerOverHeadWidget.h"

UKCPlayerOverHeadComponent::UKCPlayerOverHeadComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UKCPlayerOverHeadComponent::BeginPlay()
{
	Super::BeginPlay();

	EnsureViewModel();
	EnsureWidgetComponent();
	ApplyDisplayInfoToWidget();
}

void UKCPlayerOverHeadComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (PlayerOverHeadWidgetComponent)
	{
		PlayerOverHeadWidgetComponent->DestroyComponent();
		PlayerOverHeadWidgetComponent = nullptr;
	}

	PlayerOverHeadViewModel = nullptr;

	Super::EndPlay(EndPlayReason);
}

void UKCPlayerOverHeadComponent::SetPlayerDisplayInfo(
	const FKCPlayerDisplayInfoStruct& NewDisplayInfo)
{
	EnsureViewModel();
	if (PlayerOverHeadViewModel)
	{
		PlayerOverHeadViewModel->SetPlayerDisplayInfo(NewDisplayInfo);
	}

	EnsureWidgetComponent();
	ApplyDisplayInfoToWidget();
}

void UKCPlayerOverHeadComponent::ClearPlayerDisplayInfo()
{
	EnsureViewModel();
	if (PlayerOverHeadViewModel)
	{
		PlayerOverHeadViewModel->ClearPlayerDisplayInfo();
	}

	ApplyDisplayInfoToWidget();
}

void UKCPlayerOverHeadComponent::EnsureViewModel()
{
	if (!PlayerOverHeadViewModel)
	{
		PlayerOverHeadViewModel = NewObject<UKCPlayerOverHeadViewModel>(this);
	}
}

void UKCPlayerOverHeadComponent::EnsureWidgetComponent()
{
	if (PlayerOverHeadWidgetComponent || !PlayerOverHeadWidgetClass)
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner || !Owner->GetRootComponent())
	{
		return;
	}

	PlayerOverHeadWidgetComponent = NewObject<UWidgetComponent>(
		Owner,
		TEXT("PlayerOverHeadWidget"));
	if (!PlayerOverHeadWidgetComponent)
	{
		return;
	}

	PlayerOverHeadWidgetComponent->SetupAttachment(Owner->GetRootComponent());
	PlayerOverHeadWidgetComponent->SetRelativeLocation(WidgetRelativeLocation);
	PlayerOverHeadWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	PlayerOverHeadWidgetComponent->SetDrawSize(WidgetDrawSize);
	PlayerOverHeadWidgetComponent->SetWidgetClass(PlayerOverHeadWidgetClass);
	PlayerOverHeadWidgetComponent->SetVisibility(false);
	PlayerOverHeadWidgetComponent->RegisterComponent();
}

void UKCPlayerOverHeadComponent::ApplyDisplayInfoToWidget()
{
	if (UKCPlayerOverHeadWidget* PlayerOverHeadWidget = GetPlayerOverHeadWidget())
	{
		PlayerOverHeadWidget->SetViewModel(PlayerOverHeadViewModel);
	}

	if (PlayerOverHeadWidgetComponent && PlayerOverHeadViewModel)
	{
		PlayerOverHeadWidgetComponent->SetVisibility(
			PlayerOverHeadViewModel->IsOverHeadVisible());
	}
}

UKCPlayerOverHeadWidget* UKCPlayerOverHeadComponent::GetPlayerOverHeadWidget() const
{
	if (!PlayerOverHeadWidgetComponent)
	{
		return nullptr;
	}

	return Cast<UKCPlayerOverHeadWidget>(
		PlayerOverHeadWidgetComponent->GetUserWidgetObject());
}
