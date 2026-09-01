#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "KCMainMenuPlayerController.generated.h"

UCLASS()
class PROJECTKC_API AKCMainMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void InitializeMainMenuUI();
	void ClearMainMenuUI();
};
