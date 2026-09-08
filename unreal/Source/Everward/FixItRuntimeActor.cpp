#include "FixItRuntimeActor.h"

#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "EverwardProbePawn.h"
#include "Kismet/GameplayStatics.h"
#include "PlaytestRecorderActor.h"
#include "ProbeSimulationAdapter.h"
#include "everward/simulation/fix_it.hpp"

#include <limits>

namespace
{
const TCHAR* FixItSubsystemName(everward::simulation::PowerSubsystem Subsystem)
{
    switch (Subsystem)
    {
        case everward::simulation::PowerSubsystem::Sensors: return TEXT("SENSORS");
        case everward::simulation::PowerSubsystem::Propulsion: return TEXT("PROPULSION");
        case everward::simulation::PowerSubsystem::Computation: return TEXT("COMPUTATION");
        case everward::simulation::PowerSubsystem::Thermal: return TEXT("THERMAL");
    }
    return TEXT("UNKNOWN");
}

const TCHAR* FixItStageName(everward::simulation::FixItStage Stage)
{
    switch (Stage)
    {
        case everward::simulation::FixItStage::Repair: return TEXT("REPAIR");
        case everward::simulation::FixItStage::Replacement: return TEXT("REPLACEMENT");
        case everward::simulation::FixItStage::Upgrade: return TEXT("UPGRADE");
        case everward::simulation::FixItStage::Redesign: return TEXT("REDESIGN");
        case everward::simulation::FixItStage::Evolution: return TEXT("EVOLUTION");
    }
    return TEXT("UNKNOWN");
}

bool HasCanonicalUndamagedIntegrity(const everward::simulation::DamageAwareProbeRuntime& Core)
{
    const auto& Integrity = Core.component_integrity();
    return Integrity.sensors >= 0.999 &&
           Integrity.propulsion >= 0.999 &&
           Integrity.computation >= 0.999 &&
           Integrity.thermal >= 0.999;
}
}

AFixItRuntimeActor::AFixItRuntimeActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.0f;
}

void AFixItRuntimeActor::BeginPlay()
{
    Super::BeginPlay();
    RepairExecutor = new everward::simulation::FixItRepairExecutor();
    ReplacementExecutor = new everward::simulation::FixItReplacementExecutor();
    BindToCurrentProbe();
}

void AFixItRuntimeActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    delete RepairExecutor;
    RepairExecutor = nullptr;
    delete ReplacementExecutor;
    ReplacementExecutor = nullptr;
    Adapter = nullptr;
    BoundCore = nullptr;
    Super::EndPlay(EndPlayReason);
}

void AFixItRuntimeActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    BindToCurrentProbe();

    if (BoundCore == nullptr)
    {
        StatusSummary = TEXT("FIX_IT // WAITING FOR PROBE RUNTIME");
        return;
    }

    StepAccumulatorSeconds += FMath::Max(0.0, static_cast<double>(DeltaSeconds));
    while (StepAccumulatorSeconds >= FixedFixItStepSeconds)
    {
        AdvanceFixIt(FixedFixItStepSeconds);
        StepAccumulatorSeconds -= FixedFixItStepSeconds;
    }

    RefreshStatusSummary();

    // First playable presentation surface. Simulation truth stays in the
    // engine-independent planner/executors; this persistent line only exposes
    // what Fix_It is deciding and why until the wider HUD redesign lands.
    if (GEngine != nullptr)
    {
        GEngine->AddOnScreenDebugMessage(
            0xF17,
            0.20f,
            bWaitingForResources ? FColor::Yellow : FColor::Cyan,
            StatusSummary,
            true,
            FVector2D(1.15f, 1.15f));
    }
}

void AFixItRuntimeActor::BindToCurrentProbe()
{
    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return;
    }

    APawn* Pawn = UGameplayStatics::GetPlayerPawn(World, 0);
    AEverwardProbePawn* Probe = Cast<AEverwardProbePawn>(Pawn);
    UProbeSimulationAdapter* NewAdapter = Probe != nullptr ? Probe->GetSimulationAdapter() : nullptr;
    if (NewAdapter == nullptr || NewAdapter->Core == nullptr)
    {
        return;
    }

    if (Adapter != NewAdapter || BoundCore != NewAdapter->Core)
    {
        Adapter = NewAdapter;
        BoundCore = NewAdapter->Core;
        bRepairRunning = false;
        bReplacementRunning = false;
        bWaitingForResources = false;
        ActiveStage = TEXT("DIAGNOSE");
        ActiveSubsystem = TEXT("NONE");
        ActiveReason.Reset();

        if (!bSeededDamagedAwakening && HasCanonicalUndamagedIntegrity(*BoundCore))
        {
            SeedDamagedAwakening();
        }

        RecordFixItEvent(
            TEXT("fix_it_runtime_bound"),
            TEXT("Fix_It connected to the authoritative EV-0001 probe runtime."));
    }
}

void AFixItRuntimeActor::SeedDamagedAwakening()
{
    if (BoundCore == nullptr)
    {
        return;
    }

    // Generation-1 wakes damaged but not helpless. Every subsystem remains
    // above zero, so the player can limp, scan slowly, and mine nearby matter
    // while Fix_It restores existence/capability in the canonical order.
    BoundCore->set_subsystem_integrity(everward::simulation::PowerSubsystem::Computation, 0.18);
    BoundCore->set_subsystem_integrity(everward::simulation::PowerSubsystem::Thermal, 0.22);
    BoundCore->set_subsystem_integrity(everward::simulation::PowerSubsystem::Sensors, 0.35);
    BoundCore->set_subsystem_integrity(everward::simulation::PowerSubsystem::Propulsion, 0.40);

    bSeededDamagedAwakening = true;
    RecordFixItEvent(
        TEXT("fix_it_awakened"),
        TEXT("Fix_It active: Gen-1 startup integrity COMPUTATION 18%, THERMAL 22%, SENSORS 35%, PROPULSION 40%."));
}

void AFixItRuntimeActor::AdvanceFixIt(double DeltaSeconds)
{
    if (BoundCore == nullptr || RepairExecutor == nullptr || ReplacementExecutor == nullptr)
    {
        return;
    }

    if (!bRepairRunning && !bReplacementRunning)
    {
        ReplanOrStart();
        return;
    }

    if (bRepairRunning)
    {
        RepairExecutor->advance(*BoundCore, DeltaSeconds);
        const auto& Status = RepairExecutor->status();

        if (Status.state == everward::simulation::FixItExecutionState::Completed)
        {
            bRepairRunning = false;
            bWaitingForResources = false;
            RecordFixItEvent(
                TEXT("fix_it_repair_completed"),
                FString::Printf(
                    TEXT("%s restored to %.0f%%; consumed %.2f kg and %.0f J."),
                    *ActiveSubsystem,
                    Status.decision.has_value() ? Status.decision->target_integrity * 100.0 : 0.0,
                    Status.material_consumed_kg,
                    Status.energy_consumed_j));
        }
        else if (Status.state == everward::simulation::FixItExecutionState::Interrupted)
        {
            bRepairRunning = false;
            bWaitingForResources = true;
            RecordFixItEvent(TEXT("fix_it_repair_interrupted"), UTF8_TO_TCHAR(Status.detail.c_str()));
        }
        return;
    }

    if (bReplacementRunning)
    {
        ReplacementExecutor->advance(*BoundCore, DeltaSeconds);
        const auto& Status = ReplacementExecutor->status();
        if (Status.state == everward::simulation::FixItExecutionState::Completed)
        {
            bReplacementRunning = false;
            bWaitingForResources = false;
            RecordFixItEvent(
                TEXT("fix_it_replacement_completed"),
                FString::Printf(TEXT("%s replacement fabricated and installed."), *ActiveSubsystem));
        }
        else if (Status.state == everward::simulation::FixItExecutionState::Interrupted)
        {
            bReplacementRunning = false;
            bWaitingForResources = true;
            RecordFixItEvent(TEXT("fix_it_replacement_interrupted"), UTF8_TO_TCHAR(Status.detail.c_str()));
        }
    }
}

void AFixItRuntimeActor::ReplanOrStart()
{
    if (BoundCore == nullptr)
    {
        return;
    }

    const auto& Snapshot = BoundCore->snapshot();
    everward::simulation::FixItResources Resources;
    Resources.material_kg = Snapshot.storage_used_kg;
    Resources.energy_j = Snapshot.stored_energy_j;
    Resources.available_time_s = std::numeric_limits<double>::max();
    Resources.fabrication_available = false;
    Resources.upgrade_design_available = false;
    Resources.redesign_design_available = false;
    Resources.evolution_design_available = false;

    const auto Decision = everward::simulation::FixItPlanner::plan_next(
        BoundCore->component_integrity(), Resources);

    if (!Decision.has_value())
    {
        ActiveStage = TEXT("MONITOR");
        ActiveSubsystem = TEXT("ALL");
        ActiveReason = TEXT("All currently repairable Generation-1 subsystems are nominal.");
        bWaitingForResources = false;
        return;
    }

    ActiveStage = FixItStageName(Decision->stage);
    ActiveSubsystem = FixItSubsystemName(Decision->subsystem);
    ActiveReason = UTF8_TO_TCHAR(Decision->reason.c_str());

    const bool bHasMaterial = Snapshot.storage_used_kg + 1e-9 >= Decision->material_required_kg;
    const bool bHasEnergy = Snapshot.stored_energy_j + 1e-6 >= Decision->energy_required_j;

    if (Decision->stage == everward::simulation::FixItStage::Repair)
    {
        if (!bHasMaterial || !bHasEnergy)
        {
            bWaitingForResources = true;
            return;
        }

        RepairExecutor->start(*Decision);
        bRepairRunning = true;
        bWaitingForResources = false;
        RecordFixItEvent(
            TEXT("fix_it_repair_started"),
            FString::Printf(
                TEXT("%s %.0f%% -> %.0f%% // need %.2f kg, %.0f J, %.1f s // %s"),
                *ActiveSubsystem,
                Decision->integrity_before * 100.0,
                Decision->target_integrity * 100.0,
                Decision->material_required_kg,
                Decision->energy_required_j,
                Decision->time_required_s,
                *ActiveReason));
        return;
    }

    if (Decision->stage == everward::simulation::FixItStage::Replacement)
    {
        bWaitingForResources = true;
        ActiveReason = TEXT("Replacement recommended; fabrication capability is not online yet.");
        return;
    }

    // Upgrade / Redesign / Evolution are recommendation stages. Fix_It may
    // surface them but must never invent design knowledge or mutate hardware.
    bWaitingForResources = false;
}

void AFixItRuntimeActor::RefreshStatusSummary()
{
    if (BoundCore == nullptr)
    {
        StatusSummary = TEXT("FIX_IT // WAITING FOR PROBE RUNTIME");
        return;
    }

    if (bRepairRunning && RepairExecutor != nullptr)
    {
        const auto& Status = RepairExecutor->status();
        if (Status.decision.has_value())
        {
            const double Progress = Status.decision->time_required_s > 0.0
                ? FMath::Clamp(Status.elapsed_s / Status.decision->time_required_s, 0.0, 1.0)
                : 0.0;
            StatusSummary = FString::Printf(
                TEXT("FIX_IT // REPAIR %s // %.0f%% // %.2f/%.2f KG // %s"),
                *ActiveSubsystem,
                Progress * 100.0,
                Status.material_consumed_kg,
                Status.decision->material_required_kg,
                *ActiveReason);
            return;
        }
    }

    if (bWaitingForResources)
    {
        everward::simulation::FixItResources Resources;
        const auto& Snapshot = BoundCore->snapshot();
        Resources.material_kg = Snapshot.storage_used_kg;
        Resources.energy_j = Snapshot.stored_energy_j;
        Resources.available_time_s = std::numeric_limits<double>::max();

        const auto Decision = everward::simulation::FixItPlanner::plan_next(
            BoundCore->component_integrity(), Resources);
        if (Decision.has_value())
        {
            StatusSummary = FString::Printf(
                TEXT("FIX_IT // PRIORITY %s // WAITING: %.2f KG + %.0f KJ // STORED %.2f KG // %s"),
                *ActiveSubsystem,
                Decision->material_required_kg,
                Decision->energy_required_j / 1000.0,
                Snapshot.storage_used_kg,
                *ActiveReason);
            return;
        }
    }

    StatusSummary = FString::Printf(
        TEXT("FIX_IT // %s %s // %s"),
        *ActiveStage,
        *ActiveSubsystem,
        ActiveReason.IsEmpty() ? TEXT("Monitoring probe state.") : *ActiveReason);
}

void AFixItRuntimeActor::RecordFixItEvent(const FString& EventName, const FString& Details) const
{
    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return;
    }

    for (TActorIterator<APlaytestRecorderActor> It(World); It; ++It)
    {
        It->RecordPlaytestEvent(EventName, Details);
        return;
    }
}
