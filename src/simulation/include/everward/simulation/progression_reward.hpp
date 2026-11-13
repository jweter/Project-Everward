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
// or reimplements any of its completion rules -- and pays its reward into
// the same std::map<std::string, double> material-inventory convention
// material_consumption.hpp's plan_material_consumption and
// ProbeStateSnapshot::material_inventory_kg already use. No processing or
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

// Pays kRestorationRewardKg of kRestorationRewardMaterialId into
// inventory_kg for every restoration-objective-cycle completion that
// reward_state has not yet rewarded. completed_cycles only ever increases
// one at a time in practice (advance_restoration_cycle completes at most
// one cycle per call), but this loop stays correct even if reward_state was
// restored from a save captured behind the cycle's own completed_cycles.
// Safe to call every tick: once rewarded_cycles catches up to
// cycle.completed_cycles, it is a no-op. A cycle rejected while Active
// (trigger_next_restoration_cycle returning false) never increments
// completed_cycles, so it is never rewarded either.
inline void advance_progression_reward(
        ProgressionRewardState& reward_state,
        const RestorationObjectiveCycle& cycle,
        std::map<std::string, double>& inventory_kg) {
    while (reward_state.rewarded_cycles < cycle.completed_cycles) {
        inventory_kg[kRestorationRewardMaterialId] += kRestorationRewardKg;
        reward_state.rewarded_cycles += 1;
    }
}

}  // namespace everward::simulation
