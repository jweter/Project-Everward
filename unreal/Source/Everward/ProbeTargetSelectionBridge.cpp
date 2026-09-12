#include "ProbeSimulationAdapter.h"

#include "everward/simulation/impact_damage.hpp"
#include "everward/simulation/jose_autopilot.hpp"
#include "everward/simulation/target_cycle_runtime.hpp"

#include <string>

namespace
{
EEverwardApproachMotion ToApproachMotion(everward::simulation::ApproachMotionState State)
{
    switch (State)
    {
        case everward::simulation::ApproachMotionState::Closing: return EEverwardApproachMotion::Closing;
        case everward::simulation::ApproachMotionState::Opening: return EEverwardApproachMotion::Opening;
        case everward::simulation::ApproachMotionState::HoldingRange:
        default: return EEverwardApproachMotion::HoldingRange;
    }
}
} // namespace

FEverwardTargetSelectionStatus UProbeSimulationAdapter::GetSelectedTargetStatus() const
{
    FEverwardTargetSelectionStatus Status;
    if (Core == nullptr)
    {
        return Status;
    }

    const everward::simulation::TargetSelectionStatus Selection = Core->selected_target_status();
    Status.bHasSelection = Selection.has_selection;
    Status.TargetId = UTF8_TO_TCHAR(Selection.body_id.c_str());
    Status.SurfaceRangeMeters = Selection.surface_range_m;
    Status.ClosingSpeedMetersPerSecond = Selection.closing_speed_mps;
    Status.ApproachMotion = ToApproachMotion(Selection.approach_motion);
    return Status;
}

FEverwardTargetKnowledgeStatus UProbeSimulationAdapter::GetSelectedTargetKnowledgeStatus() const
{
    // Slice 11 foundation: read-only, recomputed live every call from Core's
    // authoritative target_knowledge (accumulated by SimulationCore's
    // observe_active_scan_progress()), over whichever target
    // GetSelectedTargetStatus() already reports selected -- no second
    // notion of "which target" is introduced.
    FEverwardTargetKnowledgeStatus Status;
    if (Core == nullptr)
    {
        return Status;
    }

    const everward::simulation::TargetSelectionStatus Selection = Core->selected_target_status();
    if (!Selection.has_selection)
    {
        return Status;
    }

    const auto Knowledge = Core->target_knowledge_state(Selection.body_id);
    if (!Knowledge.has_value())
    {
        return Status;
    }

    Status.bHasKnowledge = true;
    switch (Knowledge->level)
    {
        case everward::simulation::KnowledgeLevel::Characterized:
            Status.Level = EEverwardKnowledgeLevel::Characterized;
            break;
        case everward::simulation::KnowledgeLevel::Observed:
            Status.Level = EEverwardKnowledgeLevel::Observed;
            break;
        case everward::simulation::KnowledgeLevel::Unknown:
        default:
            Status.Level = EEverwardKnowledgeLevel::Unknown;
            break;
    }
    Status.Confidence = Knowledge->confidence;
    Status.ActiveScanSeconds = Knowledge->active_scan_s;
    Status.Classification = UTF8_TO_TCHAR(Knowledge->classification.c_str());
    return Status;
}

bool UProbeSimulationAdapter::GetStaticBodyPositionMeters(const FString& BodyId, FVector& OutPositionMeters) const
{
    if (Core == nullptr)
    {
        return false;
    }

    const std::string SimulationBodyId(TCHAR_TO_UTF8(*BodyId));
    for (const everward::simulation::StaticSphereBody& Body : Core->static_bodies())
    {
        if (Body.body_id == SimulationBodyId)
        {
            OutPositionMeters = FVector(Body.center_m.x, Body.center_m.y, Body.center_m.z);
            return true;
        }
    }
    return false;
}

FEverwardJoseGuidanceCommand UProbeSimulationAdapter::GetJoseGuidanceCommand(
    const FString& DestinationBodyId,
    double CruiseSpeedMetersPerSecond,
    double ArrivalSurfaceStandoffMeters,
    double ArrivalToleranceMeters,
    double ApproachGainPerSecond) const
{
    // docs/JOSE_TAKE_THE_WHEEL.md: the guidance decision itself now lives in
    // jose_autopilot.hpp, engine-independent and ctest-covered, following the
    // same pattern GetManipulatorReachStatus()/GetSelectedTargetStatus()
    // already use -- this method only reads Core's live pose/registered-body
    // state and the caller's EditAnywhere-tunable parameters, then maps the
    // pure-function result onto the Blueprint-visible struct. The autopilot
    // controller interprets Outcome and issues the resulting velocity
    // command; it does not compute either itself.
    FEverwardJoseGuidanceCommand Result;
    if (Core == nullptr)
    {
        return Result;
    }

    everward::simulation::JoseAutopilotConfig Config;
    Config.cruise_speed_mps = CruiseSpeedMetersPerSecond;
    Config.arrival_surface_standoff_m = ArrivalSurfaceStandoffMeters;
    Config.arrival_tolerance_m = ArrivalToleranceMeters;
    Config.approach_gain_per_second = ApproachGainPerSecond;

    const std::string SimulationBodyId(TCHAR_TO_UTF8(*DestinationBodyId));
    const auto Guidance = everward::simulation::jose_guidance_command_for_body(
        Core->snapshot().position_m, Core->static_bodies(), SimulationBodyId, Config);
    if (!Guidance.has_value())
    {
        Result.Outcome = EEverwardJoseGuidanceOutcome::DestinationNotFound;
        return Result;
    }

    switch (Guidance->outcome)
    {
        case everward::simulation::JoseGuidanceOutcome::Arrived:
            Result.Outcome = EEverwardJoseGuidanceOutcome::Arrived;
            break;
        case everward::simulation::JoseGuidanceOutcome::DestinationUnresolved:
            Result.Outcome = EEverwardJoseGuidanceOutcome::DestinationUnresolved;
            break;
        case everward::simulation::JoseGuidanceOutcome::Continue:
        default:
            Result.Outcome = EEverwardJoseGuidanceOutcome::Continue;
            break;
    }
    Result.CommandVelocityMetersPerSecond = FVector(
        Guidance->command_velocity_mps.x, Guidance->command_velocity_mps.y, Guidance->command_velocity_mps.z);
    Result.RemainingSurfaceRangeMeters = Guidance->remaining_surface_range_m;
    return Result;
}

FEverwardProbeCommandResult UProbeSimulationAdapter::CommandSelectNearestTarget(double MaxSelectionRangeMeters)
{
    const FName CommandId(TEXT("select_nearest_target"));
    if (Core == nullptr) return RecordCommandResult(CommandId, false, TEXT("simulation unavailable"));

    Core->select_nearest_target(MaxSelectionRangeMeters);
    const everward::simulation::TargetSelectionStatus Selection = Core->selected_target_status();
    if (!Selection.has_selection)
    {
        return RecordCommandResult(CommandId, false,
            FString::Printf(TEXT("no physical target within %.0f m"), MaxSelectionRangeMeters));
    }

    return RecordCommandResult(CommandId, true,
        FString::Printf(TEXT("target selected: %s (%.1f m, closing %.2f m/s)"),
            UTF8_TO_TCHAR(Selection.body_id.c_str()), Selection.surface_range_m, Selection.closing_speed_mps));
}

FEverwardProbeCommandResult UProbeSimulationAdapter::CommandCycleTarget(double MaxSelectionRangeMeters)
{
    const FName CommandId(TEXT("cycle_target"));
    if (Core == nullptr) return RecordCommandResult(CommandId, false, TEXT("simulation unavailable"));

    const everward::simulation::TargetSelectionStatus Selection =
        everward::simulation::cycle_next_target_selection(*Core, MaxSelectionRangeMeters);
    if (!Selection.has_selection)
    {
        return RecordCommandResult(CommandId, false,
            FString::Printf(TEXT("no physical target within %.0f m"), MaxSelectionRangeMeters));
    }

    return RecordCommandResult(CommandId, true,
        FString::Printf(TEXT("target cycled: %s (%.1f m, closing %.2f m/s)"),
            UTF8_TO_TCHAR(Selection.body_id.c_str()), Selection.surface_range_m, Selection.closing_speed_mps));
}

FEverwardProbeCommandResult UProbeSimulationAdapter::CommandSelectTarget(const FString& TargetId)
{
    const FName CommandId(TEXT("select_target"));
    if (Core == nullptr) return RecordCommandResult(CommandId, false, TEXT("simulation unavailable"));

    Core->select_target(std::string(TCHAR_TO_UTF8(*TargetId)));
    const everward::simulation::TargetSelectionStatus Selection = Core->selected_target_status();
    if (!Selection.has_selection)
    {
        return RecordCommandResult(CommandId, false,
            FString::Printf(TEXT("target not currently registered: %s"), *TargetId));
    }

    return RecordCommandResult(CommandId, true,
        FString::Printf(TEXT("target selected: %s"), *TargetId));
}

FEverwardProbeCommandResult UProbeSimulationAdapter::CommandClearTargetSelection()
{
    const FName CommandId(TEXT("clear_target_selection"));
    if (Core == nullptr) return RecordCommandResult(CommandId, false, TEXT("simulation unavailable"));

    Core->clear_target_selection();
    return RecordCommandResult(CommandId, true, TEXT("target selection cleared"));
}
