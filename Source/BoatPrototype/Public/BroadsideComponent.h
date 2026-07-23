// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BroadsideComponent.generated.h"

/**
 * Which of the three fixed firing lanes a bullet belongs to. The ship art has
 * many cannons, but gameplay always resolves to exactly these three lanes -- no
 * matter how large the volley, we only ever spread across Left / Center / Right.
 */
UENUM(BlueprintType)
enum class EBroadsideLane : uint8
{
	Left,
	Center,
	Right
};

/**
 * Broadside volley engine, owned by ABoat and shared by player + enemy.
 *
 * Single responsibility hub: broadside-area detection, target locking helper,
 * per-lane spread generation, spawn cadence (clamped interval), sequential
 * bullet spawning, and the dotted trajectory preview. Kept off the Boat actor
 * so firing behaviour stays modular and easy to extend later (reload time,
 * cannon types, accuracy modifiers, crits, ship classes).
 *
 * Aiming/target selection is NOT here -- that is caller-driven (the player
 * deprojects the mouse / auto-locks; the enemy already has the player as its
 * target). Callers hand FireVolley() a single locked world point + side.
 */
UCLASS(ClassGroup = (Boat), meta = (BlueprintSpawnableComponent))
class BOATPROTOTYPE_API UBroadsideComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBroadsideComponent();

	// --- Detection --------------------------------------------------------

	// True if WorldPos lies inside either broadside guide strip. bOutStarboard
	// reports which side (+Y = starboard). Mirrors the guide-strip geometry the
	// boat draws in UpdateBroadsideGuides.
	bool IsInsideBroadsideArea(const FVector& WorldPos, bool& bOutStarboard) const;

	// --- Firing -----------------------------------------------------------

	// Fire a full volley off the given side. TargetProvider is called fresh
	// inside SpawnLaneBullet for EACH bullet -- not once for the whole volley --
	// so if the caller's aim keeps changing across a large volley's spawn window
	// (spread over BulletCount * ComputeInterval seconds), the first and last
	// bullet can end up aimed at genuinely different points. Lane/muzzle
	// selection (Left/Center/Right, ShotIndex % 3) is unaffected.
	void FireVolleyLive(TFunction<FVector()> TargetProvider, bool bStarboard);

	// Convenience wrapper: fire the whole volley at one fixed point.
	UFUNCTION(BlueprintCallable, Category = "Boat|Broadside")
	void FireVolley(const FVector& LockedTarget, bool bStarboard);

	// --- Preview ----------------------------------------------------------

	// Draw the dotted debug arc from a spawn edge to the aim point. Called every
	// frame while aiming (fixed target in lock mode, live mouse point in free aim).
	void DrawTrajectoryPreview(const FVector& From, const FVector& To) const;

	// Spawn edge on the firing side, used as the trajectory preview origin.
	FVector GetSideSpawnEdge(bool bStarboard) const;

	// Clamp a world point into the broadside guide strip on the given side, so a
	// free-aim point can never leave the firing area (correct side, within the
	// strip thickness, between hull edge and firing range).
	FVector ClampToBroadsideArea(const FVector& WorldPos, bool bStarboard) const;

	// Firing range passthrough (reads the owning boat).
	float GetOwnerFiringRange() const;

protected:
	// --- Tunables ---------------------------------------------------------

	// Reserved for a future accuracy modifier (e.g. crew skill, weather). NOT
	// currently applied -- all three lanes converge on the exact locked/aimed
	// point (see BuildSpreadTargets), fraction of firing range if reintroduced.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Broadside", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SideSpreadFraction = 0.15f;

	// Reserved for a future accuracy modifier. NOT currently applied.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Broadside", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CenterSpreadFraction = 0.04f;

	// Fore/aft spacing between the three lane spawn points along the hull (cm).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Broadside", meta = (ClampMin = "0.0"))
	float LaneSpacing = 150.0f;

	// Number of bullets in a volley. <=3 fires at BaseInterval; >3 tightens the
	// interval toward MinInterval but still only uses the three lanes.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Broadside", meta = (ClampMin = "1"))
	int32 BulletCount = 3;

	// Interval between shots for a small volley (s).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Broadside", meta = (ClampMin = "0.05"))
	float BaseInterval = 0.5f;

	// Hard floor on the interval (s). Large volleys never fire faster than this.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Broadside", meta = (ClampMin = "0.05"))
	float MinInterval = 0.2f;

	// Seconds shaved off BaseInterval per extra bullet beyond 3, before clamping.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Broadside", meta = (ClampMin = "0.0"))
	float IntervalFalloff = 0.033f;

	// Cannonball flight speed used to derive per-bullet travel time (cm/s).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Broadside", meta = (ClampMin = "1.0"))
	float BulletSpeed = 500.0f;

	// Arc peak height as a FRACTION of the shot's travel distance, not a flat
	// value -- a fixed height squeezed into a short shot's tiny TravelTime spikes
	// its vertical speed (looks like it "snaps"), while the same height stretched
	// over a long shot's TravelTime looks lazy. Scaling with distance keeps the
	// curve's visual pace consistent across ranges. Clamped by Min/MaxArcHeight.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Broadside", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ArcHeightFraction = 0.25f;

	// Floor on the per-shot arc height (cm) -- keeps very short shots from going
	// nearly flat.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Broadside", meta = (ClampMin = "0.0"))
	float MinArcHeight = 50.0f;

	// Ceiling on the per-shot arc height (cm) -- keeps very long shots from
	// lobbing absurdly high.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Broadside", meta = (ClampMin = "0.0"))
	float MaxArcHeight = 400.0f;

private:
	// Per-shot arc height for a given travel distance: Dist * ArcHeightFraction,
	// clamped to [MinArcHeight, MaxArcHeight]. Used by both the actual spawn and
	// the trajectory preview so they always match.
	float ComputeArcHeight(float Dist) const;

	// Interval for a volley of the given size, clamped to [MinInterval, BaseInterval].
	float ComputeInterval(int32 Count) const;

	// Timer callback: spawn one bullet for the current shot, advance the volley.
	void SpawnNextBullet();

	// Spawn one bullet for the given shot index (lane = index % 3). Evaluates
	// VolleyTargetProvider fresh, right here, for this shot's target.
	void SpawnLaneBullet(int32 ShotIndex);

	// Owning boat, resolved from GetOwner(). Null-safe helpers below.
	class ABoat* GetOwnerBoat() const;

	// --- Per-volley state (valid only while a volley is spawning) ---------
	// Re-evaluated at each shot's own spawn moment -- see FireVolleyLive.
	TFunction<FVector()> VolleyTargetProvider;
	bool bVolleyStarboard = false;
	int32 VolleyShotsRemaining = 0;
	int32 VolleyShotIndex = 0;
	FTimerHandle VolleyTimerHandle;
};
