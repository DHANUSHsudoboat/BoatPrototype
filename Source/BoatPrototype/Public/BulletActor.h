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
	float Speed = 2000.0f;

	// Called when the bullet collides with something.
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
};
