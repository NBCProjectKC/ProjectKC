#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProjectKC/UI/World/Player/Struct/KCPlayerDisplayInfoStruct.h"
#include "KCPlayerOverHeadComponent.generated.h"

class UWidgetComponent;
class UKCPlayerOverHeadViewModel;
class UKCPlayerOverHeadWidget;

UCLASS(ClassGroup = (KC), meta = (BlueprintSpawnableComponent))
class PROJECTKC_API UKCPlayerOverHeadComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UKCPlayerOverHeadComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetPlayerDisplayInfo(const FKCPlayerDisplayInfoStruct& NewDisplayInfo);

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void ClearPlayerDisplayInfo();

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	UKCPlayerOverHeadViewModel* GetViewModel() const { return PlayerOverHeadViewModel; }

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	UWidgetComponent* GetWidgetComponent() const { return PlayerOverHeadWidgetComponent; }

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|UI")
	TSubclassOf<UKCPlayerOverHeadWidget> PlayerOverHeadWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|UI")
	FVector WidgetRelativeLocation = FVector(0.0f, 0.0f, 140.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|UI")
	FVector2D WidgetDrawSize = FVector2D(180.0f, 40.0f);

private:
	void EnsureViewModel();
	void EnsureWidgetComponent();
	void ApplyDisplayInfoToWidget();
	UKCPlayerOverHeadWidget* GetPlayerOverHeadWidget() const;

	UPROPERTY(Transient)
	TObjectPtr<UKCPlayerOverHeadViewModel> PlayerOverHeadViewModel;

	UPROPERTY(Transient)
	TObjectPtr<UWidgetComponent> PlayerOverHeadWidgetComponent;
};
