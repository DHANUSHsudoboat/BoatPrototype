// Fill out your copyright notice in the Description page of Project Settings.

#include "BoatPrototype/Public/BulletActor.h"

#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "BoatPrototype/Public/Boat.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

ABulletActor::ABulletActor()
{
	// Tick drives the manual arc flight (broadside path). Straight-line legacy
	// shots via FireInDirection still use the projectile movement component.
	PrimaryActorTick.bCanEverTick = true;

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

	// TODO(Niagara): re-enable muzzle burst here.
	// SpawnMuzzleEffectBurst(Direction.GetSafeNormal());
}

void ABulletActor::LaunchArc(const FVector& Start, const FVector& Target, float TravelTime, float InArcHeight)
{
	ArcStart = Start;
	ArcTarget = Target;
	ArcTravelTime = FMath::Max(0.1f, TravelTime);
	ArcHeight = InArcHeight;
	ArcElapsed = 0.0f;
	bArcActive = true;

	// The arc is driven manually in Tick -- keep the projectile movement out of it.
	if (ProjectileMovement)
	{
		ProjectileMovement->StopMovementImmediately();
		ProjectileMovement->Deactivate();
	}

	SetActorLocation(Start);
	// Safety net in case the impact is somehow missed.
	SetLifeSpan(ArcTravelTime + 1.0f);

	// TODO(Niagara): spawn muzzle burst at Start here.
}

void ABulletActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bArcActive)
	{
		return;
	}

	ArcElapsed += DeltaTime;
	const float Alpha = FMath::Clamp(ArcElapsed / ArcTravelTime, 0.0f, 1.0f);

	// Manual interpolation: straight-line Lerp plus a sine arc for height. No
	// gravity/physics -- deterministic flight from spawn to the assigned target.
	FVector NewPos = FMath::Lerp(ArcStart, ArcTarget, Alpha);
	NewPos.Z += ArcHeight * FMath::Sin(PI * Alpha);

	// Swept move so a mid-flight overlap/hit still fires OnHit / OnOverlap.
	SetActorLocation(NewPos, true);

	// TODO(Niagara): spawn/advance the trail effect each tick here.

	if (Alpha >= 1.0f)
	{
		// Reached the target without striking anything en route -- resolve at the
		// landing point (splash) and consume the bullet.
		bArcActive = false;
		// TODO(Niagara): spawn splash effect at ArcTarget here.
		Destroy();
	}
}

void ABulletActor::SpawnMuzzleEffectBurst(const FVector& FireDirection)
{
	if (!MuzzleEffect)
	{
		return;
	}

	const FVector SpawnPoint = GetActorLocation();

	// All instances launch together from the same point; each gets a slightly
	// different direction within the cone, so the volley fans out wider the
	// further it travels -- like a spread of cannonballs, not one straight line.
	for (int32 i = 0; i < MuzzleEffectCount; ++i)
	{
		const FVector SpreadDirection = FMath::VRandCone(FireDirection, FMath::DegreesToRadians(MuzzleEffectSpreadAngle));
		UNiagaraComponent* SpawnedComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), MuzzleEffect, SpawnPoint, SpreadDirection.Rotation());

		// Auto-destroy only fires when the system finishes naturally -- a looping
		// emitter never does, so force it to clean up after a fixed lifetime instead.
		// The weak-lambda context must be the Niagara component itself, NOT the
		// bullet: the bullet is usually Destroy()'d almost immediately (on impact or
		// a short firing range), well before MuzzleEffectLifetime elapses. Binding to
		// the bullet meant the cleanup callback silently refused to fire once the
		// bullet was gone, orphaning the (independent, unattached) component forever.
		if (SpawnedComponent)
		{
			TWeakObjectPtr<UNiagaraComponent> WeakComponent = SpawnedComponent;
			FTimerHandle CleanupHandle;
			GetWorldTimerManager().SetTimer(CleanupHandle, FTimerDelegate::CreateWeakLambda(SpawnedComponent, [WeakComponent]()
			{
				if (UNiagaraComponent* Comp = WeakComponent.Get())
				{
					Comp->Deactivate();
					Comp->DestroyComponent();
				}
			}), MuzzleEffectLifetime, false);
		}
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
