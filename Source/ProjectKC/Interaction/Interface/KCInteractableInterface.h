#pragma once

#include "CoreMinimal.h"
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
	bool CanInteract(AActor* Interactor);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void Interact(AActor* Interactor);
};
