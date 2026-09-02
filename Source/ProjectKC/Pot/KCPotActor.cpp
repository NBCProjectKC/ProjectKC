#include "ProjectKC/Pot/KCPotActor.h"

#include "Components/BoxComponent.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Net/UnrealNetwork.h"
#include "ProjectKC/AbilitySystem/Attribute/KCCookingProgressAttributeSet.h"
#include "ProjectKC/AbilitySystem/Component/KCAbilitySystemComponent.h"
#include "ProjectKC/AbilitySystem/Effect/KCGE_CookingProgressIncrease.h"
#include "ProjectKC/AbilitySystem/Tag/KCAbilityGameplayTags.h"
#include "ProjectKC/GameSystem/KCGameState.h"
#include "ProjectKC/GameSystem/Recipe/KCDishFinishedStruct.h"
#include "ProjectKC/GameSystem/Recipe/KCDishRuinedStruct.h"
#include "ProjectKC/GameSystem/Recipe/KCRecipeCompletedStruct.h"
#include "ProjectKC/GameSystem/Recipe/KCRecipeStruct.h"
#include "ProjectKC/Item/Component/KCHeldItemComponent.h"
#include "ProjectKC/Item/Definition/KCItemDefinition.h"
#include "ProjectKC/Item/KCWorldItemActor.h"
#include "ProjectKC/Player/KCPlayerState.h"
#include "ProjectKC/Messages/KCGameplayTags.h"
#include "ProjectKC/Messages/Struct/KCIngredientSubmittedStruct.h"
#include "ProjectKC/Pot/Component/KCPotProgressBroadcasterComponent.h"
#include "TimerManager.h"

namespace KCPot
{
	constexpr float MaxCookingProgress = 100.0f;
	constexpr float ProgressTickInterval = 0.1f;
}

AKCPotActor::AKCPotActor()
{
	bReplicates = true;

	InteractionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionVolume"));
	SetRootComponent(InteractionVolume);
	InteractionVolume->SetCollisionProfileName(TEXT("Trigger"));
	InteractionVolume->SetBoxExtent(FVector(100.0f));
	InteractionVolume->SetGenerateOverlapEvents(true);
	InteractionVolume->ComponentTags.AddUnique(TEXT("Interactable"));
	InteractionVolume->OnComponentBeginOverlap.AddDynamic(
		this,
		&AKCPotActor::OnInteractionVolumeBeginOverlap);

	AbilitySystemComponent =
		CreateDefaultSubobject<UKCAbilitySystemComponent>(TEXT("AbilitySystem"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	CookingProgressAttributes =
		CreateDefaultSubobject<UKCCookingProgressAttributeSet>(TEXT("CookingProgress"));

	PotProgressBroadcaster =
		CreateDefaultSubobject<UKCPotProgressBroadcasterComponent>(TEXT("PotProgressBroadcaster"));
}

UAbilitySystemComponent* AKCPotActor::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AKCPotActor::BeginPlay()
{
	Super::BeginPlay();
	AbilitySystemComponent->InitAbilityActorInfo(this, this);

	if (!HasAuthority())
	{
		return;
	}

	CookingProgressAttributes->InitMaxCookingProgress(
		KCPot::MaxCookingProgress);
	CookingProgressAttributes->InitCookingProgress(0.0f);
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[Pot] State initialized: Pot=%s, State=Idle"),
		*GetName());
	CookingProgressChangedDelegateHandle =
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			UKCCookingProgressAttributeSet::GetCookingProgressAttribute())
			.AddUObject(this, &AKCPotActor::HandleCookingProgressChanged);

	RecipeCompletedListenerHandle =
		UGameplayMessageSubsystem::Get(this).RegisterListener<FKCRecipeCompletedStruct>(
			KCGameplayTags::Message_Recipe_Completed,
			this,
			&AKCPotActor::HandleRecipeCompleted);
	DishRuinedListenerHandle =
		UGameplayMessageSubsystem::Get(this).RegisterListener<FKCDishRuinedStruct>(
			KCGameplayTags::Message_Dish_Ruined,
			this,
			&AKCPotActor::HandleDishRuined);
}

void AKCPotActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(CookingTimerHandle);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		UKCCookingProgressAttributeSet::GetCookingProgressAttribute())
		.Remove(CookingProgressChangedDelegateHandle);
	if (HasAuthority())
	{
		UGameplayMessageSubsystem::Get(this).UnregisterListener(
			RecipeCompletedListenerHandle);
		UGameplayMessageSubsystem::Get(this).UnregisterListener(
			DishRuinedListenerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void AKCPotActor::Interact_Implementation(AActor* Interactor)
{
	if (HasAuthority() && IsValid(Interactor))
	{
		const bool bSubmitted = TrySubmitHeldIngredient(*Interactor);
		UE_LOG(
			LogTemp,
			Log,
			TEXT("[Pot] Interact: Pot=%s, Interactor=%s, IngredientSubmitted=%s"),
			*GetName(),
			*Interactor->GetName(),
			bSubmitted ? TEXT("true") : TEXT("false"));
	}
}

FGameplayTag AKCPotActor::GetInteractionPromptTag_Implementation(
	AActor* Interactor) const
{
	const UKCHeldItemComponent* HeldItemComponent = Interactor
		? Interactor->FindComponentByClass<UKCHeldItemComponent>()
		: nullptr;
	if (PotState != EKCPotStateType::Idle || !HeldItemComponent ||
		!HeldItemComponent->HasHeldItem())
	{
		return FGameplayTag();
	}

	return KCGameplayTags::Interaction_Pot_SubmitIngredient;
}

FVector AKCPotActor::GetInteractionPromptWorldLocation_Implementation(
	AActor* Interactor) const
{
	return InteractionVolume ? InteractionVolume->Bounds.Origin : GetActorLocation();
}

void AKCPotActor::OnInteractionVolumeBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (HasAuthority() && Cast<APawn>(OtherActor))
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("[Pot] Interaction range entered: Pot=%s, Actor=%s"),
			*GetName(),
			*OtherActor->GetName());
	}
}

bool AKCPotActor::ResetPot()
{
	if (!HasAuthority() || PotState == EKCPotStateType::Cooking)
	{
		return false;
	}

	ActiveRecipeRowName = NAME_None;
	ActiveProgressSpeedPerSecond = 0.0f;
	PotState = EKCPotStateType::Idle;
	bRestoringCookingProgress = true;
	CookingProgressAttributes->InitCookingProgress(0.0f);
	bRestoringCookingProgress = false;
	ForceNetUpdate();
	return true;
}

void AKCPotActor::HandleRecipeCompleted(
	FGameplayTag Channel,
	const FKCRecipeCompletedStruct& Message)
{
	if (!HasAuthority() || PotState != EKCPotStateType::Idle ||
		Message.TeamId != AssignedTeamId || !RecipeDataTable)
	{
		return;
	}

	const FKCRecipeStruct* Recipe = RecipeDataTable->FindRow<FKCRecipeStruct>(
		Message.RecipeRowName,
		TEXT("PotRecipeCompleted"));
	if (!Recipe || !FMath::IsFinite(Recipe->ProgressSpeedPerSecond) ||
		Recipe->ProgressSpeedPerSecond <= 0.0f)
	{
		return;
	}

	StartCooking(Message.RecipeRowName, Recipe->ProgressSpeedPerSecond);
}

void AKCPotActor::HandleDishRuined(
	FGameplayTag Channel,
	const FKCDishRuinedStruct& Message)
{
	if (HasAuthority() && PotState == EKCPotStateType::Idle &&
		Message.TeamId == AssignedTeamId)
	{
		if (PotProgressBroadcaster)
		{
			PotProgressBroadcaster->NotifyCookingHidden();
		}
		MulticastCookingRuined();
	}
}

bool AKCPotActor::IsRegisteredIngredient(const FGameplayTag& IngredientId) const
{
	const AKCGameState* GameState = GetWorld()
		? GetWorld()->GetGameState<AKCGameState>()
		: nullptr;
	if (!RecipeDataTable || !GameState)
	{
		return false;
	}

	for (const FName& RowName : GameState->GetActiveRecipes())
	{
		const FKCRecipeStruct* Recipe = RecipeDataTable->FindRow<FKCRecipeStruct>(
			RowName,
			TEXT("PotRegisteredIngredient"));
		if (Recipe && Recipe->RequiredIngredients.Contains(IngredientId))
		{
			return true;
		}
	}

	return false;
}

bool AKCPotActor::TrySubmitHeldIngredient(AActor& Interactor)
{
	if (PotState != EKCPotStateType::Idle)
	{
		return false;
	}

	const APawn* InteractorPawn = Cast<APawn>(&Interactor);
	const AKCPlayerState* PlayerState = InteractorPawn
		? InteractorPawn->GetPlayerState<AKCPlayerState>()
		: nullptr;
	if (!PlayerState || PlayerState->GetTeamId() != AssignedTeamId)
	{
		return false;
	}

	UKCHeldItemComponent* HeldItemComponent =
		Interactor.FindComponentByClass<UKCHeldItemComponent>();
	AKCWorldItemActor* HeldItem = HeldItemComponent
		? HeldItemComponent->GetHeldItem()
		: nullptr;
	UKCItemDefinition* ItemDefinition = HeldItem
		? HeldItem->GetItemDefinition()
		: nullptr;
	if (!HeldItemComponent || !HeldItem || !ItemDefinition ||
		!ItemDefinition->ItemId.IsValid())
	{
		return false;
	}

	if (!IsRegisteredIngredient(ItemDefinition->ItemId))
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("[Pot] Ingredient rejected: Pot=%s, ItemId=%s"),
			*GetName(),
			*ItemDefinition->ItemId.ToString());
		return false;
	}

	FKCIngredientSubmittedStruct Message;
	Message.TeamId = AssignedTeamId;
	Message.IngredientId = ItemDefinition->ItemId;
	UGameplayMessageSubsystem::Get(this).BroadcastMessage(
		KCGameplayTags::Message_Ingredient_Submitted,
		Message);

	const bool bConsumed = ConsumeHeldItem(Interactor);
	if (bConsumed)
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("[Pot] Ingredient submitted: Pot=%s, TeamId=%d, ItemId=%s"),
			*GetName(),
			AssignedTeamId,
			*ItemDefinition->ItemId.ToString());
	}

	return bConsumed;
}

bool AKCPotActor::ConsumeHeldItem(AActor& Interactor)
{
	UKCHeldItemComponent* HeldItemComponent =
		Interactor.FindComponentByClass<UKCHeldItemComponent>();
	AKCWorldItemActor* HeldItem = HeldItemComponent
		? HeldItemComponent->GetHeldItem()
		: nullptr;
	if (!HeldItemComponent || !HeldItem ||
		!HeldItemComponent->DropHeldItem(
			FTransform(HeldItem->GetActorRotation(), GetActorLocation())))
	{
		return false;
	}

	HeldItem->Destroy();
	return true;
}

void AKCPotActor::StartCooking(
	FName RecipeRowName,
	float ProgressSpeedPerSecond)
{
	ActiveRecipeRowName = RecipeRowName;
	ActiveProgressSpeedPerSecond = ProgressSpeedPerSecond;
	PotState = EKCPotStateType::Cooking;
	CookingProgressAttributes->InitCookingProgress(0.0f);
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[Pot] State changed: Pot=%s, State=Cooking, Recipe=%s"),
		*GetName(),
		*RecipeRowName.ToString());
	GetWorldTimerManager().SetTimer(
		CookingTimerHandle,
		this,
		&AKCPotActor::AdvanceCookingProgress,
		KCPot::ProgressTickInterval,
		true);
	if (PotProgressBroadcaster)
	{
		PotProgressBroadcaster->NotifyCookingStarted();
	}
	MulticastCookingStarted();
	ForceNetUpdate();
}

void AKCPotActor::AdvanceCookingProgress()
{
	if (!HasAuthority() || PotState != EKCPotStateType::Cooking)
	{
		return;
	}

	ApplyCookingProgressIncrease(
		ActiveProgressSpeedPerSecond * KCPot::ProgressTickInterval);
	if (CookingProgressAttributes->GetCookingProgress() >=
		CookingProgressAttributes->GetMaxCookingProgress())
	{
		CompleteCooking();
		return;
	}

	if (PotProgressBroadcaster)
	{
		PotProgressBroadcaster->NotifyCookingProgress();
	}
}

void AKCPotActor::CompleteCooking()
{
	GetWorldTimerManager().ClearTimer(CookingTimerHandle);
	PotState = EKCPotStateType::Completed;
	if (PotProgressBroadcaster)
	{
		PotProgressBroadcaster->NotifyCookingCompleted();
	}

	FKCDishFinishedStruct Message;
	Message.TeamId = AssignedTeamId;
	Message.RecipeRowName = ActiveRecipeRowName;
	UGameplayMessageSubsystem::Get(this).BroadcastMessage(
		KCGameplayTags::Message_Dish_Finished,
		Message);

	MulticastCookingCompleted();
	ResetPot();
}

void AKCPotActor::ApplyCookingProgressIncrease(float Amount)
{
	FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(
		UKCGE_CookingProgressIncrease::StaticClass(),
		1.0f,
		AbilitySystemComponent->MakeEffectContext());
	if (SpecHandle.IsValid())
	{
		SpecHandle.Data->SetSetByCallerMagnitude(
			TAG_KC_Data_Cooking_Progress_Increase,
			Amount);
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

void AKCPotActor::HandleCookingProgressChanged(
	const FOnAttributeChangeData& ChangeData)
{
	if (!HasAuthority() || bRestoringCookingProgress ||
		ChangeData.NewValue >= ChangeData.OldValue)
	{
		return;
	}

	if (PotState == EKCPotStateType::Cooking)
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("[Pot] Hit during cooking: Pot=%s, Progress=%.2f -> %.2f"),
			*GetName(),
			ChangeData.OldValue,
			ChangeData.NewValue);
		return;
	}

	bRestoringCookingProgress = true;
	CookingProgressAttributes->SetCookingProgress(ChangeData.OldValue);
	bRestoringCookingProgress = false;
}

void AKCPotActor::MulticastCookingStarted_Implementation()
{
	OnCookingStarted();
}

void AKCPotActor::MulticastCookingCompleted_Implementation()
{
	OnCookingCompleted();
}

void AKCPotActor::MulticastCookingRuined_Implementation()
{
	OnCookingRuined();
}

void AKCPotActor::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AKCPotActor, PotState);
}
