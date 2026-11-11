#pragma once

#include "everward/simulation/starter_restoration.hpp"

#include <cstdint>

namespace everward::simulation {

// First-playable-loop step 9 (docs/FIRST_PLAYABLE_LOOP_EVIDENCE.md): the
// smallest engine-independent repeatable objective that follows starter
// restoration. Starter restoration (starter_restoration.hpp) is a read-only,
// one-time status derived from component integrity; this adds the missing
// *repeat* mechanic on top of it without inventing a new damage/repair model
// -- completion still flows entirely through the existing
// SelfRepairPlanner::is_fully_restored rule that evaluate_starter_restoration
// already wraps. It owns no runtime mutation and no fabrication (Slice 13
// stays out of scope). Presentation layers may trigger and observe this
// state; they are never its authority.
enum class RestorationObjectiveCycleState {
    Idle,
    Active,
    Completed,
};

struct RestorationObjectiveCycle {
    RestorationObjectiveCycleState state{RestorationObjectiveCycleState::Idle};
    std::uint32_t completed_cycles{0};
    // Latches true the first time this Active cycle observes the probe NOT
    // fully restored. Required before the cycle is allowed to complete, so
    // triggering a fresh cycle while already restored -- with no further
    // damage ever occurring -- cannot trivially "complete" it.
    bool damage_observed_this_cycle{false};
};

// Starts the next repeatable objective cycle. A no-op (returns false) while
// a cycle is already Active -- a cycle cannot be re-triggered out from under
// itself. Otherwise (Idle, i.e. never triggered, or a prior cycle
// Completed) begins a fresh Active cycle and clears the damage-observed
// latch, so the same cycle can be triggered and completed any number of
// times.
inline bool trigger_next_restoration_cycle(RestorationObjectiveCycle& cycle) {
    if (cycle.state == RestorationObjectiveCycleState::Active) {
        return false;
    }
    cycle.state = RestorationObjectiveCycleState::Active;
    cycle.damage_observed_this_cycle = false;
    return true;
}

// Advances an Active cycle from the latest authoritative starter-restoration
// status. A no-op outside the Active state (nothing to trigger from, or
// already Completed and awaiting the next trigger). Completes the instant
// the probe is reported fully restored again, but only once this same cycle
// has first observed it NOT fully restored.
inline void advance_restoration_cycle(
        RestorationObjectiveCycle& cycle, const StarterRestorationStatus& status) {
    if (cycle.state != RestorationObjectiveCycleState::Active) {
        return;
    }
    if (!status.satisfied) {
        cycle.damage_observed_this_cycle = true;
        return;
    }
    if (cycle.damage_observed_this_cycle) {
        cycle.state = RestorationObjectiveCycleState::Completed;
        cycle.completed_cycles += 1;
    }
}

inline void advance_restoration_cycle(
        RestorationObjectiveCycle& cycle, const DamageAwareProbeRuntime& runtime) {
    advance_restoration_cycle(cycle, evaluate_starter_restoration(runtime));
}

}  // namespace everward::simulation
