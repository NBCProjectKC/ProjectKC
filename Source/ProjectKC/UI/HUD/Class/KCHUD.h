// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "KCHUD.generated.h"

class UKCHUDWidget;
/**
 * 
 */
UCLASS()
class PROJECTKC_API AKCHUD : public AHUD
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "FT|UI")
	void CreateMainHUD();
	
	virtual void BeginPlay() override;
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KC|HUD")
	TSubclassOf<UKCHUDWidget> HUDClass;
	
private:
	UPROPERTY()
	TObjectPtr<UKCHUDWidget> HUDWidget = nullptr;
};
