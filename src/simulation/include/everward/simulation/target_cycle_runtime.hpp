#pragma once

#include "everward/simulation/impact_damage.hpp"
#include "everward/simulation/target_selection.hpp"

#include <string>

namespace everward::simulation {

// Slice 7 runtime operation: cycle the authoritative target selection through
// the currently eligible registered bodies without moving ordering/state truth
// into Unreal. Unknown/stale/out-of-range current selections restart at the
// nearest eligible target; no eligible target clears selection fail-closed.
[[nodiscard]] inline TargetSelectionStatus cycle_next_target_selection(
    DamageAwareProbeRuntime& runtime,
    double max_selection_range_m) {
    const TargetSelectionStatus current = runtime.selected_target_status();
    const ProbeStateSnapshot& state = runtime.snapshot();
    const auto next = find_next_selectable_target(
        state.position_m,
        state.velocity_mps,
        runtime.static_bodies(),
        max_selection_range_m,
        current.has_selection ? current.body_id : std::string{});

    if (!next.has_value()) {
        runtime.clear_target_selection();
        return runtime.selected_target_status();
    }

    runtime.select_target(next->body_id);
    return runtime.selected_target_status();
}

} // namespace everward::simulation
