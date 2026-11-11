#include "everward/simulation/objective_cycle.hpp"

#undef NDEBUG
#include <cassert>
#include <iostream>

using namespace everward::simulation;

namespace {

// Drives one trigger-damage-repair-complete pass through a cycle using the
// live runtime's test-only integrity setter, matching
// starter_restoration_tests.cpp's precedent for exercising the authoritative
// evaluation without going through the full impact/self-repair pipeline.
void run_one_cycle(RestorationObjectiveCycle& cycle, DamageAwareProbeRuntime& rt) {
    assert(trigger_next_restoration_cycle(cycle));
    assert(cycle.state == RestorationObjectiveCycleState::Active);

    rt.set_subsystem_integrity(PowerSubsystem::Propulsion, 0.4);
    advance_restoration_cycle(cycle, rt);
    assert(cycle.state == RestorationObjectiveCycleState::Active);
    assert(cycle.damage_observed_this_cycle);

    rt.set_subsystem_integrity(PowerSubsystem::Propulsion, 1.0);
    advance_restoration_cycle(cycle, rt);
    assert(cycle.state == RestorationObjectiveCycleState::Completed);
}

}  // namespace

int main() {
    // A freshly triggered cycle starts Active with no damage observed yet.
    {
        RestorationObjectiveCycle cycle;
        assert(cycle.state == RestorationObjectiveCycleState::Idle);
        assert(trigger_next_restoration_cycle(cycle));
        assert(cycle.state == RestorationObjectiveCycleState::Active);
        assert(cycle.completed_cycles == 0);
        assert(!cycle.damage_observed_this_cycle);
    }
    // Re-triggering while already Active is rejected; the cycle is unchanged.
    {
        RestorationObjectiveCycle cycle;
        assert(trigger_next_restoration_cycle(cycle));
        assert(!trigger_next_restoration_cycle(cycle));
        assert(cycle.state == RestorationObjectiveCycleState::Active);
    }
    // Reaching "fully restored" without ever having been observed damaged
    // during this Active cycle does not complete it -- a trivial re-trigger
    // while already restored, with no further damage, must not auto-complete.
    {
        DamageAwareProbeRuntime rt;  // pristine: evaluate_starter_restoration(rt).satisfied
        RestorationObjectiveCycle cycle;
        assert(trigger_next_restoration_cycle(cycle));
        advance_restoration_cycle(cycle, rt);
        assert(cycle.state == RestorationObjectiveCycleState::Active);
        assert(cycle.completed_cycles == 0);
    }
    // The documented acceptance: a first cycle can be triggered and
    // completed from restored simulation state, and a *second* cycle can
    // then also be triggered and completed -- the objective repeats.
    {
        DamageAwareProbeRuntime rt;
        assert(evaluate_starter_restoration(rt).satisfied);
        RestorationObjectiveCycle cycle;

        run_one_cycle(cycle, rt);
        assert(cycle.completed_cycles == 1);

        run_one_cycle(cycle, rt);
        assert(cycle.completed_cycles == 2);
    }
    // Determinism: replaying the exact same sequence of triggers/damage/
    // repair from the same starting state yields an identical final cycle.
    {
        DamageAwareProbeRuntime rt_a;
        DamageAwareProbeRuntime rt_b;
        RestorationObjectiveCycle cycle_a;
        RestorationObjectiveCycle cycle_b;

        run_one_cycle(cycle_a, rt_a);
        run_one_cycle(cycle_a, rt_a);
        run_one_cycle(cycle_b, rt_b);
        run_one_cycle(cycle_b, rt_b);

        assert(cycle_a.state == cycle_b.state);
        assert(cycle_a.completed_cycles == cycle_b.completed_cycles);
        assert(cycle_a.damage_observed_this_cycle == cycle_b.damage_observed_this_cycle);
    }
    // advance_restoration_cycle is a no-op outside the Active state (Idle,
    // or Completed awaiting its next trigger).
    {
        DamageAwareProbeRuntime damaged_rt;
        damaged_rt.set_subsystem_integrity(PowerSubsystem::Sensors, 0.2);
        RestorationObjectiveCycle idle_cycle;
        advance_restoration_cycle(idle_cycle, damaged_rt);
        assert(idle_cycle.state == RestorationObjectiveCycleState::Idle);

        DamageAwareProbeRuntime restored_rt;
        RestorationObjectiveCycle completed_cycle;
        run_one_cycle(completed_cycle, restored_rt);  // leaves restored_rt restored, cycle Completed
        advance_restoration_cycle(completed_cycle, restored_rt);
        assert(completed_cycle.state == RestorationObjectiveCycleState::Completed);
        assert(completed_cycle.completed_cycles == 1);
    }
    std::cout << "objective_cycle_tests passed\n";
    return 0;
}
