#include "ProjectKC/UI/Interaction/Component/KCPlayerInteractionPromptComponent.h"

#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "ProjectKC/Interaction/Interface/KCInteractableInterface.h"
#include "ProjectKC/Item/Component/KCHeldItemComponent.h"
#include "ProjectKC/Item/KCWorldItemActor.h"
#include "ProjectKC/Player/Interaction/KCPlayerInteractionComponent.h"
#include "ProjectKC/UI/Common/Core/KCUISettings.h"
#include "ProjectKC/UI/Interaction/Data/KCInteractionPromptRegistry.h"
#include "ProjectKC/UI/Interaction/ViewModel/KCInteractionPromptViewModel.h"
#include "ProjectKC/UI/Interaction/Widget/KCInteractionPromptWidget.h"

UKCPlayerInteractionPromptComponent::UKCPlayerInteractionPromptComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UKCPlayerInteractionPromptComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializeLocalPrompt();
}

void UKCPlayerInteractionPromptComponent::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	UnbindObservedHeldItem();
	UnbindHeldItemComponent();
	UnbindInteractionComponent();

	if (InteractionWidgetComponent)
	{
		InteractionWidgetComponent->DestroyComponent();
		InteractionWidgetComponent = nullptr;
	}

	InteractionPromptViewModel = nullptr;
	CurrentTargetActor.Reset();
	bLocalPromptInitialized = false;

	Super::EndPlay(EndPlayReason);
}

void UKCPlayerInteractionPromptComponent::TickComponent(
	const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bLocalPromptInitialized && !InitializeLocalPrompt())
	{
		return;
	}

	UpdateWidgetLocation();
}

bool UKCPlayerInteractionPromptComponent::ShouldCreateLocalPrompt() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	return OwnerPawn && OwnerPawn->IsLocallyControlled();
}

bool UKCPlayerInteractionPromptComponent::InitializeLocalPrompt()
{
	if (!ShouldCreateLocalPrompt())
	{
		return false;
	}

	EnsurePromptAssets();
	EnsureViewModel();
	EnsureWidgetComponent();
	BindInteractionComponent();
	BindHeldItemComponent();
	bLocalPromptInitialized =
		InteractionPromptViewModel &&
		InteractionWidgetComponent &&
		BoundInteractionComponent.IsValid();
	if (!bLocalPromptInitialized)
	{
		return false;
	}

	SetTargetActor(BoundInteractionComponent.IsValid()
		? BoundInteractionComponent->GetCurrentBestInteractable()
		: nullptr);
	return true;
}

void UKCPlayerInteractionPromptComponent::BindInteractionComponent()
{
	if (BoundInteractionComponent.IsValid())
	{
		return;
	}

	AActor* Owner = GetOwner();
	UKCPlayerInteractionComponent* InteractionComponent = Owner
		? Owner->FindComponentByClass<UKCPlayerInteractionComponent>()
		: nullptr;
	if (!InteractionComponent)
	{
		return;
	}

	BoundInteractionComponent = InteractionComponent;
	InteractionComponent->OnBestInteractableChanged.AddUObject(
		this,
		&ThisClass::HandleBestInteractableChanged);
}

void UKCPlayerInteractionPromptComponent::UnbindInteractionComponent()
{
	if (UKCPlayerInteractionComponent* InteractionComponent =
		BoundInteractionComponent.Get())
	{
		InteractionComponent->OnBestInteractableChanged.RemoveAll(this);
	}

	BoundInteractionComponent.Reset();
}

void UKCPlayerInteractionPromptComponent::BindHeldItemComponent()
{
	if (BoundHeldItemComponent.IsValid())
	{
		return;
	}

	AActor* Owner = GetOwner();
	UKCHeldItemComponent* HeldItemComponent = Owner
		? Owner->FindComponentByClass<UKCHeldItemComponent>()
		: nullptr;
	if (!HeldItemComponent)
	{
		return;
	}

	BoundHeldItemComponent = HeldItemComponent;
	HeldItemComponent->OnHeldItemChanged.AddUniqueDynamic(
		this,
		&ThisClass::HandleHeldItemChanged);
	BindObservedHeldItem(HeldItemComponent->GetHeldItem());
}

void UKCPlayerInteractionPromptComponent::UnbindHeldItemComponent()
{
	if (UKCHeldItemComponent* HeldItemComponent = BoundHeldItemComponent.Get())
	{
		HeldItemComponent->OnHeldItemChanged.RemoveDynamic(
			this,
			&ThisClass::HandleHeldItemChanged);
	}

	BoundHeldItemComponent.Reset();
}

void UKCPlayerInteractionPromptComponent::BindObservedHeldItem(
	AKCWorldItemActor* NewHeldItem)
{
	UnbindObservedHeldItem();

	if (!IsValid(NewHeldItem))
	{
		return;
	}

	ObservedHeldItem = NewHeldItem;
	NewHeldItem->OnItemBroken.AddUniqueDynamic(
		this,
		&ThisClass::HandleObservedHeldItemBroken);
	NewHeldItem->OnDestroyed.AddUniqueDynamic(
		this,
		&ThisClass::HandleObservedHeldItemDestroyed);
}

void UKCPlayerInteractionPromptComponent::UnbindObservedHeldItem()
{
	if (AKCWorldItemActor* HeldItem = ObservedHeldItem.Get())
	{
		HeldItem->OnItemBroken.RemoveDynamic(
			this,
			&ThisClass::HandleObservedHeldItemBroken);
		HeldItem->OnDestroyed.RemoveDynamic(
			this,
			&ThisClass::HandleObservedHeldItemDestroyed);
	}

	ObservedHeldItem.Reset();
}

void UKCPlayerInteractionPromptComponent::EnsureViewModel()
{
	if (!InteractionPromptViewModel)
	{
		InteractionPromptViewModel =
			NewObject<UKCInteractionPromptViewModel>(this);
	}
}

void UKCPlayerInteractionPromptComponent::EnsureWidgetComponent()
{
	if (InteractionWidgetComponent || !InteractionPromptWidgetClass)
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	InteractionWidgetComponent = NewObject<UWidgetComponent>(
		Owner,
		TEXT("InteractionPromptWidget"));
	if (!InteractionWidgetComponent)
	{
		return;
	}

	if (USceneComponent* RootComponent = Owner->GetRootComponent())
	{
		InteractionWidgetComponent->SetupAttachment(RootComponent);
	}

	InteractionWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	InteractionWidgetComponent->SetDrawSize(WidgetDrawSize);
	InteractionWidgetComponent->SetWidgetClass(InteractionPromptWidgetClass);
	InteractionWidgetComponent->SetVisibility(false);
	InteractionWidgetComponent->RegisterComponent();
	ApplyViewModelToWidget();
}

void UKCPlayerInteractionPromptComponent::EnsurePromptAssets()
{
	const UKCUISettings* UISettings = GetDefault<UKCUISettings>();
	if (!UISettings)
	{
		return;
	}

	if (!InteractionPromptWidgetClass)
	{
		InteractionPromptWidgetClass =
			UISettings->InteractionPromptWidgetClass.LoadSynchronous();
	}

	if (!InteractionPromptRegistry)
	{
		InteractionPromptRegistry =
			UISettings->InteractionPromptRegistry.LoadSynchronous();
	}
}

void UKCPlayerInteractionPromptComponent::HandleBestInteractableChanged(
	AActor* NewTargetActor)
{
	SetTargetActor(NewTargetActor);
}

void UKCPlayerInteractionPromptComponent::HandleHeldItemChanged(
	AKCWorldItemActor* NewHeldItem)
{
	BindObservedHeldItem(NewHeldItem);

	if (!bLocalPromptInitialized && !InitializeLocalPrompt())
	{
		return;
	}

	ClearTargetActor();

	if (UKCPlayerInteractionComponent* InteractionComponent =
		BoundInteractionComponent.Get())
	{
		InteractionComponent->RefreshBestInteractable();
		SetTargetActor(InteractionComponent->GetCurrentBestInteractable());
		return;
	}

	RefreshPrompt();
}

void UKCPlayerInteractionPromptComponent::HandleObservedHeldItemBroken()
{
	UnbindObservedHeldItem();
	ClearTargetActor();
}

void UKCPlayerInteractionPromptComponent::HandleObservedHeldItemDestroyed(
	AActor* DestroyedActor)
{
	UnbindObservedHeldItem();
	ClearTargetActor();
}

void UKCPlayerInteractionPromptComponent::SetTargetActor(AActor* NewTargetActor)
{
	if (CurrentTargetActor.Get() == NewTargetActor)
	{
		RefreshPrompt();
		return;
	}

	CurrentTargetActor = NewTargetActor;
	RefreshPrompt();
}

void UKCPlayerInteractionPromptComponent::ClearTargetActor()
{
	CurrentTargetActor.Reset();

	if (InteractionPromptViewModel)
	{
		InteractionPromptViewModel->ClearPrompt();
	}

	if (InteractionWidgetComponent)
	{
		InteractionWidgetComponent->SetVisibility(false);
	}
}

void UKCPlayerInteractionPromptComponent::RefreshPrompt()
{
	EnsureViewModel();
	EnsureWidgetComponent();

	AActor* TargetActor = CurrentTargetActor.Get();
	if (!TargetActor ||
		!TargetActor->GetClass()->ImplementsInterface(
			UKCInteractableInterface::StaticClass()) ||
		!InteractionPromptRegistry ||
		!InteractionPromptViewModel)
	{
		ClearTargetActor();
		return;
	}

	const FGameplayTag PromptTag =
		IKCInteractableInterface::Execute_GetInteractionPromptTag(
			TargetActor,
			GetOwner());

	FKCInteractionPromptEntry PromptEntry;
	if (!InteractionPromptRegistry->FindPrompt(PromptTag, PromptEntry))
	{
		ClearTargetActor();
		return;
	}

	InteractionPromptViewModel->SetPromptData(TargetActor, PromptEntry);
	ApplyViewModelToWidget();

	if (InteractionWidgetComponent)
	{
		InteractionWidgetComponent->SetVisibility(true);
	}

	UpdateWidgetLocation();
}

void UKCPlayerInteractionPromptComponent::ApplyViewModelToWidget()
{
	if (!InteractionWidgetComponent || !InteractionPromptViewModel)
	{
		return;
	}

	UKCInteractionPromptWidget* PromptWidget =
		Cast<UKCInteractionPromptWidget>(
			InteractionWidgetComponent->GetUserWidgetObject());
	if (PromptWidget)
	{
		PromptWidget->SetViewModel(InteractionPromptViewModel);
	}
}

void UKCPlayerInteractionPromptComponent::UpdateWidgetLocation()
{
	AActor* TargetActor = CurrentTargetActor.Get();
	if (!InteractionWidgetComponent || !TargetActor ||
		!TargetActor->GetClass()->ImplementsInterface(
			UKCInteractableInterface::StaticClass()))
	{
		return;
	}

	const FVector PromptLocation =
		IKCInteractableInterface::Execute_GetInteractionPromptWorldLocation(
			TargetActor,
			GetOwner());
	InteractionWidgetComponent->SetWorldLocation(
		PromptLocation + WidgetWorldLocationOffset);
}
