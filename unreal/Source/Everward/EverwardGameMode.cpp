#include "EverwardGameMode.h"

#include "Engine/World.h"
#include "EverwardHUD.h"
#include "EverwardPhase2TestEnvironment.h"
#include "EverwardPlayerController.h"
#include "EverwardProbePawn.h"
#include "GameFramework/PlayerStart.h"
#include "PlaytestRecorderActor.h"

AEverwardGameMode::AEverwardGameMode()
{
    DefaultPawnClass = AEverwardProbePawn::StaticClass();
    HUDClass = AEverwardHUD::StaticClass();
    PlayerControllerClass = AEverwardPlayerController::StaticClass();
}

void AEverwardGameMode::InitGame(
    const FString& MapName,
    const FString& Options,
    FString& ErrorMessage)
{
    Super::InitGame(MapName, Options, ErrorMessage);

    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return;
    }

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    Phase2PlayerStart = World->SpawnActor<APlayerStart>(
        APlayerStart::StaticClass(),
        FVector::ZeroVector,
        FRotator::ZeroRotator,
        SpawnParameters);

    World->SpawnActor<AEverwardPhase2TestEnvironment>(
        AEverwardPhase2TestEnvironment::StaticClass(),
        FVector::ZeroVector,
        FRotator::ZeroRotator,
        SpawnParameters);

    // Playtest evidence is deliberately non-blocking. If recorder creation
    // ever fails, the actual game still launches and remains testable.
    if (World->SpawnActor<APlaytestRecorderActor>(
            APlaytestRecorderActor::StaticClass(),
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            SpawnParameters) == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("Everward playtest recorder unavailable; continuing without structured capture"));
    }
}

AActor* AEverwardGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
    if (IsValid(Phase2PlayerStart))
    {
        return Phase2PlayerStart;
    }

    return Super::ChoosePlayerStart_Implementation(Player);
}
