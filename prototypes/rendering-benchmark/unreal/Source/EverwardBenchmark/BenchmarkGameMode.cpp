#include "BenchmarkGameMode.h"

#include "BenchmarkAdapter.h"
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

    for (TActorIterator<ABenchmarkAdapter> It(World); It; ++It)
    {
        UE_LOG(LogTemp, Display, TEXT("Everward benchmark adapter already present; startup spawn skipped"));
        return;
    }

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    ABenchmarkAdapter* Adapter = World->SpawnActor<ABenchmarkAdapter>(
        ABenchmarkAdapter::StaticClass(),
        FTransform::Identity,
        SpawnParameters);

    if (!Adapter)
    {
        UE_LOG(LogTemp, Error, TEXT("Unable to spawn Everward benchmark adapter"));
        return;
    }

    UE_LOG(LogTemp, Display, TEXT("Everward benchmark adapter spawned by BenchmarkGameMode"));
}
