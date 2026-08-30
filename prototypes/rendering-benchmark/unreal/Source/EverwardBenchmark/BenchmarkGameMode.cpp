#include "BenchmarkGameMode.h"

#include "BenchmarkAdapter.h"
#include "PlaytestRecorderActor.h"
#include "EngineUtils.h"
#include "Engine/World.h"

void ABenchmarkGameMode::StartPlay()
{
    Super::StartPlay();

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    bool bAdapterPresent = false;
    for (TActorIterator<ABenchmarkAdapter> It(World); It; ++It)
    {
        bAdapterPresent = true;
        break;
    }

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    if (!bAdapterPresent)
    {
        ABenchmarkAdapter* Adapter = World->SpawnActor<ABenchmarkAdapter>(
            ABenchmarkAdapter::StaticClass(),
            FTransform::Identity,
            SpawnParameters);

        if (!Adapter)
        {
            UE_LOG(LogTemp, Error, TEXT("Unable to spawn Everward benchmark adapter"));
        }
        else
        {
            UE_LOG(LogTemp, Display, TEXT("Everward benchmark adapter spawned by BenchmarkGameMode"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Display, TEXT("Everward benchmark adapter already present; startup spawn skipped"));
    }

    bool bRecorderPresent = false;
    for (TActorIterator<APlaytestRecorderActor> It(World); It; ++It)
    {
        bRecorderPresent = true;
        break;
    }

    if (!bRecorderPresent)
    {
        APlaytestRecorderActor* Recorder = World->SpawnActor<APlaytestRecorderActor>(
            APlaytestRecorderActor::StaticClass(),
            FTransform::Identity,
            SpawnParameters);

        if (!Recorder)
        {
            // Evidence capture must never prevent gameplay from starting.
            UE_LOG(LogTemp, Warning, TEXT("Unable to spawn Everward playtest recorder; continuing without playtest evidence capture"));
        }
        else
        {
            UE_LOG(LogTemp, Display, TEXT("Everward non-blocking playtest recorder spawned by BenchmarkGameMode"));
        }
    }
}
