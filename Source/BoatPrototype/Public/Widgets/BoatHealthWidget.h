// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BoatHealthWidget.generated.h"

/**
 * Widget displaying the player boat's health as a progress bar.
 */
UCLASS()
class BOATPROTOTYPE_API UBoatHealthWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	// Update the health progress bar display.
	UFUNCTION(BlueprintCallable, Category = "Boat|UI")
	void UpdateHealthDisplay(float HealthPercent);

protected:
	// Binding to the UMG ProgressBar widget from the designer.
	UPROPERTY(meta = (BindWidget))
	class UProgressBar* HealthBar;

	// Cached reference to the player boat for polling current state.
	UPROPERTY()
	class ABoat* OwnerBoat;
};
