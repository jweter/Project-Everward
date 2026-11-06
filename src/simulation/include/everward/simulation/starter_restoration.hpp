#pragma once

#include "everward/simulation/impact_damage.hpp"
#include "everward/simulation/self_repair.hpp"

#include <array>
#include <cstddef>

namespace everward::simulation {

// Read-only, engine-independent starter-restoration objective status
// (docs/FIRST_PLAYABLE_LOOP_INVENTORY.md, "Smallest missing loop dependency").
// Derived purely from authoritative component integrity using the existing
// SelfRepairPlanner::is_fully_restored departure-gate rule; it owns no state,
// mutates nothing, and presentation layers may only mirror it.
//
// Scope: the four integrity-tracked subsystems. Power/thermal stability from
// STARTER_AWAKENING_AND_SELF_REPAIR.md is not part of this first objective.
struct StarterRestorationStatus {
    bool satisfied{false};
    // Subsystems still below 100% (by the is_fully_restored tolerance), in
    // fixed order sensors, propulsion, computation, thermal.
    std::array<PowerSubsystem, 4> pending{};
    std::size_t pending_count{0};
    // Authoritative integrity of the weakest pending subsystem (1.0 if none).
    double weakest_pending_integrity{1.0};
};

[[nodiscard]] inline StarterRestorationStatus evaluate_starter_restoration(
        const ComponentIntegritySnapshot& integrity) {
    StarterRestorationStatus status;
    const std::array<std::pair<PowerSubsystem, double>, 4> entries{{
        {PowerSubsystem::Sensors, integrity.sensors},
        {PowerSubsystem::Propulsion, integrity.propulsion},
        {PowerSubsystem::Computation, integrity.computation},
        {PowerSubsystem::Thermal, integrity.thermal},
    }};
    for (const auto& [subsystem, value] : entries) {
        ComponentIntegritySnapshot probe;  // all 1.0
        switch (subsystem) {
            case PowerSubsystem::Sensors: probe.sensors = value; break;
            case PowerSubsystem::Propulsion: probe.propulsion = value; break;
            case PowerSubsystem::Computation: probe.computation = value; break;
            case PowerSubsystem::Thermal: probe.thermal = value; break;
        }
        if (!SelfRepairPlanner::is_fully_restored(probe)) {
            status.pending[status.pending_count++] = subsystem;
            if (value < status.weakest_pending_integrity) {
                status.weakest_pending_integrity = value;
            }
        }
    }
    status.satisfied = SelfRepairPlanner::is_fully_restored(integrity);
    return status;
}

[[nodiscard]] inline StarterRestorationStatus evaluate_starter_restoration(
        const DamageAwareProbeRuntime& runtime) {
    return evaluate_starter_restoration(runtime.component_integrity());
}

}  // namespace everward::simulation
