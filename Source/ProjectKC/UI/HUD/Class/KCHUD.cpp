// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/Class/KCHUD.h"

#include "Blueprint/UserWidget.h"
#include "UI/HUD/Widget/KCHUDWidget.h"


void AKCHUD::CreateMainHUD()
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC)
	{
		return;
	}
	
	if (HUDClass)
	{
		if (UKCHUDWidget* CreatedMainHUDWidget = Cast<UKCHUDWidget>(CreateWidget(PC, HUDClass)))
		{
			HUDWidget = CreatedMainHUDWidget;
			HUDWidget->AddToViewport();
		}
	}
}

void AKCHUD::BeginPlay()
{
	Super::BeginPlay();
	
	CreateMainHUD();	
}
