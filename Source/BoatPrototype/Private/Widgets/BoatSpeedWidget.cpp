// Fill out your copyright notice in the Description page of Project Settings.

#include "BoatPrototype/Public/Widgets/BoatSpeedWidget.h"
#include "BoatPrototype/Public/Boat.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"

void UBoatSpeedWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Get the player boat for polling state.
	if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
	{
		OwnerBoat = Cast<ABoat>(PlayerPawn);
	}

	// Initialize display with current boat state.
	if (OwnerBoat)
	{
		UpdateGearDisplay(OwnerBoat->GetCurrentGear());
		UpdateSpeedDisplay(OwnerBoat->GetCurrentSpeed());
	}
}

void UBoatSpeedWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!OwnerBoat)
	{
		return;
	}

	// Update display every tick to reflect live boat state.
	UpdateGearDisplay(OwnerBoat->GetCurrentGear());
	UpdateSpeedDisplay(OwnerBoat->GetCurrentSpeed());
}

void UBoatSpeedWidget::UpdateGearDisplay(int32 CurrentGear)
{
	if (!GearText)
	{
		return;
	}

	FString GearLabel = FString::Printf(TEXT("GEAR %d"), CurrentGear);
	GearText->SetText(FText::FromString(GearLabel));
}

void UBoatSpeedWidget::UpdateSpeedDisplay(float InCurrentSpeed)
{
	if (!this->CurrentSpeed)
	{
		return;
	}

	FString SpeedLabel = FString::Printf(TEXT("%.0f m/s"), InCurrentSpeed);
	this->CurrentSpeed->SetText(FText::FromString(SpeedLabel));
}
