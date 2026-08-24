#include "EverwardGameMode.h"

#include "EverwardHUD.h"
#include "EverwardPlayerController.h"
#include "EverwardProbePawn.h"

AEverwardGameMode::AEverwardGameMode()
{
    DefaultPawnClass = AEverwardProbePawn::StaticClass();
    HUDClass = AEverwardHUD::StaticClass();
    PlayerControllerClass = AEverwardPlayerController::StaticClass();
}
