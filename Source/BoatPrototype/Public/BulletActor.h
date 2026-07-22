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

	// Launch the bullet along Direction; self-destruct once it has traveled Range.
	void FireInDirection(const FVector& Direction, float Range);

protected:
	// Sphere collision, root component, triggers OnHit when it hits something.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bullet")
	class USphereComponent* CollisionComponent;

	// Visual representation of the bullet.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bullet")
	class UStaticMeshComponent* BulletMesh;

	// Propels the bullet in a straight line with no gravity.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bullet")
	class UProjectileMovementComponent* ProjectileMovement;

	// Cannonball flight speed (cm/s).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet", meta = (ClampMin = "1.0"))
	float Speed = 500.0f;

	// Damage dealt to a boat on impact (HP).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet", meta = (ClampMin = "0.0"))
	float Damage = 50.0f;

	// Called when the bullet blocks against something (hit event).
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	// Called when the bullet overlaps something (for targets that overlap rather than block).
	UFUNCTION()
	void OnOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep);

	// Shared impact resolution: damage a boat, then destroy the bullet. Ignores self/owner.
	void HandleImpact(AActor* OtherActor);
};
