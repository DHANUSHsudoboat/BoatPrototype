// Fill out your copyright notice in the Description page of Project Settings.

#include "BoatPrototype/Public/BulletActor.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "BoatPrototype/Public/Boat.h"
#include "Engine/StaticMesh.h"

ABulletActor::ABulletActor()
{
	PrimaryActorTick.bCanEverTick = false;

	// Sphere collision, root component.
	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	CollisionComponent->SetSphereRadius(20.0f);
	CollisionComponent->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	RootComponent = CollisionComponent;

	// Bind the collision event.
	CollisionComponent->OnComponentHit.AddDynamic(this, &ABulletActor::OnHit);

	// Visual mesh.
	BulletMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BulletMesh"));
	BulletMesh->SetupAttachment(RootComponent);
	BulletMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Load the built-in sphere mesh.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> BulletMeshFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (BulletMeshFinder.Succeeded())
	{
		BulletMesh->SetStaticMesh(BulletMeshFinder.Object);
	}

	// Projectile movement (no gravity, constant velocity).
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->SetUpdatedComponent(CollisionComponent);
	ProjectileMovement->ProjectileGravityScale = 0.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;

	// Fallback lifespan in case FireInDirection is never called.
	SetLifeSpan(10.0f);
}

void ABulletActor::FireInDirection(const FVector& Direction, float Range)
{
	if (ProjectileMovement)
	{
		ProjectileMovement->Velocity = Direction.GetSafeNormal() * Speed;
		// Vanish exactly at the firing range.
		SetLifeSpan(FMath::Max(0.1f, Range / Speed));
	}
}

void ABulletActor::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// Ignore self and owner (the firing boat).
	if (OtherActor == this || OtherActor == GetOwner())
	{
		return;
	}

	// If we hit a boat, apply damage.
	if (ABoat* HitBoat = Cast<ABoat>(OtherActor))
	{
		HitBoat->ApplyBulletHit(this, Damage);
	}

	// Destroy the bullet on any blocking hit.
	Destroy();
}
