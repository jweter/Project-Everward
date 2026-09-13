#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FixItRuntimeActor.generated.h"

class UProbeSimulationAdapter;

namespace everward::simulation
{
class DamageAwareProbeRuntime;
class FixItRepairExecutor;
class FixItReplacementExecutor;
}

UCLASS()
class EVERWARD_API AFixItRuntimeActor : public AActor
{
    GENERATED_BODY()

public:
    AFixItRuntimeActor();
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintPure, Category="Everward|Fix_It")
    FString GetFixItStatusSummary() const { return StatusSummary; }

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void BindToCurrentProbe();
    void SeedDamagedAwakening();
    void AdvanceFixIt(double DeltaSeconds);
    void ReplanOrStart();
    void RefreshStatusSummary();
    void RecordFixItEvent(const FString& EventName, const FString& Details) const;

    static constexpr double FixedFixItStepSeconds = 1.0 / 60.0;
    // How long a completed repair/replacement's material breakdown stays on
    // screen before yielding to the next decision's status line.
    static constexpr double FixItCompletionDisplaySeconds = 4.0;

    UProbeSimulationAdapter* Adapter = nullptr;
    everward::simulation::DamageAwareProbeRuntime* BoundCore = nullptr;
    everward::simulation::FixItRepairExecutor* RepairExecutor = nullptr;
    everward::simulation::FixItReplacementExecutor* ReplacementExecutor = nullptr;

    double StepAccumulatorSeconds = 0.0;
    bool bSeededDamagedAwakening = false;
    bool bRepairRunning = false;
    bool bReplacementRunning = false;
    bool bWaitingForResources = false;

    FString ActiveStage = TEXT("DIAGNOSE");
    FString ActiveSubsystem = TEXT("NONE");
    FString ActiveReason;
    FString StatusSummary = TEXT("FIX_IT // INITIALIZING");
    FString LastCompletionSummary;
    double CompletionDisplaySecondsRemaining = 0.0;
};
