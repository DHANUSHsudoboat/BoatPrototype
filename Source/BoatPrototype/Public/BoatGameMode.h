// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BoatGameMode.generated.h"

/**
 * Minimal game mode for the boat prototype.
 * Uses a plain PlayerController (no TopDown click-to-move) so the boat
 * is driven only by its own W/S/A/D input.
 */
UCLASS()
class BOATPROTOTYPE_API ABoatGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ABoatGameMode();
};
