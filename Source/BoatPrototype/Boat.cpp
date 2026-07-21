// Fill out your copyright notice in the Description page of Project Settings.


#include "Boat.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
ABoat::ABoat()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Plain root: movement + steering act on this, independent of hull orientation.
	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	RootComponent = RootScene;

	// Visual hull attached under root; rotate freely to align art.
	HullMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HullMesh"));
	HullMesh->SetupAttachment(RootComponent);

	// Forward-vector gizmo (+X). On the root, independent of hull rotation.
	ForwardArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("ForwardArrow"));
	ForwardArrow->SetupAttachment(RootComponent);
	ForwardArrow->ArrowColor = FColor::Red;
	ForwardArrow->ArrowSize = 2.0f;
	ForwardArrow->bIsScreenSizeScaled = true;
#if WITH_EDITORONLY_DATA
	ForwardArrow->bHiddenInGame = true;
#endif

	// Camera boom on the root, angled down behind the boat.
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 1200.0f;
	SpringArm->SetRelativeRotation(FRotator(-45.0f, 0.0f, 0.0f));
	// Follow boat position but keep a fixed world rotation — camera won't spin with the boat.
	SpringArm->SetUsingAbsoluteRotation(true);
	SpringArm->bDoCollisionTest = false;
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagSpeed = 5.0f;

	// Follow camera at the end of the boom.
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
}

void ABoat::BeginPlay()
{
	Super::BeginPlay();
}

// ---------------------------------------------------------------------
// Frame loop
// ---------------------------------------------------------------------

void ABoat::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateSpeed(DeltaTime);
	UpdateSteering(DeltaTime);
	UpdateMovement(DeltaTime);

	// On-screen HUD: current gear + speed.
	if (bShowDebugHUD && GEngine)
	{
		const FString Msg = FString::Printf(TEXT("Gear: %d   Speed: %.0f cm/s"), CurrentGear, CurrentSpeed);
		GEngine->AddOnScreenDebugMessage(1, 0.0f, FColor::Green, Msg);
	}

	// Nothing left to simulate (parked, no target, not turning) -> stop ticking.
	if (ShouldSleep())
	{
		SetActorTickEnabled(false);
	}
}

void ABoat::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Gears. Bind directly to keys so no project input mappings are required.
	PlayerInputComponent->BindKey(EKeys::W, IE_Pressed, this, &ABoat::ShiftUpGear);
	PlayerInputComponent->BindKey(EKeys::S, IE_Pressed, this, &ABoat::ShiftDownGear);

	// Steering: hold A / D. Release restores straight travel.
	PlayerInputComponent->BindKey(EKeys::A, IE_Pressed, this, &ABoat::SteerLeftPressed);
	PlayerInputComponent->BindKey(EKeys::D, IE_Pressed, this, &ABoat::SteerRightPressed);
	PlayerInputComponent->BindKey(EKeys::A, IE_Released, this, &ABoat::SteerReleased);
	PlayerInputComponent->BindKey(EKeys::D, IE_Released, this, &ABoat::SteerReleased);
}

// ---------------------------------------------------------------------
// Simulation
// ---------------------------------------------------------------------

void ABoat::UpdateSpeed(float DeltaTime)
{
	const float TargetSpeed = GetTargetSpeed();

	// Accelerate or decelerate toward the target — never snap. Rates are weight-scaled.
	const float Rate = (TargetSpeed > CurrentSpeed) ? GetAccelerationRate() : GetDecelerationRate();

	CurrentSpeed = FMath::FInterpConstantTo(CurrentSpeed, TargetSpeed, DeltaTime, Rate);
	CurrentSpeed = FMath::Clamp(CurrentSpeed, 0.0f, MaximumSpeed);
}

void ABoat::UpdateSteering(float DeltaTime)
{
	// --- Helm swing: the input eases toward a smoothed helm angle. ---
	const float TargetRudder = SteerInput * MaxRudderAngle;
	RudderAngle = FMath::FInterpConstantTo(RudderAngle, TargetRudder, DeltaTime, RudderSpeed);

	// --- Yaw: ease toward a target rate (full at rest, slower with speed). ---
	const float SwayForce = GetRudderSwayForce();   // cm/s^2, side kick that drives the drift
	YawRate = FMath::FInterpTo(YawRate, GetTargetYawRate(), DeltaTime, YawResponsiveness);
	YawRate = FMath::Clamp(YawRate, -MaxYawRate, MaxYawRate);

	// r in rad/s for the rotating-frame coupling that curves the path (drift while moving).
	const float YawRateRad = FMath::DegreesToRadians(YawRate);

	// --- Sway: rudder side kick + centripetal coupling - lateral hull drag. ---
	// Wider hull grips the water sideways harder (more lateral resistance).
	const float WidthFactor = Width / ReferenceWidth;
	const float SwayAccel = SwayForce
		- YawRateRad * CurrentSpeed                               // -r*u : centripetal coupling
		- SwayDrag * SwaySpeed * WidthFactor;                    // lateral hull resistance
	SwaySpeed += SwayAccel * DeltaTime;

	// --- Surge feedback: centripetal +r*v, minus turn-induced speed loss. ---
	const float TurnDrag = TurnDragFactor
		* (FMath::Abs(SwaySpeed) + FMath::Abs(FMath::Sin(FMath::DegreesToRadians(RudderAngle))) * FMath::Abs(CurrentSpeed));
	CurrentSpeed += (YawRateRad * SwaySpeed - TurnDrag) * DeltaTime;
	CurrentSpeed = FMath::Clamp(CurrentSpeed, 0.0f, MaximumSpeed);

	// --- Apply the heading change. ---
	if (!FMath::IsNearlyZero(YawRate))
	{
		AddActorLocalRotation(FRotator(0.0f, YawRate * DeltaTime, 0.0f));
	}
}

void ABoat::UpdateMovement(float DeltaTime)
{
	if (FMath::IsNearlyZero(CurrentSpeed) && FMath::IsNearlyZero(SwaySpeed))
	{
		return;
	}

	// Travel along the hull axes: forward surge + lateral sway = the drifting arc.
	const FVector Delta = (GetActorForwardVector() * CurrentSpeed
		+ GetActorRightVector() * SwaySpeed) * DeltaTime;
	AddActorWorldOffset(Delta, false);
}

// ---------------------------------------------------------------------
// Gear / steering input
// ---------------------------------------------------------------------

void ABoat::ShiftUpGear()
{
	const int32 Prev = CurrentGear;
	CurrentGear = FMath::Min(CurrentGear + 1, 3);
	if (CurrentGear != Prev)
	{
		PlayGearShake();
		WakeSimulation();
	}
}

void ABoat::ShiftDownGear()
{
	const int32 Prev = CurrentGear;
	CurrentGear = FMath::Max(CurrentGear - 1, 0);
	if (CurrentGear != Prev)
	{
		PlayGearShake();
		WakeSimulation();
	}
}

void ABoat::SteerLeftPressed()
{
	SteerInput = -1.0f;
	WakeSimulation();
}

void ABoat::SteerRightPressed()
{
	SteerInput = 1.0f;
	WakeSimulation();
}

void ABoat::SteerReleased()
{
	SteerInput = 0.0f;
}

// ---------------------------------------------------------------------
// Derived values
// ---------------------------------------------------------------------

float ABoat::GetTargetSpeed() const
{
	float Target = 0.0f;
	switch (CurrentGear)
	{
	case 1: Target = Gear1Speed; break;
	case 2: Target = Gear2Speed; break;
	case 3: Target = Gear3Speed; break;
	default: break; // Gear 0 = full stop.
	}

	return FMath::Clamp(Target, 0.0f, MaximumSpeed);
}

float ABoat::GetAccelerationRate() const
{
	// Heavier than reference -> ratio < 1 -> slower acceleration (more inertia).
	return AccelerationRate * (ReferenceWeight / Weight);
}

float ABoat::GetDecelerationRate() const
{
	return DecelerationRate * (ReferenceWeight / Weight);
}

float ABoat::GetTargetYawRate() const
{
	// Helm demand, -1..+1 (smoothed input).
	const float Helm = (MaxRudderAngle > KINDA_SMALL_NUMBER)
		? FMath::Clamp(RudderAngle / MaxRudderAngle, -1.0f, 1.0f)
		: 0.0f;

	// Faster boat = lazier turn. Full authority at rest, floored so it never dies.
	const float SpeedNorm = (MaximumSpeed > KINDA_SMALL_NUMBER)
		? FMath::Clamp(CurrentSpeed / MaximumSpeed, 0.0f, 1.0f)
		: 0.0f;
	const float SpeedFactor = FMath::Clamp(1.0f - SpeedTurnReduction * SpeedNorm, TurnSpeedFloor, 1.0f);

	// Heavier than reference -> ratio < 1 -> slower rotation at every speed.
	const float WeightFactor = ReferenceWeight / Weight;

	return Helm * BaseTurnRate * WeightFactor * SpeedFactor;
}

float ABoat::GetRudderSwayForce() const
{
	// Stern kick that seeds the drift: scales with helm and with water flow^2, so it
	// only bites while making way (no lateral kick at rest -> clean pivot).
	// +helm to starboard kicks the stern outboard to port.
	const float FlowNorm = (MaximumSpeed > KINDA_SMALL_NUMBER)
		? CurrentSpeed / MaximumSpeed
		: 0.0f;
	const float Flow2 = FlowNorm * FMath::Abs(FlowNorm);
	const float SideForce = FMath::Sin(FMath::DegreesToRadians(RudderAngle)) * Flow2;

	// Lateral acceleration proportional to steering authority and top speed.
	return -0.15f * BaseTurnRate * SideForce * (MaximumSpeed / 100.0f);
}

void ABoat::PlayGearShake()
{
	if (!GearShake)
	{
		return;
	}

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		PC->ClientStartCameraShake(GearShake, GearShakeScale);
	}
}

// ---------------------------------------------------------------------
// Tick gating
// ---------------------------------------------------------------------

void ABoat::WakeSimulation()
{
	SetActorTickEnabled(true);
}

bool ABoat::ShouldSleep() const
{
	return FMath::IsNearlyZero(CurrentSpeed)
		&& FMath::IsNearlyZero(GetTargetSpeed())
		&& FMath::IsNearlyZero(YawRate)
		&& FMath::IsNearlyZero(SwaySpeed)
		&& FMath::IsNearlyZero(RudderAngle)
		&& FMath::IsNearlyZero(SteerInput);
}
