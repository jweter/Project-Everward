#include "everward/simulation/starter_restoration.hpp"

#undef NDEBUG
#include <cassert>
#include <iostream>

using namespace everward::simulation;

int main() {
    // Pristine probe: objective satisfied, nothing pending.
    {
        auto s = evaluate_starter_restoration(ComponentIntegritySnapshot{});
        assert(s.satisfied && s.pending_count == 0 && s.weakest_pending_integrity == 1.0);
    }
    // Damaged awakening (inventory values): all four pending, fixed order.
    {
        ComponentIntegritySnapshot wake{0.35, 0.40, 0.18, 0.22};
        auto s = evaluate_starter_restoration(wake);
        assert(!s.satisfied && s.pending_count == 4);
        assert(s.pending[0] == PowerSubsystem::Sensors);
        assert(s.pending[1] == PowerSubsystem::Propulsion);
        assert(s.pending[2] == PowerSubsystem::Computation);
        assert(s.pending[3] == PowerSubsystem::Thermal);
        assert(s.weakest_pending_integrity == 0.18);
    }
    // One subsystem short blocks the objective.
    {
        auto s = evaluate_starter_restoration(ComponentIntegritySnapshot{1.0, 0.75, 1.0, 1.0});
        assert(!s.satisfied && s.pending_count == 1 && s.pending[0] == PowerSubsystem::Propulsion);
    }
    // Agrees with the departure-gate rule at the tolerance boundary.
    {
        ComponentIntegritySnapshot edge{1.0 - 1e-10, 1.0, 1.0, 1.0};
        assert(evaluate_starter_restoration(edge).satisfied == SelfRepairPlanner::is_fully_restored(edge));
        assert(evaluate_starter_restoration(edge).satisfied);
        ComponentIntegritySnapshot below{1.0 - 1e-6, 1.0, 1.0, 1.0};
        assert(!evaluate_starter_restoration(below).satisfied);
    }
    // Live runtime: repairing via the authoritative model flips the objective;
    // the evaluation itself never mutates state and is repeatable.
    {
        DamageAwareProbeRuntime rt;
        rt.set_subsystem_integrity(PowerSubsystem::Thermal, 0.5);
        auto a = evaluate_starter_restoration(rt);
        auto b = evaluate_starter_restoration(rt);
        assert(!a.satisfied && a.pending_count == b.pending_count);
        assert(rt.subsystem_integrity(PowerSubsystem::Thermal) == 0.5);
        rt.set_subsystem_integrity(PowerSubsystem::Thermal, 1.0);
        assert(evaluate_starter_restoration(rt).satisfied);
    }
    std::cout << "starter_restoration_tests passed\n";
    return 0;
}
