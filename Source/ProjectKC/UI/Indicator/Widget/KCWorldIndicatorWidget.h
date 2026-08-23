#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/UI/Common/Widget/KCUserWidget.h"
#include "KCWorldIndicatorWidget.generated.h"

UCLASS(Abstract, Blueprintable)
class PROJECTKC_API UKCWorldIndicatorWidget : public UKCUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetTargetActor(AActor* InTargetActor);

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	AActor* GetTargetActor() const { return TargetActor.Get(); }

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "KC|UI")
	void OnTargetActorChanged(AActor* InTargetActor);

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> TargetActor;
};
