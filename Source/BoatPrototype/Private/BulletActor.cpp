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

	// Bind collision events. Handle both cases: targets that BLOCK the bullet
	// (hit event) and targets that only OVERLAP it (overlap event) — otherwise a
	// boat set to overlap would let the bullet pass through with no damage.
	CollisionComponent->SetGenerateOverlapEvents(true);
	CollisionComponent->OnComponentHit.AddDynamic(this, &ABulletActor::OnHit);
	CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &ABulletActor::OnOverlap);

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
	HandleImpact(OtherActor);
}

void ABulletActor::OnOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep)
{
	HandleImpact(OtherActor);
}

void ABulletActor::HandleImpact(AActor* OtherActor)
{
	// Ignore self and the firing boat (the broadside spawns right beside its owner).
	if (OtherActor == this || OtherActor == GetOwner())
	{
		return;
	}

	// If we struck a boat, apply damage.
	if (ABoat* HitBoat = Cast<ABoat>(OtherActor))
	{
		HitBoat->ApplyBulletHit(this, Damage);
	}

	// Consume the bullet on any impact.
	Destroy();
}
