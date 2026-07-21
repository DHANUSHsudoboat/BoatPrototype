// Fill out your copyright notice in the Description page of Project Settings.



#include "BoatPrototype/Public/BoatGameMode.h"


#include "BoatPrototype/Public/Boat.h"
#include "GameFramework/PlayerController.h"

ABoatGameMode::ABoatGameMode()
{
	// Drive the boat directly; no navigation / click-to-move.
	DefaultPawnClass = ABoat::StaticClass();
	PlayerControllerClass = APlayerController::StaticClass();
}
