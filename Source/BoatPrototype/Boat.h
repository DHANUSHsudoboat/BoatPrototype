// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Boat.generated.h"

/**
 * Generic drivable boat pawn (kinematic movement + force-style steering).
 *
 * Forward motion is kinematic: speed eases toward the current gear's target and
 * the actor is translated along its facing. Steering is modelled as a turning
 * FORCE rather than a fixed radius: steer input applies an angular acceleration
 * that scales with the boat's current speed, and water drag decays the yaw rate
 * back to zero when you let go. Slow boat = weak turn, fast boat = strong turn.
 *
 * Designed as a base class: the sim steps and derived values are small virtual
 * methods subclasses override without touching the loop.
 */
UCLASS()
class BOATPROTOTYPE_API ABoat : public APawn
{
	GENERATED_BODY()

public:
	ABoat();

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	virtual void BeginPlay() override;

#pragma region Components
	// Pawn root. Movement + steering act on this; the camera and arrow ride it.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boat|Components")
	class USceneComponent* RootScene;

	// Boat art. Rotate freely to align the model; does not affect movement.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boat|Components")
	class UStaticMeshComponent* HullMesh;

	// Shows the pawn's true forward (+X). On the root, so hull rotation can't skew it.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boat|Components")
	class UArrowComponent* ForwardArrow;

	// Boom holding the camera behind/above the boat.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boat|Components")
	class USpringArmComponent* SpringArm;

	// Follow camera on the spring arm.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boat|Components")
	class UCameraComponent* FollowCamera;
#pragma endregion

#pragma region Gears
	// Current gear (0 = idle, 3 = max). Clamped to [0, 3].
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boat|Gears", meta = (ClampMin = "0", ClampMax = "3"))
	int32 CurrentGear = 0;

	// Target speed for each gear (cm/s). Gear 0 is always 0.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Gears")
	float Gear1Speed = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Gears")
	float Gear2Speed = 275.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Gears")
	float Gear3Speed = 400.0f;
#pragma endregion

#pragma region Speed
	// Current forward speed (cm/s). Eased toward the gear target.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boat|Speed")
	float CurrentSpeed = 0.0f;

	// Hard cap on speed (cm/s). Independent of weight — every boat can reach this.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Speed")
	float MaximumSpeed = 400.0f;

	// Base rate the boat speeds up toward a higher gear (cm/s^2). Weight-scaled.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Speed")
	float AccelerationRate = 300.0f;

	// Base rate the boat slows down toward a lower gear (cm/s^2). Weight-scaled.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Speed")
	float DecelerationRate = 500.0f;
#pragma endregion

#pragma region Physical
	// Boat mass (kg). Heavier = slower to change speed AND slower to start/stop turning.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Physical", meta = (ClampMin = "1.0"))
	float Weight = 1000.0f;

	// Hull beam / width (cm). Wider than reference = sluggish, weaker turns.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Physical", meta = (ClampMin = "1.0"))
	float Width = 300.0f;

	// Baselines the tuned rates assume. Weight/Width at these values = no change.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Physical", meta = (ClampMin = "1.0"))
	float ReferenceWeight = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Physical", meta = (ClampMin = "1.0"))
	float ReferenceWidth = 300.0f;
#pragma endregion

#pragma region Steering
	// Rudder strength: angular acceleration (deg/s^2) applied by full steer at full speed.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Steering")
	float TurnForce = 120.0f;

	// Water drag on yaw: how quickly the turn rate decays back to zero when you release.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Steering", meta = (ClampMin = "0.0"))
	float TurnDamping = 2.5f;

	// Cap on how fast the boat can rotate (deg/s).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Steering", meta = (ClampMin = "0.0"))
	float MaxYawRate = 45.0f;
#pragma endregion

#pragma region Feedback
	// Camera shake played on every gear change. Assign a CameraShake asset.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Feedback")
	TSubclassOf<class UCameraShakeBase> GearShake;

	// Scales the shake strength.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Feedback")
	float GearShakeScale = 1.0f;

	// Draw the gear / speed readout on screen.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Feedback")
	bool bShowDebugHUD = true;
#pragma endregion

#pragma region Simulation
	// Ease forward speed toward the gear target. Override to reshape accel.
	virtual void UpdateSpeed(float DeltaTime);

	// Integrate the turning force into a yaw rate and rotate. Override for feel.
	virtual void UpdateSteering(float DeltaTime);

	// Translate the boat along its facing direction this frame.
	virtual void UpdateMovement(float DeltaTime);
#pragma endregion

#pragma region Input Handlers
	virtual void ShiftUpGear();
	virtual void ShiftDownGear();

	virtual void SteerLeftPressed();
	virtual void SteerRightPressed();
	virtual void SteerReleased();
#pragma endregion

#pragma region Derived Values
	// Target speed for the current gear (clamped to MaximumSpeed). Override per boat type.
	virtual float GetTargetSpeed() const;

	// Weight-scaled accel / decel (cm/s^2). Heavier boat = smaller values.
	virtual float GetAccelerationRate() const;
	virtual float GetDecelerationRate() const;

	// Turning force this frame as angular acceleration (deg/s^2): steer * speed * width * weight.
	virtual float GetTurnAcceleration() const;

	// Play the gear-change camera shake. Override to add sound/FX.
	virtual void PlayGearShake();
#pragma endregion

	// -1 = left, +1 = right, 0 = straight. Set from input.
	float SteerInput = 0.0f;

	// Current yaw rate (deg/s), built up by the turning force and decayed by drag.
	float YawRate = 0.0f;

private:
	// Wake the sim on input; sleep it when fully parked and not turning.
	void WakeSimulation();
	bool ShouldSleep() const;
};
