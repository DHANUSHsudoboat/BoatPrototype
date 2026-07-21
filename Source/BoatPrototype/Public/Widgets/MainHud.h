// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/HUD.h"
#include "MainHud.generated.h"

/**
 * Main HUD for the game. Displays the boat speed/gear widget.
 */
UCLASS()
class BOATPROTOTYPE_API UMainHud : public UUserWidget
{
	GENERATED_BODY()

protected:

	// Instance of the boat speed widget currently displayed.
	UPROPERTY(meta = (BindWidget))
	class UBoatSpeedWidget* BoatSpeedWidget;
};
