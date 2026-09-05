#include "ProbeSimulationAdapter.h"

#include "EverwardPhase2TestEnvironment.h"
#include "everward/simulation/impact_damage.hpp"
#include "everward/simulation/manipulator.hpp"
#include "everward/simulation/mining.hpp"

#include <string>

namespace
{
constexpr double BootstrapExtractionKilogramsPerCycle = 5.0;

std::string BootstrapTargetIdUtf8()
{
    return std::string(TCHAR_TO_UTF8(AEverwardPhase2TestEnvironment::BootstrapScanTargetId));
}

// The bootstrap deposit's mining-relevant position must track wherever the
// registered body actually is right now, not where it spawned: Slice 7's
// manipulator move mechanic (ProbeRuntime::update_static_sphere_body_position(),
// see PHASE2_MANIPULATOR_MOVE_TEST.md) can relocate this exact registered
// body while an arm grasps and carries it, and the visible mesh/label
// already mirror that live position every tick
// (AEverwardPhase2TestEnvironment::RefreshScanTargetPosition() reads it back
// through GetStaticBodyPositionMeters()). Reusing the original spawn-time
// constants here instead would compute the tool-tip surface gap against a
// location the mesh no longer occupies once the target has been carried
// away from its bootstrap position. The spawn-time constants remain only as
// a defensive fallback: the bootstrap body is always registered in
// UProbeSimulationAdapter::BeginPlay(), so the "not found" branch should not
// be reachable in practice.
everward::simulation::Vector3d LiveBootstrapBodyCenterMeters(
    const everward::simulation::DamageAwareProbeRuntime& Core,
    const std::string& TargetId)
{
    for (const everward::simulation::StaticSphereBody& Body : Core.static_bodies())
    {
        if (Body.body_id == TargetId)
        {
            return Body.center_m;
        }
    }
    return {
        AEverwardPhase2TestEnvironment::BootstrapBodyCenterXMeters,
        AEverwardPhase2TestEnvironment::BootstrapBodyCenterYMeters,
        AEverwardPhase2TestEnvironment::BootstrapBodyCenterZMeters,
    };
}

everward::simulation::ResourceDeposit MakeBootstrapDeposit(
    double RemainingKilograms,
    everward::simulation::Vector3d LiveCenterMeters)
{
    everward::simulation::ResourceDeposit Deposit;
    Deposit.body.body_id = BootstrapTargetIdUtf8();
    Deposit.body.center_m = LiveCenterMeters;
    Deposit.body.radius_m = AEverwardPhase2TestEnvironment::BootstrapBodyRadiusMeters;
    Deposit.material_id = "iron_bearing_silicate_regolith";
    Deposit.display_name = "Iron-bearing silicate regolith";
    Deposit.remaining_kg = RemainingKilograms;
    Deposit.extraction_kg_per_cycle = BootstrapExtractionKilogramsPerCycle;
    return Deposit;
}
}

FEverwardMiningStatus UProbeSimulationAdapter::GetMiningStatus() const
{
    FEverwardMiningStatus Status;
    Status.TargetId = AEverwardPhase2TestEnvironment::BootstrapScanTargetId;
    Status.MaterialName = TEXT("Iron-bearing silicate regolith");
    Status.bSurveyed = bBootstrapResourceSurveyed ||
        (LastScanLifecycleNotice.bCompleted &&
         LastScanLifecycleNotice.Detail.Contains(AEverwardPhase2TestEnvironment::BootstrapScanTargetId));
    Status.DepositRemainingKilograms = BootstrapDepositRemainingKilograms;
    Status.ExtractedMaterialKilograms = BootstrapExtractedMaterialKilograms;
    Status.ExtractionKilogramsPerCycle = BootstrapExtractionKilogramsPerCycle;
    Status.ToolWorkingReachMeters = everward::simulation::MiningSystem::kGeneration1ToolWorkingReachM;
    return Status;
}

FEverwardProbeCommandResult UProbeSimulationAdapter::CommandMineBootstrapTarget()
{
    const FName CommandId(TEXT("mine_bootstrap_target"));
    if (Core == nullptr || Manipulators == nullptr)
    {
        return RecordCommandResult(CommandId, false, TEXT("simulation or manipulator unavailable"));
    }

    if (!bBootstrapResourceSurveyed &&
        LastScanLifecycleNotice.bCompleted &&
        LastScanLifecycleNotice.Detail.Contains(AEverwardPhase2TestEnvironment::BootstrapScanTargetId))
    {
        bBootstrapResourceSurveyed = true;
    }

    everward::simulation::MiningSystem Mining;
    const std::string TargetId = BootstrapTargetIdUtf8();
    Mining.add_deposit(MakeBootstrapDeposit(
        BootstrapDepositRemainingKilograms, LiveBootstrapBodyCenterMeters(*Core, TargetId)));
    if (bBootstrapResourceSurveyed)
    {
        Mining.mark_surveyed(TargetId);
    }

    const auto& Snapshot = Core->snapshot();
    const everward::simulation::MiningAttemptResult Result = Mining.mine_once(
        TargetId,
        *Manipulators,
        everward::simulation::ProbeWorldPose{Snapshot.position_m, Snapshot.attitude_degrees},
        Snapshot.storage_used_kg,
        Snapshot.storage_capacity_kg);

    if (Result.accepted)
    {
        // storage_used_kg is the authoritative field the always-visible HUD's
        // STORAGE readout reads (see ProbeSimulationAdapter.cpp's telemetry
        // conversion); it must move here or mining never appears in that
        // readout even though this mining status widget shows it moving.
        Core->add_stored_material_kg(Result.extracted_kg);
        BootstrapExtractedMaterialKilograms += Result.extracted_kg;
        BootstrapDepositRemainingKilograms = Result.remaining_deposit_kg;
    }

    LastAutomationNotice.Sequence = ++AutomationSequence;
    LastAutomationNotice.bRejected = !Result.accepted;
    LastAutomationNotice.Detail = UTF8_TO_TCHAR(Result.detail.c_str());

    return RecordCommandResult(CommandId, Result.accepted, UTF8_TO_TCHAR(Result.detail.c_str()));
}
