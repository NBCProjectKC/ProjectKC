#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"
#include "KCInteractableInterface.generated.h"

class AActor;

UINTERFACE(BlueprintType)
class PROJECTKC_API UKCInteractableInterface : public UInterface
{
	GENERATED_BODY()
};

class PROJECTKC_API IKCInteractableInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void Interact(AActor* Interactor);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	FGameplayTag GetInteractionPromptTag(AActor* Interactor) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	FVector GetInteractionPromptWorldLocation(AActor* Interactor) const;
};
