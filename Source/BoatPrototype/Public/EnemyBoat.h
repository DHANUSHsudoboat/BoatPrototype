// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Boat.h"
#include "EnemyBoat.generated.h"

/**
 * AI-driven boat variant. Inherits the full steering + movement system from
 * ABoat and drives it each Tick: finds the player boat, circles to keep it on
 * the beam (broadside guns), holds firing range, and fires the facing side when
 * the player is lined up and in range.
 */
UCLASS()
class BOATPROTOTYPE_API AEnemyBoat : public ABoat
{
	GENERATED_BODY()

public:
	AEnemyBoat();

	virtual void Tick(float DeltaTime) override;

protected:
	// AI never parks its tick — it must keep steering/analysing.
	virtual bool ShouldSleep() const override { return false; }

	// Distance (cm) the enemy tries to hold from the player while circling.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|AI", meta = (ClampMin = "0.0"))
	float StandoffDistance = 500.0f;

	// Fire only when the player is within this angle (deg) of the broadside line.
	// Kept wide: while circling the beam is never perfectly abeam, so a tight
	// tolerance means the enemy almost never shoots.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|AI", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float FireAngleTolerance = 35.0f;

	// Seconds between broadside shots.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|AI", meta = (ClampMin = "0.1"))
	float FireInterval = 2.0f;

	// Heading error (deg) at which steer input saturates to +/-1 (full helm).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|AI", meta = (ClampMin = "1.0"))
	float FullSteerHeadingError = 30.0f;

	// Time left before the next shot is allowed.
	float FireCooldown = 0.0f;
};
