// Fill out your copyright notice in the Description page of Project Settings.

#include "BoatPrototype/Public/PlayerBoat.h"
#include "BoatPrototype/Public/BoatBrainComponent.h"
#include "BoatPrototype/Public/Widgets/PlayerBoatCompass.h"

#include "Components/InputComponent.h"
#include "Components/WidgetComponent.h"

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

void APlayerBoat::Tick(float DeltaTime)
{
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
