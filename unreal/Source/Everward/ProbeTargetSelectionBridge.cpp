#include "ProbeSimulationAdapter.h"

#include "everward/simulation/impact_damage.hpp"

#include <string>

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
    return Status;
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
