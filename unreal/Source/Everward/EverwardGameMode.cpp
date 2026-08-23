#include "EverwardGameMode.h"

#include "EverwardProbePawn.h"

AEverwardGameMode::AEverwardGameMode()
{
    DefaultPawnClass = AEverwardProbePawn::StaticClass();
}
