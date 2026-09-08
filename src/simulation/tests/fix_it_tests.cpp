#include "everward/simulation/fix_it.hpp"

#undef NDEBUG
#include <cassert>
#include <cstdio>

namespace {

using namespace everward::simulation;

FixItResources ample() {
    return {1000.0, 1.0e9, 1.0e6, false, false, false, false};
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

void test_upgrade_redesign_and_evolution_require_explicit_capability() {
    ComponentIntegritySnapshot integrity;
    auto resources = ample();

    resources.upgrade_design_available = true;
    auto decision = FixItPlanner::plan_next(integrity, resources);
    assert(decision.has_value());
    assert(decision->stage == FixItStage::Upgrade);

    resources.redesign_design_available = true;
    decision = FixItPlanner::plan_next(integrity, resources);
    assert(decision.has_value());
    assert(decision->stage == FixItStage::Redesign);

    resources.evolution_design_available = true;
    decision = FixItPlanner::plan_next(integrity, resources);
    assert(decision.has_value());
    assert(decision->stage == FixItStage::Evolution);
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



void test_executor_consumes_real_resources_and_restores_integrity() {
    DamageAwareProbeRuntime runtime = DamageAwareProbeRuntime::make_canonical_ev0001();
    runtime.set_subsystem_integrity(PowerSubsystem::Sensors, 0.0);
    runtime.add_stored_material_kg(10.0);

    const auto decision = FixItPlanner::plan_next(runtime.component_integrity(), ample());
    assert(decision.has_value());
    assert(decision->subsystem == PowerSubsystem::Sensors);

    const double initial_energy = runtime.snapshot().stored_energy_j;
    const double initial_material = runtime.snapshot().storage_used_kg;

    FixItRepairExecutor executor;
    executor.start(*decision);
    executor.advance(runtime, decision->time_required_s / 2.0);

    assert(executor.status().state == FixItExecutionState::Running);
    assert(runtime.subsystem_integrity(PowerSubsystem::Sensors) > 0.0);
    assert(runtime.subsystem_integrity(PowerSubsystem::Sensors) < decision->target_integrity);
    assert(runtime.snapshot().storage_used_kg < initial_material);
    assert(runtime.snapshot().stored_energy_j < initial_energy);

    executor.advance(runtime, decision->time_required_s / 2.0);
    assert(executor.status().state == FixItExecutionState::Completed);
    assert(runtime.subsystem_integrity(PowerSubsystem::Sensors) == decision->target_integrity);
    assert(executor.status().material_consumed_kg == decision->material_required_kg);
    assert(executor.status().energy_consumed_j == decision->energy_required_j);
}

void test_executor_interrupts_atomically_when_material_is_insufficient() {
    DamageAwareProbeRuntime runtime = DamageAwareProbeRuntime::make_canonical_ev0001();
    runtime.set_subsystem_integrity(PowerSubsystem::Sensors, 0.0);

    auto resources = ample();
    const auto decision = FixItPlanner::plan_next(runtime.component_integrity(), resources);
    assert(decision.has_value());

    const double energy_before = runtime.snapshot().stored_energy_j;
    const double material_before = runtime.snapshot().storage_used_kg;

    FixItRepairExecutor executor;
    executor.start(*decision);
    executor.advance(runtime, decision->time_required_s / 2.0);

    assert(executor.status().state == FixItExecutionState::Interrupted);
    assert(runtime.snapshot().stored_energy_j == energy_before);
    assert(runtime.snapshot().storage_used_kg == material_before);
    assert(runtime.subsystem_integrity(PowerSubsystem::Sensors) == 0.0);
}

void test_executor_interrupts_atomically_when_energy_is_insufficient() {
    DamageAwareProbeRuntime runtime = DamageAwareProbeRuntime::make_canonical_ev0001();
    runtime.set_subsystem_integrity(PowerSubsystem::Sensors, 0.0);
    runtime.add_stored_material_kg(10.0);
    runtime.consume_stored_energy_j(runtime.snapshot().stored_energy_j);

    const auto decision = FixItPlanner::plan_next(runtime.component_integrity(), ample());
    assert(decision.has_value());

    const double material_before = runtime.snapshot().storage_used_kg;

    FixItRepairExecutor executor;
    executor.start(*decision);
    executor.advance(runtime, decision->time_required_s / 2.0);

    assert(executor.status().state == FixItExecutionState::Interrupted);
    assert(runtime.snapshot().storage_used_kg == material_before);
    assert(runtime.snapshot().stored_energy_j == 0.0);
    assert(runtime.subsystem_integrity(PowerSubsystem::Sensors) == 0.0);
}

void test_executor_rejects_non_repair_stage() {
    FixItDecision decision;
    decision.stage = FixItStage::Replacement;
    FixItRepairExecutor executor;
    bool threw = false;
    try {
        executor.start(decision);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    assert(threw);
}



void test_offline_replacement_uses_explicit_fabrication_recipe() {
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
    assert(decision->stage == FixItStage::Replacement);
    assert(decision->subsystem == PowerSubsystem::Sensors);
    assert(decision->target_integrity == 1.0);
    assert(decision->material_required_kg == 20.0);
    assert(decision->energy_required_j == 3.0e6);
    assert(decision->time_required_s == 600.0);
}

void test_replacement_consumes_resources_but_installs_only_when_complete() {
    DamageAwareProbeRuntime runtime = DamageAwareProbeRuntime::make_canonical_ev0001();
    runtime.set_subsystem_integrity(PowerSubsystem::Sensors, 0.0);
    runtime.add_stored_material_kg(25.0);

    auto resources = ample();
    resources.material_kg = 0.0;
    resources.energy_j = 0.0;
    resources.available_time_s = 0.0;
    resources.fabrication_available = true;
    const auto decision = FixItPlanner::plan_next(runtime.component_integrity(), resources);
    assert(decision.has_value());
    assert(decision->stage == FixItStage::Replacement);

    const double initial_energy = runtime.snapshot().stored_energy_j;
    const double initial_material = runtime.snapshot().storage_used_kg;

    FixItReplacementExecutor executor;
    executor.start(*decision, true);
    executor.advance(runtime, decision->time_required_s / 2.0);

    assert(executor.status().state == FixItExecutionState::Running);
    assert(runtime.subsystem_integrity(PowerSubsystem::Sensors) == 0.0);
    assert(runtime.snapshot().stored_energy_j < initial_energy);
    assert(runtime.snapshot().storage_used_kg < initial_material);

    executor.advance(runtime, decision->time_required_s / 2.0);
    assert(executor.status().state == FixItExecutionState::Completed);
    assert(runtime.subsystem_integrity(PowerSubsystem::Sensors) == 1.0);
    assert(executor.status().material_consumed_kg == decision->material_required_kg);
    assert(executor.status().energy_consumed_j == decision->energy_required_j);
}

void test_replacement_requires_fabrication_capability() {
    FixItDecision decision;
    decision.stage = FixItStage::Replacement;
    decision.integrity_before = 0.0;
    decision.target_integrity = 1.0;
    decision.material_required_kg = 1.0;
    decision.energy_required_j = 1.0;
    decision.time_required_s = 1.0;

    FixItReplacementExecutor executor;
    bool threw = false;
    try {
        executor.start(decision, false);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    assert(threw);
}

void test_replacement_interruption_does_not_install_partial_component() {
    DamageAwareProbeRuntime runtime = DamageAwareProbeRuntime::make_canonical_ev0001();
    runtime.set_subsystem_integrity(PowerSubsystem::Sensors, 0.0);
    runtime.add_stored_material_kg(5.0);

    auto resources = ample();
    resources.material_kg = 0.0;
    resources.energy_j = 0.0;
    resources.available_time_s = 0.0;
    resources.fabrication_available = true;
    const auto decision = FixItPlanner::plan_next(runtime.component_integrity(), resources);
    assert(decision.has_value());

    FixItReplacementExecutor executor;
    executor.start(*decision, true);
    executor.advance(runtime, decision->time_required_s / 2.0);

    assert(executor.status().state == FixItExecutionState::Interrupted);
    assert(runtime.subsystem_integrity(PowerSubsystem::Sensors) == 0.0);
}

} // namespace

int main() {
    test_preserves_computation_before_noncritical_repairs();
    test_restores_offline_capability_before_polishing();
    test_improves_weakest_functioning_component_in_bands();
    test_full_repair_waits_without_fabrication();
    test_full_repair_advances_to_replacement_when_fabrication_exists();
    test_upgrade_redesign_and_evolution_require_explicit_capability();
    test_evolution_requires_explicit_design_capability();
    test_offline_component_can_surface_replacement_when_repair_unaffordable();
    test_executor_consumes_real_resources_and_restores_integrity();
    test_executor_interrupts_atomically_when_material_is_insufficient();
    test_executor_interrupts_atomically_when_energy_is_insufficient();
    test_executor_rejects_non_repair_stage();
    test_offline_replacement_uses_explicit_fabrication_recipe();
    test_replacement_consumes_resources_but_installs_only_when_complete();
    test_replacement_requires_fabrication_capability();
    test_replacement_interruption_does_not_install_partial_component();
    std::puts("fix_it_tests: all tests passed");
    return 0;
}


static_assert(everward::simulation::FixItPlanner::canonical_generation1_policy().size() == 4);
