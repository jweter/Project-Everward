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

// Read-only Blueprint projection of the simulation-owned Fix_It decision.
// It does not grant repair or invent a resource source or capability unlock.
UENUM(BlueprintType)
enum class EEverwardRecoveryGoalState : uint8
{
    NoRecommendation,
    Blocked,
    Ready,
    InProgress,
    Interrupted,
    Completed,
    VerificationFailed,
    FullyRestored,
    AdvisoryOnly
};

USTRUCT(BlueprintType)
struct EVERWARD_API FEverwardRecoveryGoalStatus
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Everward|Fix_It|Goal") bool bHasGoal = false;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Fix_It|Goal") FString GoalId;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Fix_It|Goal") EEverwardRecoveryGoalState State = EEverwardRecoveryGoalState::NoRecommendation;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Fix_It|Goal") FString Stage;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Fix_It|Goal") FString Subsystem;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Fix_It|Goal") FString PlannerReason;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Fix_It|Goal") double CurrentIntegrity = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Fix_It|Goal") double TargetIntegrity = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Fix_It|Goal") double RequiredMaterialKg = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Fix_It|Goal") double MissingMaterialKg = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Fix_It|Goal") double RequiredEnergyJoules = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Fix_It|Goal") double MissingEnergyJoules = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Fix_It|Goal") double EstimatedDurationSeconds = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Fix_It|Goal") double ProgressFraction = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Fix_It|Goal") bool bFabricationUnavailable = false;
};

UCLASS()
class EVERWARD_API AFixItRuntimeActor : public AActor
{
    GENERATED_BODY()

public:
    AFixItRuntimeActor();
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintPure, Category="Everward|Fix_It")
    FString GetFixItStatusSummary() const { return StatusSummary; }

    // Recomputed from current authoritative integrity, resources and the
    // current executor receipt. The HUD must not treat this as permission.
    UFUNCTION(BlueprintPure, Category="Everward|Fix_It|Goal")
    FEverwardRecoveryGoalStatus GetRecoveryGoal() const;

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
