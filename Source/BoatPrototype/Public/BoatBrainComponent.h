// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BoatBrainComponent.generated.h"

/**
 * Player boat steering behavior. Computes steer input toward the mouse's
 * world position when enabled (e.g. on RMB press), settling onto heading
 * proportionally rather than bang-bang.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class BOATPROTOTYPE_API UBoatBrainComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// The switch RMB press/release flips.
	UFUNCTION(BlueprintCallable, Category = "Boat|Brain")
	void SetMouseSteerEnabled(bool bEnabled) { bMouseSteerEnabled = bEnabled; }

	bool IsMouseSteerEnabled() const { return bMouseSteerEnabled; }

	// Steer input (-1..1) that turns boat toward the mouse's world position.
	// 0 if disabled or prerequisites are missing (no controller, ray fails, etc).
	float ComputeMouseSteerInput(const APawn* Boat) const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Brain")
	bool bMouseSteerEnabled = false;

	// Heading error (deg) at which steer input saturates to +/-1 (full helm).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Brain", meta = (ClampMin = "1.0"))
	float FullSteerHeadingError = 30.0f;
};
