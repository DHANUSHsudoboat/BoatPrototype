// Fill out your copyright notice in the Description page of Project Settings.

#include "BoatPrototype/Public/Widgets/PlayerBoatCompass.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/SizeBox.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

void UPlayerBoatCompass::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!ArrowSizeBox)
	{
		return;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	APawn* BoatPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!PC || !BoatPawn)
	{
		return;
	}

	FVector RayOrigin, RayDirection;
	if (!PC->DeprojectMousePositionToWorld(RayOrigin, RayDirection) || FMath::IsNearlyZero(RayDirection.Z))
	{
		return;
	}

	const FVector BoatLocation = BoatPawn->GetActorLocation();

	// Intersect the mouse ray with the horizontal plane through the boat, so
	// the mouse's world position is comparable to the boat's location.
	const float T = (BoatLocation.Z - RayOrigin.Z) / RayDirection.Z;
	const FVector MouseWorldPos = RayOrigin + RayDirection * T;

	const FVector2D Direction(MouseWorldPos.X - BoatLocation.X, MouseWorldPos.Y - BoatLocation.Y);
	if (Direction.IsNearlyZero())
	{
		return;
	}
	const float Distance = Direction.Size();

	// Camera yaw is fixed (SetUsingAbsoluteRotation on the SpringArm), so its
	// forward/right vectors are a stable "up/right on screen" basis. North
	// (screen-up) = 0 deg, east (screen-right) = 90 deg, and anything between
	// (e.g. NE) falls out proportionally from the dot products.
	const FRotator CameraYaw(0.0f, PC->PlayerCameraManager->GetCameraRotation().Yaw, 0.0f);
	const FVector CameraForward = FRotationMatrix(CameraYaw).GetUnitAxis(EAxis::X);
	const FVector CameraRight = FRotationMatrix(CameraYaw).GetUnitAxis(EAxis::Y);
	const FVector WorldOffset(Direction.X, Direction.Y, 0.0f);

	const float ForwardComponent = FVector::DotProduct(WorldOffset, CameraForward);
	const float RightComponent = FVector::DotProduct(WorldOffset, CameraRight);

	float RawAngleDeg = FMath::RadiansToDegrees(FMath::Atan2(RightComponent, ForwardComponent));
	float AngleDeg = RawAngleDeg - ArrowRestAngle;
	AngleDeg = FMath::Fmod(AngleDeg + 360.0f, 360.0f);

	if (GEngine)
	{
		const FString Msg = FString::Printf(TEXT("Mouse dist: %.0f cm   Angle: %.1f deg   [Up=0, Right=90, Down=180, Left=270]"), Distance, AngleDeg);
		GEngine->AddOnScreenDebugMessage(2, 0.0f, FColor::Cyan, Msg);
	}

	ArrowSizeBox->SetRenderTransformAngle(AngleDeg);
}
