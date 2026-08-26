#include "ProjectKC/Pot/KCPotActor.h"

#include "Components/BoxComponent.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
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
#include "ProjectKC/Messages/KCGameplayTags.h"
#include "ProjectKC/Messages/Struct/KCIngredientSubmittedStruct.h"
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
	CookingProgressAttributes->InitCookingProgress(0.0f);
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

	// 독립형 테스트에서는 기본 APlayerState를 사용하므로 팀 검사를 비활성화한다.
	// const APawn* InteractorPawn = Cast<APawn>(&Interactor);
	// const AKCLobbyPlayerState* PlayerState = InteractorPawn
	// 	? InteractorPawn->GetPlayerState<AKCLobbyPlayerState>()
	// 	: nullptr;
	// if (!PlayerState || PlayerState->GetTeamId() != AssignedTeamId)
	// {
	// 	return false;
	// }

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
	GetWorldTimerManager().SetTimer(
		CookingTimerHandle,
		this,
		&AKCPotActor::AdvanceCookingProgress,
		KCPot::ProgressTickInterval,
		true);
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
	}
}

void AKCPotActor::CompleteCooking()
{
	GetWorldTimerManager().ClearTimer(CookingTimerHandle);
	PotState = EKCPotStateType::Completed;

	FKCDishFinishedStruct Message;
	Message.TeamId = AssignedTeamId;
	Message.RecipeRowName = ActiveRecipeRowName;
	UGameplayMessageSubsystem::Get(this).BroadcastMessage(
		KCGameplayTags::Message_Dish_Finished,
		Message);

	MulticastCookingCompleted();
	ForceNetUpdate();
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
