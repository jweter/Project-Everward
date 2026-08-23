#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "EverwardGameMode.generated.h"

// Production Phase 2 bootstrap. The default single local player receives one
// AEverwardProbePawn, which in turn owns one UProbeSimulationAdapter.
UCLASS()
class EVERWARD_API AEverwardGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AEverwardGameMode();
};
