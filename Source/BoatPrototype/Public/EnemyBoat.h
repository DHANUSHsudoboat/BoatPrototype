// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Boat.h"
#include "EnemyBoat.generated.h"

/**
 * AI-driven boat variant. Inherits the full steering + movement system from
 * ABoat. Ready for AI-specific behavior, tuning, or component overrides.
 */
UCLASS()
class BOATPROTOTYPE_API AEnemyBoat : public ABoat
{
	GENERATED_BODY()

public:
	AEnemyBoat();
};
