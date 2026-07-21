// Fill out your copyright notice in the Description page of Project Settings.

#include "BoatPrototype/Public/Widgets/BoatHealthWidget.h"
#include "BoatPrototype/Public/Boat.h"
#include "Components/ProgressBar.h"
#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"

void UBoatHealthWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Get the boat this widget is mounted on.
	if (UWidgetComponent* OwnerComponent = Cast<UWidgetComponent>(GetOuter()))
	{
		OwnerBoat = Cast<ABoat>(OwnerComponent->GetOwner());
	}
	
}


void UBoatHealthWidget::UpdateHealthDisplay(float HealthPercent)
{
	if (!HealthBar)
	{
		return;
	}

	HealthBar->SetPercent(FMath::Clamp(HealthPercent, 0.0f, 1.0f));
}
