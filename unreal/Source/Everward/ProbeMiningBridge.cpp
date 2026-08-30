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

everward::simulation::ResourceDeposit MakeBootstrapDeposit(double RemainingKilograms)
{
    everward::simulation::ResourceDeposit Deposit;
    Deposit.body.body_id = BootstrapTargetIdUtf8();
    Deposit.body.center_m = {
        AEverwardPhase2TestEnvironment::BootstrapBodyCenterXMeters,
        AEverwardPhase2TestEnvironment::BootstrapBodyCenterYMeters,
        AEverwardPhase2TestEnvironment::BootstrapBodyCenterZMeters,
    };
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
    Mining.add_deposit(MakeBootstrapDeposit(BootstrapDepositRemainingKilograms));
    if (bBootstrapResourceSurveyed)
    {
        Mining.mark_surveyed(TargetId);
    }

    const auto& Snapshot = Core->snapshot();
    const everward::simulation::MiningAttemptResult Result = Mining.mine_once(
        TargetId,
        *Manipulators,
        everward::simulation::ProbeWorldPose{Snapshot.position_m, Snapshot.attitude_degrees},
        Snapshot.storage_used_kg + BootstrapExtractedMaterialKilograms,
        Snapshot.storage_capacity_kg);

    if (Result.accepted)
    {
        BootstrapExtractedMaterialKilograms += Result.extracted_kg;
        BootstrapDepositRemainingKilograms = Result.remaining_deposit_kg;
    }

    LastAutomationNotice.Sequence = ++AutomationSequence;
    LastAutomationNotice.bRejected = !Result.accepted;
    LastAutomationNotice.Detail = UTF8_TO_TCHAR(Result.detail.c_str());

    return RecordCommandResult(CommandId, Result.accepted, UTF8_TO_TCHAR(Result.detail.c_str()));
}
