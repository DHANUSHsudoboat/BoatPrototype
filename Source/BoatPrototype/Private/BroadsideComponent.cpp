// Fill out your copyright notice in the Description page of Project Settings.

#include "BoatPrototype/Public/BroadsideComponent.h"
#include "BoatPrototype/Public/Boat.h"
#include "BoatPrototype/Public/BulletActor.h"

#include "DrawDebugHelpers.h"
#include "TimerManager.h"

UBroadsideComponent::UBroadsideComponent()
{
	// No per-frame work: the volley is timer-driven, the preview is caller-driven.
	PrimaryComponentTick.bCanEverTick = false;
}

ABoat* UBroadsideComponent::GetOwnerBoat() const
{
	return Cast<ABoat>(GetOwner());
}

float UBroadsideComponent::GetOwnerFiringRange() const
{
	const ABoat* Boat = GetOwnerBoat();
	return Boat ? Boat->GetFiringRange() : 0.0f;
}

// ---------------------------------------------------------------------------
// Detection
// ---------------------------------------------------------------------------

bool UBroadsideComponent::IsInsideBroadsideArea(const FVector& WorldPos, bool& bOutStarboard) const
{
	const ABoat* Boat = GetOwnerBoat();
	if (!Boat)
	{
		return false;
	}

	// Project into the boat's local frame. The guide strips run outboard along
	// local Y (length = firing range), with thickness along local X. This mirrors
	// the transforms in ABoat::UpdateBroadsideGuides.
	const FVector Local = Boat->GetActorTransform().InverseTransformPosition(WorldPos);

	const float HalfBeam = Boat->GetBeamWidth() * 0.5f;
	const float Range = Boat->GetFiringRange();
	const float HalfThickness = Boat->GetBroadsideGuideThickness() * 0.5f;

	if (FMath::Abs(Local.X) > HalfThickness)
	{
		return false;
	}

	const float OutboardDist = FMath::Abs(Local.Y);
	if (OutboardDist < HalfBeam || OutboardDist > HalfBeam + Range)
	{
		return false;
	}

	bOutStarboard = Local.Y > 0.0f;
	return true;
}

// ---------------------------------------------------------------------------
// Spread + cadence
// ---------------------------------------------------------------------------

float UBroadsideComponent::ComputeArcHeight(float Dist) const
{
	return FMath::Clamp(Dist * ArcHeightFraction, MinArcHeight, MaxArcHeight);
}

float UBroadsideComponent::ComputeInterval(int32 Count) const
{
	// Small volleys fire at BaseInterval; each bullet beyond 3 shaves the interval
	// down toward the MinInterval floor. Clamped so it never dips below the floor.
	const float Raw = BaseInterval - FMath::Max(0, Count - 3) * IntervalFalloff;
	return FMath::Clamp(Raw, MinInterval, BaseInterval);
}

// ---------------------------------------------------------------------------
// Firing
// ---------------------------------------------------------------------------

void UBroadsideComponent::FireVolley(const FVector& LockedTarget, bool bStarboard)
{
	// Convenience: fire the whole volley at one fixed point (wraps it as a
	// provider that always returns the same value).
	FireVolleyLive([LockedTarget]() { return LockedTarget; }, bStarboard);
}

void UBroadsideComponent::FireVolleyLive(TFunction<FVector()> TargetProvider, bool bStarboard)
{
	if (!GetWorld() || BulletCount <= 0 || !TargetProvider)
	{
		// TEMP DIAGNOSTIC: pinpoints which precondition aborted the volley.
		UE_LOG(LogTemp, Warning, TEXT("FireVolleyLive: abort -- World=%s BulletCount=%d TargetProvider=%s"),
			GetWorld() ? TEXT("valid") : TEXT("NULL"), BulletCount, TargetProvider ? TEXT("set") : TEXT("NULL"));
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("FireVolleyLive: starting volley, BulletCount=%d"), BulletCount);

	// Store the provider -- it's called fresh per shot in SpawnLaneBullet, not
	// snapshotted here, so a moving aim/target keeps steering later bullets.
	VolleyTargetProvider = MoveTemp(TargetProvider);
	bVolleyStarboard = bStarboard;
	VolleyShotsRemaining = BulletCount;
	VolleyShotIndex = 0;

	// First bullet fires immediately, the rest on the clamped interval.
	SpawnNextBullet();

	if (VolleyShotsRemaining > 0)
	{
		const float Interval = ComputeInterval(BulletCount);
		GetWorld()->GetTimerManager().SetTimer(
			VolleyTimerHandle, this, &UBroadsideComponent::SpawnNextBullet, Interval, true);
	}
}

void UBroadsideComponent::SpawnNextBullet()
{
	if (VolleyShotsRemaining <= 0)
	{
		GetWorld()->GetTimerManager().ClearTimer(VolleyTimerHandle);
		return;
	}

	SpawnLaneBullet(VolleyShotIndex);
	++VolleyShotIndex;
	--VolleyShotsRemaining;

	if (VolleyShotsRemaining <= 0)
	{
		GetWorld()->GetTimerManager().ClearTimer(VolleyTimerHandle);
	}
}

FVector UBroadsideComponent::ClampToBroadsideArea(const FVector& WorldPos, bool bStarboard) const
{
	const ABoat* Boat = GetOwnerBoat();
	if (!Boat)
	{
		return WorldPos;
	}

	const float HalfBeam = Boat->GetBeamWidth() * 0.5f;
	const float Range = Boat->GetFiringRange();
	const float HalfThickness = Boat->GetBroadsideGuideThickness() * 0.5f;

	// Work in the boat's local frame, then clamp to the guide-strip rectangle:
	// thickness along X, outboard span [HalfBeam, HalfBeam+Range] along Y on the
	// chosen side. Mirrors IsInsideBroadsideArea / UpdateBroadsideGuides.
	FVector Local = Boat->GetActorTransform().InverseTransformPosition(WorldPos);
	Local.X = FMath::Clamp(Local.X, -HalfThickness, HalfThickness);
	const float Sign = bStarboard ? 1.0f : -1.0f;
	const float Outboard = FMath::Clamp(FMath::Abs(Local.Y), HalfBeam, HalfBeam + Range);
	Local.Y = Sign * Outboard;

	return Boat->GetActorTransform().TransformPosition(Local);
}

FVector UBroadsideComponent::GetSideSpawnEdge(bool bStarboard) const
{
	const ABoat* Boat = GetOwnerBoat();
	if (!Boat)
	{
		return FVector::ZeroVector;
	}

	// Start the trajectory preview from the real Center-lane muzzle.
	return Boat->GetLaneMuzzleLocation(bStarboard, 1);
}

void UBroadsideComponent::SpawnLaneBullet(int32 ShotIndex)
{
	ABoat* Boat = GetOwnerBoat();
	if (!Boat || !GetWorld())
	{
		return;
	}

	const TSubclassOf<ABulletActor> BulletClass = Boat->GetBulletClass();
	if (!BulletClass || !VolleyTargetProvider)
	{
		// TEMP DIAGNOSTIC: pinpoints a null BulletClass / missing provider as the
		// reason a shot silently produces no bullet.
		UE_LOG(LogTemp, Warning, TEXT("SpawnLaneBullet: abort on %s -- BulletClass=%s VolleyTargetProvider=%s"),
			*Boat->GetName(), BulletClass ? TEXT("set") : TEXT("NULL"), VolleyTargetProvider ? TEXT("set") : TEXT("NULL"));
		return;
	}

	// Resolve the lane (only ever three), regardless of volley size.
	const int32 LaneIdx = ShotIndex % 3;

	// Evaluated fresh for THIS shot -- not a snapshot from when the volley
	// started -- so a live-aiming caller can steer later bullets differently.
	const FVector Target = VolleyTargetProvider();

	// Spawn point: the designer-placed muzzle arrow for this side + lane (falls
	// back to a procedural point off the beam if the arrow is unset).
	const FVector SpawnLocation = Boat->GetLaneMuzzleLocation(bVolleyStarboard, LaneIdx);

	const float Dist = FVector::Dist(SpawnLocation, Target);
	const float TravelTime = FMath::Max(0.1f, Dist / FMath::Max(1.0f, BulletSpeed));
	const float ShotArcHeight = ComputeArcHeight(Dist);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Boat;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FRotator SpawnRotation = (Target - SpawnLocation).Rotation();
	if (ABulletActor* Bullet = GetWorld()->SpawnActor<ABulletActor>(BulletClass, SpawnLocation, SpawnRotation, SpawnParams))
	{
		Bullet->LaunchArc(SpawnLocation, Target, TravelTime, ShotArcHeight);
		// TEMP DIAGNOSTIC: confirms a bullet actor was actually spawned.
		UE_LOG(LogTemp, Warning, TEXT("SpawnLaneBullet: spawned bullet for %s at %s -> %s"),
			*Boat->GetName(), *SpawnLocation.ToString(), *Target.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnLaneBullet: SpawnActor FAILED for %s"), *Boat->GetName());
	}

	// TODO(Niagara): spawn muzzle flash + smoke at SpawnLocation here.
}

// ---------------------------------------------------------------------------
// Trajectory preview
// ---------------------------------------------------------------------------

void UBroadsideComponent::DrawTrajectoryPreview(const FVector& From, const FVector& To) const
{
	// TEMP DIAGNOSTIC: isolate why GetWorld() fails here -- registered? owner
	// resolved? owner's own GetWorld() valid even when this->GetWorld() isn't?
	const AActor* Owner = GetOwner();
	UE_LOG(LogTemp, Warning,
		TEXT("DrawTrajectoryPreview: IsRegistered=%d Owner=%s OwnerWorld=%s ThisWorld=%s Outer=%s"),
		IsRegistered() ? 1 : 0,
		Owner ? *Owner->GetName() : TEXT("NULL"),
		(Owner && Owner->GetWorld()) ? TEXT("valid") : TEXT("NULL"),
		GetWorld() ? TEXT("valid") : TEXT("NULL"),
		GetOuter() ? *GetOuter()->GetName() : TEXT("NULL"));

	if (!GetWorld())
	{
		return;
	}

	// Sample the same Lerp + sin arc the bullet flies (same distance-scaled height)
	// and stamp dots along it, so the preview always matches what actually fires.
	const float PreviewArcHeight = ComputeArcHeight(FVector::Dist(From, To));

	constexpr int32 NumDots = 20;
	for (int32 i = 0; i <= NumDots; ++i)
	{
		const float Alpha = static_cast<float>(i) / NumDots;
		FVector Point = FMath::Lerp(From, To, Alpha);
		Point.Z += PreviewArcHeight * FMath::Sin(PI * Alpha);
		DrawDebugPoint(GetWorld(), Point, 6.0f, FColor::Yellow, false, -1.0f, 0);
	}

	// TODO(Niagara): replace this dotted debug arc with a spline/Niagara ribbon.
}
