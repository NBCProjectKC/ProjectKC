#include "ProjectKC/UI/World/Player/Component/KCPlayerOverHeadComponent.h"

#include "AbilitySystemComponent.h"
#include "Components/WidgetComponent.h"
#include "ProjectKC/AbilitySystem/Attribute/KCCharacterAttributeSet.h"
#include "ProjectKC/AbilitySystem/Component/KCAbilitySystemComponent.h"
#include "ProjectKC/Player/KCPlayerCharacter.h"
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
	BindStaminaDelegates();
	RefreshStaminaFromOwner();
	ApplyDisplayInfoToWidget();
}

void UKCPlayerOverHeadComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindStaminaDelegates();

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

	RefreshStaminaFromOwner();
	EnsureWidgetComponent();
	ApplyDisplayInfoToWidget();
}

void UKCPlayerOverHeadComponent::ClearPlayerDisplayInfo()
{
	EnsureViewModel();
	if (PlayerOverHeadViewModel)
	{
		PlayerOverHeadViewModel->ClearPlayerDisplayInfo();
		PlayerOverHeadViewModel->SetStamina(0.0f, 0.0f);
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

void UKCPlayerOverHeadComponent::BindStaminaDelegates()
{
	AKCPlayerCharacter* PlayerCharacter = Cast<AKCPlayerCharacter>(GetOwner());
	UKCAbilitySystemComponent* AbilitySystemComponent = PlayerCharacter
		? PlayerCharacter->GetKCAbilitySystemComponent()
		: nullptr;
	if (!AbilitySystemComponent)
	{
		RefreshStaminaFromOwner();
		return;
	}

	if (BoundAbilitySystemComponent.Get() == AbilitySystemComponent &&
		StaminaChangedDelegateHandle.IsValid() &&
		MaxStaminaChangedDelegateHandle.IsValid())
	{
		return;
	}

	UnbindStaminaDelegates();
	BoundAbilitySystemComponent = AbilitySystemComponent;

	FOnGameplayAttributeValueChange& StaminaDelegate =
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			UKCCharacterAttributeSet::GetStaminaAttribute());
	StaminaChangedDelegateHandle = StaminaDelegate.AddUObject(
		this,
		&ThisClass::HandleStaminaChanged);

	FOnGameplayAttributeValueChange& MaxStaminaDelegate =
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			UKCCharacterAttributeSet::GetMaxStaminaAttribute());
	MaxStaminaChangedDelegateHandle = MaxStaminaDelegate.AddUObject(
		this,
		&ThisClass::HandleMaxStaminaChanged);
}

void UKCPlayerOverHeadComponent::UnbindStaminaDelegates()
{
	UKCAbilitySystemComponent* AbilitySystemComponent = BoundAbilitySystemComponent.Get();
	if (!AbilitySystemComponent)
	{
		StaminaChangedDelegateHandle.Reset();
		MaxStaminaChangedDelegateHandle.Reset();
		BoundAbilitySystemComponent.Reset();
		return;
	}

	if (StaminaChangedDelegateHandle.IsValid())
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			UKCCharacterAttributeSet::GetStaminaAttribute()).Remove(StaminaChangedDelegateHandle);
		StaminaChangedDelegateHandle.Reset();
	}

	if (MaxStaminaChangedDelegateHandle.IsValid())
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			UKCCharacterAttributeSet::GetMaxStaminaAttribute()).Remove(MaxStaminaChangedDelegateHandle);
		MaxStaminaChangedDelegateHandle.Reset();
	}

	BoundAbilitySystemComponent.Reset();
}

void UKCPlayerOverHeadComponent::RefreshStaminaFromOwner()
{
	EnsureViewModel();
	if (!PlayerOverHeadViewModel)
	{
		return;
	}

	const AKCPlayerCharacter* PlayerCharacter = Cast<AKCPlayerCharacter>(GetOwner());
	const UKCCharacterAttributeSet* CharacterAttributes = PlayerCharacter
		? PlayerCharacter->GetCharacterAttributes()
		: nullptr;
	if (!CharacterAttributes)
	{
		PlayerOverHeadViewModel->SetStamina(0.0f, 0.0f);
		return;
	}

	PlayerOverHeadViewModel->SetStamina(
		CharacterAttributes->GetStamina(),
		CharacterAttributes->GetMaxStamina());
}

void UKCPlayerOverHeadComponent::HandleStaminaChanged(
	const FOnAttributeChangeData& ChangeData)
{
	RefreshStaminaFromOwner();
}

void UKCPlayerOverHeadComponent::HandleMaxStaminaChanged(
	const FOnAttributeChangeData& ChangeData)
{
	RefreshStaminaFromOwner();
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