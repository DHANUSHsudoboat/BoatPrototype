// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Boat.generated.h"

/**
 * Generic drivable boat pawn (turn-in-place steering + drift).
 *
 * Forward drive stays kinematic: surge speed eases toward the current gear's
 * target. Steering is twin-screw / thruster style rather than rudder-gated:
 *
 *   full turn authority at ZERO speed (the boat pivots in place) -> going faster
 *   REDUCES the turn rate (fast = lazy heading change) -> a heavier hull turns
 *   slower at every speed. The helm sets a target yaw rate the boat eases toward.
 *
 * While making way the hull still drifts: a body-frame sway velocity slides the
 * boat outboard through the turn (curved arc). At zero speed the drift term falls
 * to zero, so the boat simply spins on the spot.
 *
 * State integrated each Tick -- surge (u, CurrentSpeed), sway (v, SwaySpeed),
 * yaw rate (r, YawRate) -- plus a helm angle smoothed toward the input.
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

	// Called by a bullet on impact. Destroys the boat once health reaches zero.
	UFUNCTION(BlueprintCallable, Category = "Boat|Combat")
	void ApplyBulletHit(AActor* HitInstigator, float DamageAmount);

	// Getter for current gear.
	UFUNCTION(BlueprintCallable, Category = "Boat|Gears")
	FORCEINLINE int32 GetCurrentGear() const { return CurrentGear; }

	// Getter for current speed.
	UFUNCTION(BlueprintCallable, Category = "Boat|Speed")
	FORCEINLINE float GetCurrentSpeed() const { return CurrentSpeed; }

	// Getter for health as 0..1 fraction.
	UFUNCTION(BlueprintCallable, Category = "Boat|Combat")
	FORCEINLINE float GetHealthPercent() const { return (MaxHealth > KINDA_SMALL_NUMBER) ? FMath::Clamp(CurrentHealth / MaxHealth, 0.0f, 1.0f) : 0.0f; }

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

	// Port (left) broadside aiming guide -- flat plane mesh, not a decal (decals
	// can't render on translucent water).
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boat|Components")
	class UStaticMeshComponent* PortGuideMesh;

	// Starboard (right) broadside aiming guide.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boat|Components")
	class UStaticMeshComponent* StarboardGuideMesh;

	// Health display widget mounted on the boat.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boat|Components")
	class UWidgetComponent* HealthWidget;
#pragma endregion

#pragma region Gears
	// Current gear (0 = idle, 3 = max). Clamped to [0, 3].
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boat|Gears", meta = (ClampMin = "0", ClampMax = "3"))
	int32 CurrentGear = 0;

	// Target speed for each gear as a percent (0-100) of MaximumSpeed. Gear 0 is always 0.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Gears", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float Gear1Percent = 40.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Gears", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float Gear2Percent = 70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Gears", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float Gear3Percent = 100.0f;
#pragma endregion

#pragma region Speed
	// Current forward speed (cm/s). Eased toward the gear target.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boat|Speed")
	float CurrentSpeed = 0.0f;

	// Hard cap on speed (cm/s). Independent of weight — every boat can reach this.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Speed")
	float MaximumSpeed = 200.0f;

	// Base rate the boat speeds up toward a higher gear (cm/s^2). Weight-scaled.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Speed")
	float AccelerationRate = 150.0f;

	// Base rate the boat slows down toward a lower gear (cm/s^2). Weight-scaled.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Speed")
	float DecelerationRate = 250.0f;
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
	// Yaw rate at full helm, ZERO speed, reference weight (deg/s). The turn-in-place rate.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Steering", meta = (ClampMin = "0.0"))
	float BaseTurnRate = 200.0f;

	// How fast the yaw rate eases toward its target (input lag / settle, higher = snappier).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Steering", meta = (ClampMin = "0.1"))
	float YawResponsiveness = 3.0f;

	// Full-helm angle (deg). The input eases toward +/- this; only smooths the helm feel.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Steering", meta = (ClampMin = "1.0", ClampMax = "180.0"))
	float MaxRudderAngle = 35.0f;

	// How fast the helm swings toward the input (deg/s). Not instant.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Steering", meta = (ClampMin = "1.0"))
	float RudderSpeed = 90.0f;

	// Lateral hull resistance: how hard the hull fights sideways motion. High = little drift.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Steering", meta = (ClampMin = "0.0"))
	float SwayDrag = 3.0f;

	// Turn-induced speed loss: extra surge drag from drift + helm while maneuvering.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Steering", meta = (ClampMin = "0.0"))
	float TurnDragFactor = 0.6f;
#pragma endregion

#pragma region Visuals
	// Max hull roll (deg) at full yaw rate. The hull heels into the turn — the
	// side it's turning toward dips. Purely cosmetic (rolls HullMesh, not the root).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Visuals", meta = (ClampMin = "0.0"))
	float MaxBankAngle = 8.0f;

	// How fast the hull eases toward its target roll (higher = snappier heel).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Visuals", meta = (ClampMin = "0.1"))
	float BankResponsiveness = 4.0f;
#pragma endregion

#pragma region Combat
	// Max broadside firing range (cm) — also the guide line's length. Override
	// GetFiringRange() per subclass for different ship classes.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Combat", meta = (ClampMin = "0.0"))
	float MaxFiringRange = 500.0f;

	// Guide line thickness (cm).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Combat", meta = (ClampMin = "1.0"))
	float BroadsideGuideWidth = 40.0f;

	// Guide tint, read by the guide material's "Color" vector parameter.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Combat")
	FLinearColor BroadsideGuideColor = FLinearColor::White;

	// Translucent unlit material asset (must expose a "Color" vector param and
	// fade Opacity along its length). Assign in the editor. NOT a decal material
	// -- deferred decals can't render on translucent water, so guides are a
	// flat plane mesh riding just above the water instead.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Combat")
	class UMaterialInterface* BroadsideGuideMaterial;

	UFUNCTION(BlueprintCallable, Category = "Boat|Combat")
	void SetPortGuideVisible(bool bVisible);

	UFUNCTION(BlueprintCallable, Category = "Boat|Combat")
	void SetStarboardGuideVisible(bool bVisible);

	// Override for different ship classes' firing range.
	virtual float GetFiringRange() const { return MaxFiringRange; }

	// Max health (HP).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Combat", meta = (ClampMin = "1.0"))
	float MaxHealth = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boat|Combat")
	float CurrentHealth = 100.0f;

	// Bullet class to spawn on fire. Defaults to ABulletActor; override with a
	// Blueprint subclass for custom art.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Combat")
	TSubclassOf<class ABulletActor> BulletClass;


protected:
	// Spawn a bullet off the given side (perpendicular to the hull), out to firing range.
	void FireBroadside(bool bStarboardSide);

	// Refresh guide mesh size/transform from current range/width. Called each Tick.
	virtual void UpdateBroadsideGuides();
#pragma endregion

#pragma region Camera
	// Min spring arm length (cm). Scroll wheel zooms between this and max.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Camera", meta = (ClampMin = "100.0"))
	float MinSpringArmLength = 500.0f;

	// Max spring arm length (cm).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Camera", meta = (ClampMin = "100.0"))
	float MaxSpringArmLength = 1500.0f;

	// Camera rotation at min spring arm length (deg). Typically more tilted down (-45).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Camera")
	float CameraRotationAtMinZoom = -45.0f;

	// Camera rotation at max spring arm length (deg). Typically more top-down (-90).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Camera")
	float CameraRotationAtMaxZoom = -75.0f;

	// Adjust spring arm length by this amount per scroll wheel tick (cm).
	UFUNCTION(BlueprintCallable, Category = "Boat|Camera")
	void AdjustCameraZoom(float DeltaZoom);
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

	// Target yaw rate this frame (deg/s): full at rest, reduced by speed, scaled by weight.
	virtual float GetTargetYawRate() const;

	// Sway (lateral) acceleration from the helm side kick (cm/s^2). Drives the drift arc.
	virtual float GetRudderSwayForce() const;

	// Play the gear-change camera shake. Override to add sound/FX.
	virtual void PlayGearShake();

	// Roll the visual hull to heel into the turn (cosmetic only). Called each Tick.
	virtual void UpdateVisualBank(float DeltaTime);
#pragma endregion

	// -1 = left, +1 = right, 0 = straight. Set from input (the helm order).
	float SteerInput = 0.0f;

	// Current rudder angle (deg). Swings toward SteerInput * MaxRudderAngle at RudderSpeed.
	float RudderAngle = 0.0f;

	// Current yaw rate (deg/s), built by the rudder moment and settled by yaw damping.
	float YawRate = 0.0f;

	// Current sway (lateral, +right) velocity (cm/s) -- the drift/sideslip of the hull.
	float SwaySpeed = 0.0f;

	// Hull's artist-set relative rotation, cached at BeginPlay. Bank rolls on top of it.
	FRotator HullBaseRotation = FRotator::ZeroRotator;

	// Current smoothed hull roll (deg) — eases toward the turn-driven target.
	float CurrentBank = 0.0f;

	void WakeSimulation();
	// Wake the sim on input; sleep it when fully parked and not turning.
	virtual bool ShouldSleep() const;
	
};
