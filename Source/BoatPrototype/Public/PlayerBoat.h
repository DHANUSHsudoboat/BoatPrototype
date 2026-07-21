// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Boat.h"
#include "PlayerBoat.generated.h"

/**
 * Player-controlled boat variant. Adds a compass widget HUD that rides on
 * the boat position but stays fixed in world rotation (not spinning with the
 * hull as it turns). Right-click steering via a brain component steers the
 * boat toward the mouse while RMB is held.
 */
UCLASS()
class BOATPROTOTYPE_API APlayerBoat : public ABoat
{
	GENERATED_BODY()

public:
	APlayerBoat();

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void SteerLeftPressed() override;
	virtual void SteerRightPressed() override;

	void OnAimPressed();
	void OnAimReleased();

protected:
	void OnRightMousePressed();
	void OnRightMouseReleased();
	void UpdateAimSideFromMouse();

	bool bIsAiming = false;

	// Compass HUD riding on the boat. Attached to root but rotation is absolute,
	// so the dial face doesn't spin with the hull as it turns. Hidden by default,
	// shown while RMB is held.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boat|Components")
	class UWidgetComponent* CompassWidget;

	// Steering brain that computes steer input toward mouse position when enabled.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boat|Components")
	class UBoatBrainComponent* Brain;
};
