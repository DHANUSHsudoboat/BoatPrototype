// Fill out your copyright notice in the Description page of Project Settings.

#include "BoatPrototype/Public/BoatBrainComponent.h"

#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

float UBoatBrainComponent::ComputeMouseSteerInput(const APawn* Boat) const
{
	if (!Boat)
	{
		return 0.0f;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		return 0.0f;
	}

	FVector RayOrigin, RayDirection;
	if (!PC->DeprojectMousePositionToWorld(RayOrigin, RayDirection) || FMath::IsNearlyZero(RayDirection.Z))
	{
		return 0.0f;
	}

	const FVector BoatLocation = Boat->GetActorLocation();
	const float T = (BoatLocation.Z - RayOrigin.Z) / RayDirection.Z;
	const FVector MouseWorldPos = RayOrigin + RayDirection * T;

	const FVector ToMouse = MouseWorldPos - BoatLocation;
	if (ToMouse.IsNearlyZero())
	{
		return 0.0f;
	}

	const float DesiredYaw = FMath::RadiansToDegrees(FMath::Atan2(ToMouse.Y, ToMouse.X));
	const float CurrentYaw = Boat->GetActorRotation().Yaw;
	const float HeadingError = FMath::FindDeltaAngleDegrees(CurrentYaw, DesiredYaw);

	return FMath::Clamp(HeadingError / FullSteerHeadingError, -1.0f, 1.0f);
}
