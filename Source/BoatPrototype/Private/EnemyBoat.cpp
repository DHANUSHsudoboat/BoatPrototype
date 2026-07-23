// Fill out your copyright notice in the Description page of Project Settings.


#include "BoatPrototype/Public/EnemyBoat.h"

#include "Kismet/GameplayStatics.h"

AEnemyBoat::AEnemyBoat()
{
	// AI drives the inherited sim from Tick.
	PrimaryActorTick.bCanEverTick = true;
	bShowDebugHUD = false; // The player's HUD reflects the player boat, not this one.
}

void AEnemyBoat::Tick(float DeltaTime)
{
	// --- Acquire target: the player boat. ---
	ABoat* Player = Cast<ABoat>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!Player)
	{
		SteerInput = 0.0f;
		CurrentGear = 0;
		Super::Tick(DeltaTime);
		return;
	}

	// --- Analyse the player (flatten to the water plane). ---
	FVector ToPlayer = Player->GetActorLocation() - GetActorLocation();
	ToPlayer.Z = 0.0f;
	const float Dist = ToPlayer.Size2D();
	const FVector ToPlayerDir = ToPlayer.GetSafeNormal2D();

	if (!ToPlayerDir.IsNearlyZero())
	{
		// --- Pick the broadside already closer to the player. ---
		const bool bStarboard = FVector::DotProduct(ToPlayerDir, GetActorRightVector()) > 0.0f;

		// Tangent heading: face so the chosen side's right-vector points at the player.
		const FVector TangentDir = ToPlayerDir.RotateAngleAxis(bStarboard ? -90.0f : 90.0f, FVector::UpVector);

		// Blend in a radial correction so we spiral toward the standoff ring:
		// too far -> lean toward the player, too close -> lean away, on the ring -> pure circle.
		const float RadialErr = (StandoffDistance > KINDA_SMALL_NUMBER)
			? FMath::Clamp((Dist - StandoffDistance) / StandoffDistance, -1.0f, 1.0f)
			: 0.0f;
		const FVector DesiredDir = (TangentDir + RadialErr * ToPlayerDir).GetSafeNormal2D();

		if (!DesiredDir.IsNearlyZero())
		{
			const float DesiredYaw = FMath::RadiansToDegrees(FMath::Atan2(DesiredDir.Y, DesiredDir.X));
			const float HeadingErr = FMath::FindDeltaAngleDegrees(GetActorRotation().Yaw, DesiredYaw);
			SteerInput = FMath::Clamp(HeadingErr / FullSteerHeadingError, -1.0f, 1.0f);
		}

		// --- Throttle by distance band: close fast when far, keep way to circle. ---
		if (Dist > StandoffDistance * 1.5f)
		{
			CurrentGear = 3;
		}
		else if (Dist > StandoffDistance)
		{
			CurrentGear = 2;
		}
		else
		{
			CurrentGear = 1;
		}

		// --- Fire whichever broadside is lined up when the player is abeam and in range. ---
		FireCooldown -= DeltaTime;
		// |dot(right, toPlayer)| ~ how abeam the player is (1 = perfectly on the beam,
		// either side). Sign picks port vs starboard. Independent of the steering side.
		const float BeamDot = FVector::DotProduct(GetActorRightVector(), ToPlayerDir);
		const bool bFireStarboard = BeamDot > 0.0f;
		const float Aligned = FMath::Abs(BeamDot);
		if (Dist <= GetFiringRange()
			&& Aligned > FMath::Cos(FMath::DegreesToRadians(FireAngleTolerance))
			&& FireCooldown <= 0.0f)
		{
			// Enemy already has its target (the player) -- fire an arced volley that
			// re-reads the player's position at EACH bullet's own spawn moment, so a
			// large volley keeps tracking the player instead of aiming at where they
			// were when the burst started. Weak pointer: the volley can outlive this
			// frame by a couple seconds, and the player pawn could be gone by then.
			TWeakObjectPtr<ABoat> WeakPlayer(Player);
			FireVolleyLive([WeakPlayer]() -> FVector
			{
				const ABoat* LivePlayer = WeakPlayer.Get();
				return LivePlayer ? LivePlayer->GetActorLocation() : FVector::ZeroVector;
			}, bFireStarboard);
			FireCooldown = FireInterval;
		}

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow,
				FString::Printf(TEXT("%s: Dist %.0f / Range %.0f  beam %.0f deg  cd %.1f"),
					*GetName(), Dist, GetFiringRange(),
					FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Aligned, 0.f, 1.f))), FireCooldown));
		}
	}

	Super::Tick(DeltaTime);
}
