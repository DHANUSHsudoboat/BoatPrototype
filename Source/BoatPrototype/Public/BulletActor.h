// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BulletActor.generated.h"

/**
 * Cannonball projectile fired from a broadside. Travels in a straight line
 * at a constant speed, collides with boats (dealing damage) or vanishes once
 * it reaches its maximum range.
 */
UCLASS()
class BOATPROTOTYPE_API ABulletActor : public AActor
{
	GENERATED_BODY()

public:
	ABulletActor();

	virtual void Tick(float DeltaTime) override;

	// Launch the bullet along Direction; self-destruct once it has traveled Range.
	// Straight-line path via the projectile movement component (legacy/back-compat).
	void FireInDirection(const FVector& Direction, float Range);

	// Launch the bullet on a manual arc from Start to Target over TravelTime,
	// rising InArcHeight at the apex. No ProjectileMovement physics -- position is
	// interpolated each Tick (Lerp + sin arc). This is the broadside flight path.
	void LaunchArc(const FVector& Start, const FVector& Target, float TravelTime, float InArcHeight);

protected:
	// Sphere collision, root component, triggers OnHit when it hits something.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bullet")
	class USphereComponent* CollisionComponent;

	// Propels the bullet in a straight line with no gravity.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bullet")
	class UProjectileMovementComponent* ProjectileMovement;

	// Cannonball flight speed (cm/s).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet", meta = (ClampMin = "1.0"))
	float Speed = 500.0f;

	// Damage dealt to a boat on impact (HP).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet", meta = (ClampMin = "0.0"))
	float Damage = 50.0f;

#pragma region VFX
	// Niagara system played at the muzzle when fired -- a volley of these all
	// launch together from the same point, like several cannonballs firing at once.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet|VFX")
	class UNiagaraSystem* MuzzleEffect;

	// How many muzzle-effect instances to spawn per shot.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet|VFX", meta = (ClampMin = "1"))
	int32 MuzzleEffectCount = 5;

	// Half-angle (deg) of the cone each instance's direction is randomized within.
	// All instances spawn together at the same point; giving each a slightly
	// different direction is what makes the volley fan out wider as it travels.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet|VFX", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float MuzzleEffectSpreadAngle = 15.0f;

	// How long (seconds) each spawned instance is force-deactivated/destroyed after,
	// regardless of whether the Niagara system naturally finishes. Required for
	// looping effects -- SpawnSystemAtLocation's auto-destroy only fires on natural
	// completion, so a looping emitter would otherwise linger in the level forever.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet|VFX", meta = (ClampMin = "0.05"))
	float MuzzleEffectLifetime = 1.5f;

	// Spawns the whole volley at once (called from FireInDirection).
	void SpawnMuzzleEffectBurst(const FVector& FireDirection);
#pragma endregion

	// Called when the bullet blocks against something (hit event).
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	// Called when the bullet overlaps something (for targets that overlap rather than block).
	UFUNCTION()
	void OnOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep);

	// Shared impact resolution: damage a boat, then destroy the bullet. Ignores self/owner.
	void HandleImpact(AActor* OtherActor);

#pragma region Arc Movement
	// Whether the manual arc flight is active (set by LaunchArc). While true, Tick
	// drives the transform and ProjectileMovement is left inactive.
	bool bArcActive = false;

	FVector ArcStart = FVector::ZeroVector;
	FVector ArcTarget = FVector::ZeroVector;
	float ArcTravelTime = 1.0f;
	float ArcHeight = 150.0f;
	float ArcElapsed = 0.0f;
#pragma endregion
};
