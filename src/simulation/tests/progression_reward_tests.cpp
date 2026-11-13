#include "everward/simulation/progression_reward.hpp"

#undef NDEBUG
#include <cassert>
#include <iostream>
#include <map>

using namespace everward::simulation;

namespace {

// Drives one trigger-damage-repair-complete pass through a cycle, matching
// objective_cycle_tests.cpp's precedent for exercising the cycle via its
// own rules rather than mutating completed_cycles directly.
void run_one_cycle(RestorationObjectiveCycle& cycle, DamageAwareProbeRuntime& rt) {
    assert(trigger_next_restoration_cycle(cycle));
    rt.set_subsystem_integrity(PowerSubsystem::Propulsion, 0.4);
    advance_restoration_cycle(cycle, rt);
    assert(cycle.damage_observed_this_cycle);
    rt.set_subsystem_integrity(PowerSubsystem::Propulsion, 1.0);
    advance_restoration_cycle(cycle, rt);
    assert(cycle.state == RestorationObjectiveCycleState::Completed);
}

}  // namespace

int main() {
    // No reward before the first completion.
    {
        RestorationObjectiveCycle cycle;
        ProgressionRewardState reward_state;
        std::map<std::string, double> inventory_kg;

        assert(trigger_next_restoration_cycle(cycle));
        advance_progression_reward(reward_state, cycle, inventory_kg);
        assert(inventory_kg.empty());
        assert(reward_state.rewarded_cycles == 0);
    }
    // Exactly one reward per completed cycle.
    {
        DamageAwareProbeRuntime rt;
        assert(evaluate_starter_restoration(rt).satisfied);
        RestorationObjectiveCycle cycle;
        ProgressionRewardState reward_state;
        std::map<std::string, double> inventory_kg;

        run_one_cycle(cycle, rt);
        assert(cycle.completed_cycles == 1);
        advance_progression_reward(reward_state, cycle, inventory_kg);
        assert(reward_state.rewarded_cycles == 1);
        assert(inventory_kg.at(kRestorationRewardMaterialId) == kRestorationRewardKg);

        // Calling again before the next completion must not pay twice.
        advance_progression_reward(reward_state, cycle, inventory_kg);
        assert(reward_state.rewarded_cycles == 1);
        assert(inventory_kg.at(kRestorationRewardMaterialId) == kRestorationRewardKg);

        run_one_cycle(cycle, rt);
        assert(cycle.completed_cycles == 2);
        advance_progression_reward(reward_state, cycle, inventory_kg);
        assert(reward_state.rewarded_cycles == 2);
        assert(inventory_kg.at(kRestorationRewardMaterialId) == 2 * kRestorationRewardKg);
    }
    // No reward for a re-trigger rejected while Active: completed_cycles
    // (and therefore the reward) must stay unchanged.
    {
        RestorationObjectiveCycle cycle;
        ProgressionRewardState reward_state;
        std::map<std::string, double> inventory_kg;

        assert(trigger_next_restoration_cycle(cycle));
        assert(!trigger_next_restoration_cycle(cycle));  // rejected: already Active
        advance_progression_reward(reward_state, cycle, inventory_kg);
        assert(cycle.completed_cycles == 0);
        assert(reward_state.rewarded_cycles == 0);
        assert(inventory_kg.empty());
    }
    // advance_progression_reward never mutates the cycle itself.
    {
        DamageAwareProbeRuntime rt;
        RestorationObjectiveCycle cycle;
        ProgressionRewardState reward_state;
        std::map<std::string, double> inventory_kg;

        run_one_cycle(cycle, rt);
        const RestorationObjectiveCycleState state_before = cycle.state;
        const std::uint32_t completed_before = cycle.completed_cycles;
        const bool latch_before = cycle.damage_observed_this_cycle;

        advance_progression_reward(reward_state, cycle, inventory_kg);

        assert(cycle.state == state_before);
        assert(cycle.completed_cycles == completed_before);
        assert(cycle.damage_observed_this_cycle == latch_before);
    }
    std::cout << "progression_reward_tests passed\n";
    return 0;
}
