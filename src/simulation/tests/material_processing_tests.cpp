#include "everward/simulation/material_processing.hpp"

#include "everward/simulation/fix_it.hpp"
#include "everward/simulation/manipulator.hpp"

#undef NDEBUG
#include <cassert>
#include <cstdio>

namespace {

using namespace everward::simulation;

ManipulatorRig rig_with_ready_tool() {
    ManipulatorArmState port;
    port.is_deployed = true;
    port.deployment_fraction = 1.0;
    port.tool_attached = true;
    ManipulatorArmState starboard;
    return ManipulatorRig::restore_from_snapshot(port, starboard);
}

ManipulatorRig rig_with_no_tool() {
    return ManipulatorRig::restore_from_snapshot(ManipulatorArmState{}, ManipulatorArmState{});
}

MaterialProcessingRecipe processing_recipe() {
    return MaterialProcessingSystem::canonical_generation1_recipes()[0];
}

MaterialProcessingRecipe fabrication_recipe() {
    return MaterialProcessingSystem::canonical_generation1_recipes()[1];
}

void test_canonical_recipes_conserve_mass_and_declare_their_stage() {
    const auto recipes = MaterialProcessingSystem::canonical_generation1_recipes();
    assert(recipes[0].stage == MaterialProcessingStage::Processing);
    assert(recipes[0].output_kg <= recipes[0].input_kg);
    assert(recipes[1].stage == MaterialProcessingStage::Fabrication);
    assert(recipes[1].output_kg <= recipes[1].input_kg);

    MaterialProcessingSystem system;
    assert(!system.has_fabrication_capability());
    system.register_recipe(recipes[0]);
    assert(!system.has_fabrication_capability());
    system.register_recipe(recipes[1]);
    assert(system.has_fabrication_capability());
}

void test_register_recipe_rejects_mass_creation() {
    MaterialProcessingSystem system;
    MaterialProcessingRecipe recipe = processing_recipe();
    recipe.output_kg = recipe.input_kg + 1.0;
    bool threw = false;
    try {
        system.register_recipe(recipe);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    assert(threw);
}

void test_register_recipe_rejects_duplicate_id() {
    MaterialProcessingSystem system;
    system.register_recipe(processing_recipe());
    bool threw = false;
    try {
        system.register_recipe(processing_recipe());
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    assert(threw);
}

void test_start_rejects_unknown_recipe() {
    MaterialProcessingSystem system;
    DamageAwareProbeRuntime runtime = DamageAwareProbeRuntime::make_canonical_ev0001();
    const auto result = system.start("no_such_recipe", runtime, rig_with_ready_tool());
    assert(!result.accepted);
}

void test_start_rejects_insufficient_material() {
    MaterialProcessingSystem system;
    system.register_recipe(processing_recipe());
    DamageAwareProbeRuntime runtime = DamageAwareProbeRuntime::make_canonical_ev0001();
    runtime.add_stored_material_kg(1.0, "raw_regolith");
    const auto result = system.start("process_regolith_to_alloy", runtime, rig_with_no_tool());
    assert(!result.accepted);
    assert(system.status().state == MaterialProcessingJobState::Idle);
}

void test_start_rejects_insufficient_energy() {
    MaterialProcessingSystem system;
    system.register_recipe(processing_recipe());
    DamageAwareProbeRuntime runtime = DamageAwareProbeRuntime::make_canonical_ev0001();
    runtime.add_stored_material_kg(10.0, "raw_regolith");
    runtime.consume_stored_energy_j(runtime.snapshot().stored_energy_j);
    const auto result = system.start("process_regolith_to_alloy", runtime, rig_with_no_tool());
    assert(!result.accepted);
}

void test_processing_does_not_require_a_deployed_tool() {
    MaterialProcessingSystem system;
    system.register_recipe(processing_recipe());
    DamageAwareProbeRuntime runtime = DamageAwareProbeRuntime::make_canonical_ev0001();
    runtime.add_stored_material_kg(10.0, "raw_regolith");
    const auto result = system.start("process_regolith_to_alloy", runtime, rig_with_no_tool());
    assert(result.accepted);
}

void test_fabrication_requires_a_deployed_tool_with_a_working_position() {
    MaterialProcessingSystem system;
    system.register_recipe(fabrication_recipe());
    DamageAwareProbeRuntime runtime = DamageAwareProbeRuntime::make_canonical_ev0001();
    runtime.add_stored_material_kg(5.0, "structural_alloy");

    const auto rejected = system.start("fabricate_spare_part_mk1", runtime, rig_with_no_tool());
    assert(!rejected.accepted);
    assert(system.status().state == MaterialProcessingJobState::Idle);

    const auto accepted = system.start("fabricate_spare_part_mk1", runtime, rig_with_ready_tool());
    assert(accepted.accepted);
    assert(system.status().state == MaterialProcessingJobState::Running);
}

void test_double_start_is_rejected_while_running() {
    MaterialProcessingSystem system;
    system.register_recipe(processing_recipe());
    DamageAwareProbeRuntime runtime = DamageAwareProbeRuntime::make_canonical_ev0001();
    runtime.add_stored_material_kg(20.0, "raw_regolith");
    assert(system.start("process_regolith_to_alloy", runtime, rig_with_no_tool()).accepted);
    const auto second = system.start("process_regolith_to_alloy", runtime, rig_with_no_tool());
    assert(!second.accepted);
}

void test_advance_rejects_when_not_running() {
    MaterialProcessingSystem system;
    DamageAwareProbeRuntime runtime = DamageAwareProbeRuntime::make_canonical_ev0001();
    bool threw = false;
    try {
        system.advance(runtime, 1.0);
    } catch (const std::logic_error&) {
        threw = true;
    }
    assert(threw);
}

void test_advance_rejects_negative_elapsed_time() {
    MaterialProcessingSystem system;
    system.register_recipe(processing_recipe());
    DamageAwareProbeRuntime runtime = DamageAwareProbeRuntime::make_canonical_ev0001();
    runtime.add_stored_material_kg(10.0, "raw_regolith");
    assert(system.start("process_regolith_to_alloy", runtime, rig_with_no_tool()).accepted);
    bool threw = false;
    try {
        system.advance(runtime, -1.0);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    assert(threw);
}

void test_full_processing_run_consumes_input_and_produces_output_conserving_mass() {
    const auto recipe = processing_recipe();
    MaterialProcessingSystem system;
    system.register_recipe(recipe);
    DamageAwareProbeRuntime runtime = DamageAwareProbeRuntime::make_canonical_ev0001();
    runtime.add_stored_material_kg(recipe.input_kg, "raw_regolith");

    const double initial_energy = runtime.snapshot().stored_energy_j;
    assert(system.start(recipe.recipe_id, runtime, rig_with_no_tool()).accepted);

    system.advance(runtime, recipe.time_required_s / 2.0);
    assert(system.status().state == MaterialProcessingJobState::Running);
    assert(runtime.material_inventory_kg().count("raw_regolith") == 1);
    assert(runtime.material_inventory_kg().at("raw_regolith") > 0.0);
    assert(runtime.material_inventory_kg().count("structural_alloy") == 0);

    system.advance(runtime, recipe.time_required_s / 2.0);
    assert(system.status().state == MaterialProcessingJobState::Completed);
    assert(system.status().material_consumed_kg == recipe.input_kg);
    assert(system.status().energy_consumed_j == recipe.energy_required_j);

    // Raw material is fully consumed and never re-created.
    assert(runtime.material_inventory_kg().count("raw_regolith") == 0);
    const double produced = runtime.material_inventory_kg().at("structural_alloy");
    assert(produced == recipe.output_kg);
    assert(produced < recipe.input_kg);
    assert(initial_energy - runtime.snapshot().stored_energy_j == recipe.energy_required_j);
}

void test_interruption_is_atomic_when_material_is_drained_mid_run() {
    const auto recipe = processing_recipe();
    MaterialProcessingSystem system;
    system.register_recipe(recipe);
    DamageAwareProbeRuntime runtime = DamageAwareProbeRuntime::make_canonical_ev0001();
    runtime.add_stored_material_kg(recipe.input_kg, "raw_regolith");
    assert(system.start(recipe.recipe_id, runtime, rig_with_no_tool()).accepted);

    // Something else (e.g. Fix_It) drains the stockpile out from under the
    // running job between ticks.
    runtime.consume_stored_material_kg("raw_regolith", recipe.input_kg - 0.01);
    const double energy_before = runtime.snapshot().stored_energy_j;

    system.advance(runtime, recipe.time_required_s / 2.0);
    assert(system.status().state == MaterialProcessingJobState::Interrupted);
    // Energy must not have been debited for the step that failed on material.
    assert(runtime.snapshot().stored_energy_j == energy_before);
}

void test_interruption_is_atomic_when_energy_is_drained_mid_run() {
    const auto recipe = processing_recipe();
    MaterialProcessingSystem system;
    system.register_recipe(recipe);
    DamageAwareProbeRuntime runtime = DamageAwareProbeRuntime::make_canonical_ev0001();
    runtime.add_stored_material_kg(recipe.input_kg, "raw_regolith");
    assert(system.start(recipe.recipe_id, runtime, rig_with_no_tool()).accepted);

    runtime.consume_stored_energy_j(runtime.snapshot().stored_energy_j);
    const double material_before = runtime.material_inventory_kg().at("raw_regolith");

    system.advance(runtime, recipe.time_required_s / 2.0);
    assert(system.status().state == MaterialProcessingJobState::Interrupted);
    // Material must not have been debited for the step that failed on energy.
    assert(runtime.material_inventory_kg().at("raw_regolith") == material_before);
}

void test_restore_status_resumes_a_running_job_from_persisted_progress() {
    const auto recipe = processing_recipe();
    MaterialProcessingSystem system;
    system.register_recipe(recipe);
    DamageAwareProbeRuntime runtime = DamageAwareProbeRuntime::make_canonical_ev0001();
    runtime.add_stored_material_kg(recipe.input_kg, "raw_regolith");

    MaterialProcessingJobStatus persisted;
    persisted.state = MaterialProcessingJobState::Running;
    persisted.recipe_id = recipe.recipe_id;
    persisted.stage = recipe.stage;
    persisted.elapsed_s = recipe.time_required_s / 2.0;
    persisted.material_consumed_kg = recipe.input_kg / 2.0;
    persisted.energy_consumed_j = recipe.energy_required_j / 2.0;
    system.restore_status(persisted);
    // The restored job's already-consumed half is reflected in the save, so
    // the probe's remaining stock in this test fixture represents the other
    // half still outstanding.
    runtime.consume_stored_material_kg("raw_regolith", recipe.input_kg / 2.0);

    assert(system.status().state == MaterialProcessingJobState::Running);
    system.advance(runtime, recipe.time_required_s / 2.0);
    assert(system.status().state == MaterialProcessingJobState::Completed);
    assert(runtime.material_inventory_kg().at("structural_alloy") == recipe.output_kg);
}

void test_fabrication_capability_bridges_into_fix_it_replacement_stage() {
    MaterialProcessingSystem system;
    ComponentIntegritySnapshot integrity; // fully restored
    FixItResources resources{1000.0, 1.0e9, 1.0e6, false, false, false, false};

    // With no fabrication recipe known, Fix_It has nothing left to do once
    // every subsystem is restored.
    resources.fabrication_available = system.has_fabrication_capability();
    assert(!FixItPlanner::plan_next(integrity, resources).has_value());

    // Once the probe's material-processing system actually knows a
    // fabrication recipe, the same real capability flag (not a caller
    // constant) unlocks Fix_It's Replacement stage.
    system.register_recipe(fabrication_recipe());
    resources.fabrication_available = system.has_fabrication_capability();
    const auto decision = FixItPlanner::plan_next(integrity, resources);
    assert(decision.has_value());
    assert(decision->stage == FixItStage::Replacement);
}

void test_two_stage_pipeline_processes_then_fabricates_without_creating_mass() {
    const auto processing = processing_recipe();
    const auto fabrication = fabrication_recipe();
    MaterialProcessingSystem system;
    system.register_recipe(processing);
    system.register_recipe(fabrication);

    DamageAwareProbeRuntime runtime = DamageAwareProbeRuntime::make_canonical_ev0001();
    runtime.add_stored_material_kg(processing.input_kg, "raw_regolith");

    assert(system.start(processing.recipe_id, runtime, rig_with_no_tool()).accepted);
    system.advance(runtime, processing.time_required_s);
    assert(system.status().state == MaterialProcessingJobState::Completed);
    assert(runtime.material_inventory_kg().count("raw_regolith") == 0);
    assert(runtime.material_inventory_kg().at("structural_alloy") == processing.output_kg);

    assert(system.start(fabrication.recipe_id, runtime, rig_with_ready_tool()).accepted);
    system.advance(runtime, fabrication.time_required_s);
    assert(system.status().state == MaterialProcessingJobState::Completed);

    const double leftover_alloy = runtime.material_inventory_kg().count("structural_alloy")
        ? runtime.material_inventory_kg().at("structural_alloy")
        : 0.0;
    const double fabricated = runtime.material_inventory_kg().at("fabricated_spare_part_mk1");
    assert(fabricated == fabrication.output_kg);

    // End-to-end conservation: everything that exists after the pipeline
    // (leftover alloy + the fabricated component) can never exceed the raw
    // mass that was originally mined in.
    assert(leftover_alloy + fabricated <= processing.input_kg + 1e-9);
    assert(leftover_alloy + fabricated < processing.input_kg);
}

} // namespace

int main() {
    test_canonical_recipes_conserve_mass_and_declare_their_stage();
    test_register_recipe_rejects_mass_creation();
    test_register_recipe_rejects_duplicate_id();
    test_start_rejects_unknown_recipe();
    test_start_rejects_insufficient_material();
    test_start_rejects_insufficient_energy();
    test_processing_does_not_require_a_deployed_tool();
    test_fabrication_requires_a_deployed_tool_with_a_working_position();
    test_double_start_is_rejected_while_running();
    test_advance_rejects_when_not_running();
    test_advance_rejects_negative_elapsed_time();
    test_full_processing_run_consumes_input_and_produces_output_conserving_mass();
    test_interruption_is_atomic_when_material_is_drained_mid_run();
    test_interruption_is_atomic_when_energy_is_drained_mid_run();
    test_restore_status_resumes_a_running_job_from_persisted_progress();
    test_fabrication_capability_bridges_into_fix_it_replacement_stage();
    test_two_stage_pipeline_processes_then_fabricates_without_creating_mass();
    std::puts("material_processing_tests: all tests passed");
    return 0;
}
