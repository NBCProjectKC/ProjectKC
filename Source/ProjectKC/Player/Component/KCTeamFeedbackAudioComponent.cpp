#include "Player/Component/KCTeamFeedbackAudioComponent.h"

#include "Core/Audio/KCBGMSubsystem.h"
#include "Core/Audio/KCSoundSettings.h"
#include "GameSystem/Recipe/KCDishRuinedStruct.h"
#include "Messages/KCGameplayTags.h"
#include "Player/KCPlayerController.h"
#include "Player/KCPlayerState.h"

UKCTeamFeedbackAudioComponent::UKCTeamFeedbackAudioComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UKCTeamFeedbackAudioComponent::BeginPlay()
{
	Super::BeginPlay();

	const AKCPlayerController* PlayerController = Cast<AKCPlayerController>(GetOwner());
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return;
	}

	DishRuinedListenerHandle = UGameplayMessageSubsystem::Get(this).RegisterListener<FKCDishRuinedStruct>(
		KCGameplayTags::Message_Dish_Ruined,
		this,
		&ThisClass::HandleDishRuined);
}

void UKCTeamFeedbackAudioComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DishRuinedListenerHandle.Unregister();

	Super::EndPlay(EndPlayReason);
}

void UKCTeamFeedbackAudioComponent::HandleDishRuined(
	FGameplayTag Channel,
	const FKCDishRuinedStruct& Message)
{
	if (Message.TeamId != GetOwnerTeamId())
	{
		return;
	}

	PlayRecipeFailedSound();
}

void UKCTeamFeedbackAudioComponent::PlayRecipeFailedSound() const
{
	if (!bPlayRecipeFailedSound)
	{
		return;
	}

	const UKCSoundSettings* SoundSettings = GetDefault<UKCSoundSettings>();
	USoundBase* RecipeFailedSound = SoundSettings
		? SoundSettings->RecipeFailedSound.LoadSynchronous()
		: nullptr;
	if (!RecipeFailedSound)
	{
		return;
	}

	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	UKCBGMSubsystem* AudioSubsystem = GameInstance
		? GameInstance->GetSubsystem<UKCBGMSubsystem>()
		: nullptr;
	if (!AudioSubsystem)
	{
		return;
	}

	AudioSubsystem->PlayLocalSFX2D(RecipeFailedSound);
}

int32 UKCTeamFeedbackAudioComponent::GetOwnerTeamId() const
{
	const AKCPlayerController* PlayerController = Cast<AKCPlayerController>(GetOwner());
	const AKCPlayerState* PlayerState = PlayerController
		? PlayerController->GetPlayerState<AKCPlayerState>()
		: nullptr;
	return PlayerState ? PlayerState->GetTeamId() : INDEX_NONE;
}