#include "everward/simulation/fix_it.hpp"

#undef NDEBUG
#include <cassert>
#include <cstdio>

namespace {

using namespace everward::simulation;

FixItResources ample() {
    return {1000.0, 1.0e9, 1.0e6, false, false};
}

void test_preserves_computation_before_noncritical_repairs() {
    ComponentIntegritySnapshot integrity;
    integrity.computation = 0.10;
    integrity.sensors = 0.0;
    const auto decision = FixItPlanner::plan_next(integrity, ample());
    assert(decision.has_value());
    assert(decision->stage == FixItStage::Repair);
    assert(decision->subsystem == PowerSubsystem::Computation);
    assert(decision->target_integrity == 0.25);
}

void test_restores_offline_capability_before_polishing() {
    ComponentIntegritySnapshot integrity;
    integrity.computation = 0.30;
    integrity.thermal = 0.40;
    integrity.sensors = 0.0;
    integrity.propulsion = 0.80;
    const auto decision = FixItPlanner::plan_next(integrity, ample());
    assert(decision.has_value());
    assert(decision->subsystem == PowerSubsystem::Sensors);
    assert(decision->target_integrity == 0.25);
}

void test_improves_weakest_functioning_component_in_bands() {
    ComponentIntegritySnapshot integrity;
    integrity.computation = 0.80;
    integrity.thermal = 0.70;
    integrity.sensors = 0.60;
    integrity.propulsion = 0.40;
    const auto decision = FixItPlanner::plan_next(integrity, ample());
    assert(decision.has_value());
    assert(decision->subsystem == PowerSubsystem::Propulsion);
    assert(decision->target_integrity == 0.50);
}

void test_full_repair_waits_without_fabrication() {
    ComponentIntegritySnapshot integrity;
    const auto decision = FixItPlanner::plan_next(integrity, ample());
    assert(!decision.has_value());
}

void test_full_repair_advances_to_replacement_when_fabrication_exists() {
    ComponentIntegritySnapshot integrity;
    auto resources = ample();
    resources.fabrication_available = true;
    const auto decision = FixItPlanner::plan_next(integrity, resources);
    assert(decision.has_value());
    assert(decision->stage == FixItStage::Replacement);
}

void test_evolution_requires_explicit_design_capability() {
    ComponentIntegritySnapshot integrity;
    auto resources = ample();
    resources.fabrication_available = true;
    resources.evolution_design_available = true;
    const auto decision = FixItPlanner::plan_next(integrity, resources);
    assert(decision.has_value());
    assert(decision->stage == FixItStage::Evolution);
}

void test_offline_component_can_surface_replacement_when_repair_unaffordable() {
    ComponentIntegritySnapshot integrity;
    integrity.computation = 0.40;
    integrity.thermal = 0.40;
    integrity.sensors = 0.0;
    auto resources = ample();
    resources.material_kg = 0.0;
    resources.energy_j = 0.0;
    resources.available_time_s = 0.0;
    resources.fabrication_available = true;
    const auto decision = FixItPlanner::plan_next(integrity, resources);
    assert(decision.has_value());
    assert(decision->subsystem == PowerSubsystem::Sensors);
    assert(decision->stage == FixItStage::Replacement);
}

} // namespace

int main() {
    test_preserves_computation_before_noncritical_repairs();
    test_restores_offline_capability_before_polishing();
    test_improves_weakest_functioning_component_in_bands();
    test_full_repair_waits_without_fabrication();
    test_full_repair_advances_to_replacement_when_fabrication_exists();
    test_evolution_requires_explicit_design_capability();
    test_offline_component_can_surface_replacement_when_repair_unaffordable();
    std::puts("fix_it_tests: all tests passed");
    return 0;
}
