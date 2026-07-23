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

	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void SteerLeftPressed() override;
	virtual void SteerRightPressed() override;

	void OnAimPressed();
	void OnAimReleased();
	void OnScrollUp();
	void OnScrollDown();

protected:
	// Keep ticking while the fire cooldown is still counting down (so a parked boat
	// can recharge its guns), otherwise defer to the base sleep rule.
	virtual bool ShouldSleep() const override;

	void OnRightMousePressed();
	void OnRightMouseReleased();

	// Refresh the aim each frame while LMB is held: always free-aim from the mouse
	// (no auto-lock -- that's an enemy-only behaviour). Draws the dotted trajectory
	// preview and updates the live target every frame.
	void UpdateAiming();

	// Deproject the mouse onto the boat's Z-plane. False if it can't be resolved.
	bool GetMouseWorldPointOnBoatPlane(FVector& OutWorld) const;

	bool bIsAiming = false;
	bool bAimingStarboard = false;

	// Seconds between broadside shots. Firing is blocked until the cooldown elapses.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Combat", meta = (ClampMin = "0.0"))
	float FireInterval = 1.5f;

	// Time left before the next shot is allowed.
	float FireCooldown = 0.0f;

	// Compass HUD riding on the boat. Attached to root but rotation is absolute,
	// so the dial face doesn't spin with the hull as it turns. Hidden by default,
	// shown while RMB is held.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boat|Components")
	class UWidgetComponent* CompassWidget;

	// Steering brain that computes steer input toward mouse position when enabled.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boat|Components")
	class UBoatBrainComponent* Brain;
};
