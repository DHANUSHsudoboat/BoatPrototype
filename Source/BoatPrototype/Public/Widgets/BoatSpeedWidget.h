// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BoatSpeedWidget.generated.h"

/**
 * Widget displaying the player boat's gear and speed.
 */
UCLASS()
class BOATPROTOTYPE_API UBoatSpeedWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// Update the gear text display.
	UFUNCTION(BlueprintCallable, Category = "Boat|UI")
	void UpdateGearDisplay(int32 CurrentGear);

	// Update the speed text display.
	UFUNCTION(BlueprintCallable, Category = "Boat|UI")
	void UpdateSpeedDisplay(float InCurrentSpeed);

protected:
	// Bindings to the UMG TextBlock widgets from the designer.
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* GearText;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* CurrentSpeed;

	// Cached reference to the player boat for polling current state.
	UPROPERTY()
	class ABoat* OwnerBoat;
};
