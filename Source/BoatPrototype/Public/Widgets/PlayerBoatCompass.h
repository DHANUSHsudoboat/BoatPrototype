// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerBoatCompass.generated.h"

/**
 * Compass HUD widget. Displays an arrow that rotates to point from the
 * compass center toward the current mouse cursor position.
 */
UCLASS()
class BOATPROTOTYPE_API UPlayerBoatCompass : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// SizeBox containing the arrow; rotates to point at the mouse.
	// Add a SizeBox widget named exactly "ArrowSizeBox" in this widget's UMG
	// Designer, above the compass background, pivoted at its own center.
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class USizeBox* ArrowSizeBox;

	// Degrees to offset the computed bearing. Accounts for both the arrow art's
	// rest orientation and whichever world axis the compass dial treats as north.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compass")
	float ArrowRestAngle = 0.0f;
};
