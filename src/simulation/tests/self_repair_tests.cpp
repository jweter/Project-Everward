#include "everward/simulation/self_repair.hpp"

#undef NDEBUG
#include <cassert>
#include <iostream>
#include <stdexcept>

using namespace everward::simulation;

static const SelfRepairResources kPlenty{100.0, 1e9};

int main() {
    // Fully restored probe: nothing to do, departure gate satisfied.
    {
        ComponentIntegritySnapshot full;
        assert(!SelfRepairPlanner::next_step(full, kPlenty).has_value());
        assert(SelfRepairPlanner::is_fully_restored(full));
    }

    // Priority 1: a sub-minimum core outranks an offline sensor suite.
    {
        ComponentIntegritySnapshot s{0.0, 0.0, 0.08, 0.14};
        auto step = SelfRepairPlanner::next_step(s, kPlenty);
        assert(step && step->subsystem == PowerSubsystem::Computation);
        assert(step->priority == SelfRepairPriority::PreserveExistence);
        assert(step->integrity_after == 0.25);
    }

    // Capability before perfection: with core/thermal minimally up, an
    // offline system is restored before a working one is polished.
    {
        ComponentIntegritySnapshot s{0.0, 0.0, 0.30, 0.90};
        auto step = SelfRepairPlanner::next_step(s, kPlenty);
        assert(step && step->subsystem == PowerSubsystem::Sensors);  // importance breaks 0/0 tie
        assert(step->priority == SelfRepairPriority::RestoreOffline);
        assert(step->integrity_before == 0.0 && step->integrity_after == 0.25);
        assert(std::fabs(step->material_kg - 1.0) < 1e-9);
        assert(std::fabs(step->energy_j - 1'250'000.0) < 1e-6);
        assert(std::fabs(step->duration_s - 150.0) < 1e-9);
    }

    // Everything online: weakest system first, staged to the next band.
    {
        ComponentIntegritySnapshot s{0.60, 0.30, 0.90, 0.80};
        auto step = SelfRepairPlanner::next_step(s, kPlenty);
        assert(step && step->subsystem == PowerSubsystem::Propulsion);
        assert(step->priority == SelfRepairPriority::ReassessWeakest);
        assert(step->integrity_after == 0.50);
    }

    // Unaffordable step reports shortfalls rather than substituting.
    {
        ComponentIntegritySnapshot s{1.0, 1.0, 1.0, 0.0};
        auto step = SelfRepairPlanner::next_step(s, {0.25, 1'000'000.0});
        assert(step && step->subsystem == PowerSubsystem::Thermal && !step->affordable);
        assert(std::fabs(step->material_shortfall_kg - 0.75) < 1e-9);
        assert(std::fabs(step->energy_shortfall_j - 250'000.0) < 1e-6);
    }

    // Reassessment loop from a damaged awakening reaches 100% with a valid
    // recovery path, consuming exactly the summed step costs, and never
    // repairs a subsystem past its priority (no soft-lock, no regression).
    {
        ImpactDamageModel model = ImpactDamageModel::restore_from_snapshot({0.0, 0.0, 0.08, 0.14}, false, 0);
        double material = 0.0, energy = 0.0;
        int steps = 0;
        while (auto step = SelfRepairPlanner::next_step(model.integrity(), kPlenty)) {
            assert(step->integrity_after > step->integrity_before);
            material += step->material_kg;
            energy += step->energy_j;
            SelfRepairPlanner::apply(model, *step);
            assert(++steps < 100);
        }
        assert(SelfRepairPlanner::is_fully_restored(model.integrity()));
        // total delta = (1-0)+(1-0)+(1-0.08)+(1-0.14) = 3.78 full repairs
        assert(std::fabs(material - 3.78 * SelfRepairPlanner::kMaterialPerFullRepairKg) < 1e-9);
        assert(std::fabs(energy - 3.78 * SelfRepairPlanner::kEnergyPerFullRepairJ) < 1e-3);
    }

    // apply() never lowers integrity; invalid input is rejected.
    {
        ImpactDamageModel model = ImpactDamageModel::restore_from_snapshot({0.9, 1.0, 1.0, 1.0}, false, 0);
        SelfRepairStep stale;
        stale.subsystem = PowerSubsystem::Sensors;
        stale.integrity_after = 0.5;
        SelfRepairPlanner::apply(model, stale);
        assert(model.subsystem_integrity(PowerSubsystem::Sensors) == 0.9);

        bool threw = false;
        try { (void)SelfRepairPlanner::next_step({}, {-1.0, 0.0}); } catch (const std::invalid_argument&) { threw = true; }
        assert(threw);
        threw = false;
        try { (void)SelfRepairPlanner::next_step({1.5, 1, 1, 1}, kPlenty); } catch (const std::invalid_argument&) { threw = true; }
        assert(threw);
    }

    std::cout << "self_repair_tests passed\n";
    return 0;
}
