// Fill out your copyright notice in the Description page of Project Settings.

#include "BoatPrototype/Public/PlayerBoat.h"
#include "BoatPrototype/Public/BoatBrainComponent.h"
#include "BoatPrototype/Public/BroadsideComponent.h"
#include "BoatPrototype/Public/Widgets/PlayerBoatCompass.h"
#include "BoatPrototype/Public/BulletActor.h"

#include "Components/InputComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

APlayerBoat::APlayerBoat()
{
	// Compass HUD mounted on the boat. Follows position but stays level in
	// world space — the dial doesn't spin with the hull. Hidden until RMB.
	CompassWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("CompassWidget"));
	CompassWidget->SetupAttachment(RootScene);
	CompassWidget->SetUsingAbsoluteRotation(true);
	CompassWidget->SetWidgetClass(UPlayerBoatCompass::StaticClass());
	CompassWidget->SetVisibility(false);

	// Brain that steers toward the mouse when enabled (RMB held).
	Brain = CreateDefaultSubobject<UBoatBrainComponent>(TEXT("Brain"));
	

}

void APlayerBoat::BeginPlay()
{
	Super::BeginPlay();
	MaxHealth += MaxHealth * 5;
	CurrentHealth = MaxHealth;
}

void APlayerBoat::PostInitializeComponents()
{
	// Runs after ABoat::PostInitializeComponents (which may rebuild Broadside
	// via the stale-component self-heal), so apply the player's fixed lane
	// layout here -- authoritative regardless of whether the heal fired.
	Super::PostInitializeComponents();

	if (Broadside)
	{
		// Player boat fields a heavy broadside: 12 lanes per side, one bullet
		// per lane (BulletCount >= NumLanes so every lane fires).
		Broadside->SetNumLanes(12);
		Broadside->SetBulletCount(12);
	}
}

void APlayerBoat::Tick(float DeltaTime)
{
	if (FireCooldown > 0.0f)
	{
		FireCooldown -= DeltaTime;
	}
	if (bIsAiming)
	{
		UpdateAiming();
	}
	if (Brain && Brain->IsMouseSteerEnabled())
	{
		SteerInput = Brain->ComputeMouseSteerInput(this);
	}
	Super::Tick(DeltaTime);
}

void APlayerBoat::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PlayerInputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &APlayerBoat::OnRightMousePressed);
	PlayerInputComponent->BindKey(EKeys::RightMouseButton, IE_Released, this, &APlayerBoat::OnRightMouseReleased);
	PlayerInputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &APlayerBoat::OnAimPressed);
	PlayerInputComponent->BindKey(EKeys::LeftMouseButton, IE_Released, this, &APlayerBoat::OnAimReleased);
	PlayerInputComponent->BindKey(EKeys::MouseScrollUp, IE_Pressed, this, &APlayerBoat::OnScrollUp);
	PlayerInputComponent->BindKey(EKeys::MouseScrollDown, IE_Pressed, this, &APlayerBoat::OnScrollDown);
}

void APlayerBoat::OnScrollUp()
{
	// Scroll up = zoom in (shorter arm).
	constexpr float ZoomSensitivity = 50.0f;
	AdjustCameraZoom(-ZoomSensitivity);
}

void APlayerBoat::OnScrollDown()
{
	// Scroll down = zoom out (longer arm).
	constexpr float ZoomSensitivity = 50.0f;
	AdjustCameraZoom(ZoomSensitivity);
}

void APlayerBoat::SteerLeftPressed()
{
	// Ignore manual steering while RMB mouse steering is active.
	if (Brain && Brain->IsMouseSteerEnabled())
	{
		return;
	}
	Super::SteerLeftPressed();
}

void APlayerBoat::SteerRightPressed()
{
	// Ignore manual steering while RMB mouse steering is active.
	if (Brain && Brain->IsMouseSteerEnabled())
	{
		return;
	}
	Super::SteerRightPressed();
}

void APlayerBoat::OnRightMousePressed()
{
	if (CompassWidget) { CompassWidget->SetVisibility(true); }
	if (Brain)
	{
		Brain->SetMouseSteerEnabled(true);
		WakeSimulation();
	}
}

void APlayerBoat::OnRightMouseReleased()
{
	if (CompassWidget) { CompassWidget->SetVisibility(false); }
	if (Brain)
	{
		Brain->SetMouseSteerEnabled(false);
		SteerInput = 0;
	}
}

bool APlayerBoat::ShouldSleep() const
{
	return Super::ShouldSleep() && FireCooldown <= 0.0f;
}

void APlayerBoat::OnAimPressed()
{
	bIsAiming = true;

	// TEMP DIAGNOSTIC: confirm LMB press actually reaches this handler.
	UE_LOG(LogTemp, Warning, TEXT("OnAimPressed"));
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1999, 1.0f, FColor::Green, TEXT("AimPressed"));
	}
}

void APlayerBoat::OnAimReleased()
{
	if (bIsAiming && FireCooldown <= 0.0f)
	{
		// Resolve an initial target now -- used as both the fallback and the
		// likely value for the first shot. Side is decided here and stays fixed
		// for the whole volley; only the target position keeps re-aiming live.
		FVector InitialTarget;
		bool bFireStarboard;

		FVector MouseWorld;
		if (GetMouseWorldPointOnBoatPlane(MouseWorld))
		{
			bFireStarboard = FVector::DotProduct(MouseWorld - GetActorLocation(), GetActorRightVector()) > 0.0f;
			InitialTarget = Broadside->ClampToBroadsideArea(MouseWorld, bFireStarboard);
		}
		else
		{
			// Fall back to a straight beam shot if the mouse can't be resolved.
			bFireStarboard = bAimingStarboard;
			const FVector Dir = bFireStarboard ? GetActorRightVector() : -GetActorRightVector();
			InitialTarget = GetActorLocation() + Dir * (GetBeamWidth() * 0.5f + GetFiringRange());
		}

		// Re-deproject the mouse at EACH bullet's own spawn moment, so a large
		// volley's later bullets follow wherever the cursor moves to, even after
		// LMB is released and the preview is hidden. Falls back to the last
		// successfully resolved point if deprojection briefly fails (e.g. cursor
		// off-viewport). Raw `this` capture is safe: the volley timer is owned by
		// Broadside, a subobject that's destroyed (and its timer cancelled) along
		// with this boat, so the lambda can't outlive its actor.
		// TEMP DIAGNOSTIC: confirm release logic runs and with what values.
		UE_LOG(LogTemp, Warning, TEXT("OnAimReleased: firing. InitialTarget=%s bFireStarboard=%d"),
			*InitialTarget.ToString(), bFireStarboard ? 1 : 0);

		const bool bSide = bFireStarboard;
		TSharedRef<FVector> LastGoodTarget = MakeShared<FVector>(InitialTarget);
		FireVolleyLive([this, bSide, LastGoodTarget]() -> FVector
		{
			FVector MouseNow;
			if (GetMouseWorldPointOnBoatPlane(MouseNow))
			{
				*LastGoodTarget = Broadside->ClampToBroadsideArea(MouseNow, bSide);
			}
			return *LastGoodTarget;
		}, bSide);

		FireCooldown = FireInterval;
		WakeSimulation(); // keep ticking so the cooldown counts down even when parked
	}

	bIsAiming = false;
	SetPortGuideVisible(false);
	SetStarboardGuideVisible(false);
}

bool APlayerBoat::GetMouseWorldPointOnBoatPlane(FVector& OutWorld) const
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC) { return false; }

	FVector RayOrigin, RayDirection;
	if (!PC->DeprojectMousePositionToWorld(RayOrigin, RayDirection) || FMath::IsNearlyZero(RayDirection.Z))
	{
		return false;
	}

	const FVector BoatLocation = GetActorLocation();
	const float T = (BoatLocation.Z - RayOrigin.Z) / RayDirection.Z;
	OutWorld = RayOrigin + RayDirection * T;
	return true;
}

void APlayerBoat::UpdateAiming()
{
	// TEMP DIAGNOSTIC: confirm UpdateAiming is entered every frame while aiming,
	// and whether Broadside is null at this point.
	UE_LOG(LogTemp, Warning, TEXT("UpdateAiming entered. Broadside=%s"), Broadside ? TEXT("VALID") : TEXT("NULL"));
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(2000, 0.0f, FColor::White,
			FString::Printf(TEXT("UpdateAiming entered. Broadside=%s"), Broadside ? TEXT("VALID") : TEXT("NULL")));
	}
	// TEMP DIAGNOSTIC: always-on rendering sanity check, independent of Broadside.
	// Also confirms whether the BOAT ACTOR itself has a valid world at this
	// moment, for comparison against Broadside's own GetWorld() result.
	UE_LOG(LogTemp, Warning, TEXT("UpdateAiming: this->GetWorld()=%s"), GetWorld() ? TEXT("valid") : TEXT("NULL"));
	DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + GetActorForwardVector() * 300.0f, FColor::Magenta, false, 0.0f, 0, 5.0f);

	if (!Broadside) { return; }

	// Always free aim: no auto-lock -- that's an enemy-only behaviour. Live
	// target follows the mouse; trajectory length tracks the mouse distance,
	// clamped to firing range.
	FVector MouseWorld;
	if (!GetMouseWorldPointOnBoatPlane(MouseWorld))
	{
		// TEMP DIAGNOSTIC: mouse deprojection failed this frame.
		UE_LOG(LogTemp, Warning, TEXT("UpdateAiming: mouse deproject FAILED"));
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(2001, 0.0f, FColor::Red, TEXT("UpdateAiming: mouse deproject FAILED"));
		}
		return;
	}

	// TEMP DIAGNOSTIC: deprojection succeeded, show the resolved world point.
	UE_LOG(LogTemp, Warning, TEXT("UpdateAiming: Mouse=%s"), *MouseWorld.ToString());
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(2002, 0.0f, FColor::Cyan,
			FString::Printf(TEXT("UpdateAiming: Mouse=%s"), *MouseWorld.ToString()));
	}

	const FVector BoatLocation = GetActorLocation();
	bAimingStarboard = FVector::DotProduct(MouseWorld - BoatLocation, GetActorRightVector()) > 0.0f;
	SetStarboardGuideVisible(bAimingStarboard);
	SetPortGuideVisible(!bAimingStarboard);

	// Constrain the aim point into the broadside strip so the preview can never
	// leave the firing area, no matter where the mouse points.
	const FVector SpawnEdge = Broadside->GetSideSpawnEdge(bAimingStarboard);
	const FVector AimPoint = Broadside->ClampToBroadsideArea(MouseWorld, bAimingStarboard);

	Broadside->DrawTrajectoryPreview(SpawnEdge, AimPoint);
}
