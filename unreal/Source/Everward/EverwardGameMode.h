#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "EverwardGameMode.generated.h"

class AController;
class APlayerStart;

// Production Phase 2 bootstrap. The default single local player receives one
// AEverwardProbePawn, which in turn owns one UProbeSimulationAdapter. During
// Phase 2 this game mode also creates a reproducible integration environment
// and deterministic player start so a useful first-run test does not depend
// on an editor-authored map.
UCLASS()
class EVERWARD_API AEverwardGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AEverwardGameMode();

    virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
    virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

private:
    UPROPERTY()
    TObjectPtr<APlayerStart> Phase2PlayerStart;
};
