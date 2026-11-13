#pragma once

#include "everward/simulation/objective_cycle.hpp"

#include <cstdint>
#include <map>
#include <string>

namespace everward::simulation {

// Phase 2 first-loop step 9 (docs/FIRST_PLAYABLE_LOOP_EVIDENCE.md): the
// roadmap's "meaningful construction or progression" requirement for the
// first playable loop, closed with the smallest engine-independent rule on
// top of the already-existing repeatable objective
// (objective_cycle.hpp). This header only *reads*
// RestorationObjectiveCycle::completed_cycles -- it never mutates the cycle
// or reimplements any of its completion rules -- and pays its reward through
// the runtime's authoritative storage mutator (add_stored_material_kg), the
// same path mining uses, so storage_used_kg, the capacity limit and the
// per-material inventory move together and the probe stays saveable
// (SimulationCore::restore_from_snapshot rejects an inventory that does not
// sum to storage_used_kg). No processing or
// fabrication model is introduced (Slice 13 stays out of scope): the reward
// is exactly one bounded unit of an already-defined material, available for
// the next cycle (e.g. a future Fix_It repair) to consume like any other
// stored material.
inline constexpr const char* kRestorationRewardMaterialId = "raw_regolith";
inline constexpr double kRestorationRewardKg = 1.0;

struct ProgressionRewardState {
    // Number of restoration-objective-cycle completions this state has
    // already paid a reward for. Always <= the cycle's completed_cycles;
    // advance_progression_reward pays out exactly the difference.
    std::uint32_t rewarded_cycles{0};
};

// Deposits kRestorationRewardKg of kRestorationRewardMaterialId into the
// runtime's storage for every restoration-objective-cycle completion that
// reward_state has not yet rewarded, and returns how many rewards it paid.
// rewarded_cycles advances only after a successful deposit: when storage has
// no room the reward stays owed (no exception, no partial payment) and is
// paid by a later call once room exists. completed_cycles only ever increases
// one at a time in practice (advance_restoration_cycle completes at most
// one cycle per call), but this loop stays correct even if reward_state was
// restored from a save captured behind the cycle's own completed_cycles.
// Safe to call every tick: once rewarded_cycles catches up to
// cycle.completed_cycles, it is a no-op. A cycle rejected while Active
// (trigger_next_restoration_cycle returning false) never increments
// completed_cycles, so it is never rewarded either.
//
// Runtime is any probe runtime exposing snapshot() and
// add_stored_material_kg (SimulationCore, ProbeRuntime,
// DamageAwareProbeRuntime).
template <typename Runtime>
inline std::uint32_t advance_progression_reward(
        ProgressionRewardState& reward_state,
        const RestorationObjectiveCycle& cycle,
        Runtime& runtime) {
    std::uint32_t paid = 0;
    while (reward_state.rewarded_cycles < cycle.completed_cycles) {
        const auto& probe = runtime.snapshot();
        // Same tolerance as add_stored_material_kg's own capacity check.
        if (probe.storage_used_kg + kRestorationRewardKg > probe.storage_capacity_kg + 1e-6) {
            break;
        }
        runtime.add_stored_material_kg(kRestorationRewardKg, kRestorationRewardMaterialId);
        reward_state.rewarded_cycles += 1;
        paid += 1;
    }
    return paid;
}

}  // namespace everward::simulation
