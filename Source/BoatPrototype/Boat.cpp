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
	// Turning FORCE -> angular acceleration (scales with speed), then water drag on the yaw rate.
	const float TurnAccel = GetTurnAcceleration();
	YawRate += TurnAccel * DeltaTime;
	YawRate -= YawRate * TurnDamping * DeltaTime;      // water resistance settles the turn
	YawRate = FMath::Clamp(YawRate, -MaxYawRate, MaxYawRate);

	if (!FMath::IsNearlyZero(YawRate))
	{
		AddActorLocalRotation(FRotator(0.0f, YawRate * DeltaTime, 0.0f));
	}
}

void ABoat::UpdateMovement(float DeltaTime)
{
	if (FMath::IsNearlyZero(CurrentSpeed))
	{
		return;
	}

	const FVector Delta = GetActorForwardVector() * CurrentSpeed * DeltaTime;
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

float ABoat::GetTurnAcceleration() const
{
	// A rudder only bites while making way: turning force scales with speed.
	const float SpeedFactor = (MaximumSpeed > KINDA_SMALL_NUMBER)
		? FMath::Clamp(CurrentSpeed / MaximumSpeed, 0.0f, 1.0f)
		: 0.0f;

	// Wider hull weakens the turn; heavier boat resists rotating.
	const float WidthFactor = ReferenceWidth / Width;
	const float WeightFactor = ReferenceWeight / Weight;

	return SteerInput * TurnForce * SpeedFactor * WidthFactor * WeightFactor;
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
		&& FMath::IsNearlyZero(SteerInput);
}
