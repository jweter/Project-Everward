#include "everward/simulation/recovery_goal.hpp"

#undef NDEBUG
#include <cassert>
#include <cmath>
#include <cstdio>
#include <optional>
#include <string>

namespace {
using namespace everward::simulation;

FixItResources resources(double material_kg = 0.0, double energy_j = 0.0) {
    return {material_kg, energy_j, 1.0e6, false, false, false, false};
}

ComponentIntegritySnapshot awakening() {
    ComponentIntegritySnapshot i{};
    i.computation = 0.18;
    i.thermal = 0.22;
    i.sensors = 0.35;
    i.propulsion = 0.40;
    return i;
}

void set_integrity(ComponentIntegritySnapshot& i, PowerSubsystem subsystem, double value) {
    switch (subsystem) {
        case PowerSubsystem::Sensors: i.sensors = value; break;
        case PowerSubsystem::Propulsion: i.propulsion = value; break;
        case PowerSubsystem::Computation: i.computation = value; break;
        case PowerSubsystem::Thermal: i.thermal = value; break;
    }
}

void test_awakening_survival_first_goal_and_true_cost() {
    const auto i = awakening();
    const auto d = FixItPlanner::plan_next(i, resources());
    assert(d.has_value());
    assert(d->subsystem == PowerSubsystem::Computation);
    assert(std::abs(d->target_integrity - 0.25) < 1e-9);
    const auto g = resolve_recovery_goal(i, resources(), d);
    assert(g.has_goal);
    assert(g.state == RecoveryGoalState::Blocked);
    assert(g.goal_id == "fix_it:repair:computation:250000");
    assert(g.reason == d->reason);
    assert(std::abs(g.missing_material_kg - d->material_required_kg) < 1e-9);
    assert(std::abs(g.missing_energy_j - d->energy_required_j) < 1e-5);
    assert(g.progress_fraction == 0.0);
}

void test_stable_id_across_inventory_and_partial_recovery() {
    auto i = awakening();
    const auto d = FixItPlanner::plan_next(i, resources());
    const auto before = resolve_recovery_goal(i, resources(), d);
    const auto after_collection = resolve_recovery_goal(i, resources(10.0, 1e8), d);
    assert(after_collection.state == RecoveryGoalState::Ready);
    assert(before.goal_id == after_collection.goal_id);
    i.computation = 0.21;
    const auto mid = resolve_recovery_goal(i, resources(10.0, 1e8), d);
    assert(mid.goal_id == before.goal_id);
    assert(mid.current_integrity == 0.21);
}

void test_material_only_and_energy_only_blockers() {
    const auto i = awakening();
    const auto d = FixItPlanner::plan_next(i, resources());
    const auto energy_only = resolve_recovery_goal(i, resources(10.0, 0.0), d);
    assert(energy_only.missing_material_kg == 0.0);
    assert(energy_only.missing_energy_j > 0.0);
    const auto material_only = resolve_recovery_goal(i, resources(0.0, 1e8), d);
    assert(material_only.missing_material_kg > 0.0);
    assert(material_only.missing_energy_j == 0.0);
}

void test_running_receipt_outweighs_new_planner_priority() {
    auto i = awakening();
    const auto d = FixItPlanner::plan_next(i, resources(10.0, 1e8));
    FixItExecutionStatus receipt;
    receipt.state = FixItExecutionState::Running;
    receipt.decision = d;
    receipt.elapsed_s = d->time_required_s / 2.0;

    // A concurrent external change brought computation above its minimum,
    // while thermal became critical. The running receipt must still win.
    i.computation = 0.26;
    i.thermal = 0.10;
    const auto next = FixItPlanner::plan_next(i, resources(10.0, 1e8));
    assert(next->subsystem == PowerSubsystem::Thermal);
    const auto g = resolve_recovery_goal(i, resources(10.0, 1e8), next, &receipt);
    assert(g.state == RecoveryGoalState::InProgress);
    assert(g.decision.subsystem == PowerSubsystem::Computation);
    assert(g.goal_id == recovery_goal_id(*d));
    assert(std::abs(g.progress_fraction - 0.5) < 1e-9);
}

void test_completion_must_have_receipt_and_verified_integrity() {
    auto i = awakening();
    const auto d = FixItPlanner::plan_next(i, resources(10.0, 1e8));
    FixItExecutionStatus receipt;
    receipt.state = FixItExecutionState::Completed;
    receipt.decision = d;
    receipt.elapsed_s = d->time_required_s;
    const auto false_complete = resolve_recovery_goal(i, resources(10.0, 1e8), d, &receipt);
    assert(false_complete.state == RecoveryGoalState::VerificationFailed);
    i.computation = d->target_integrity;
    const auto verified = resolve_recovery_goal(i, resources(10.0, 1e8), d, &receipt);
    assert(verified.state == RecoveryGoalState::Completed);
    const auto no_receipt = resolve_recovery_goal(i, resources(10.0, 1e8), d);
    assert(no_receipt.state != RecoveryGoalState::Completed);
}

void test_interruption_does_not_fake_success() {
    const auto i = awakening();
    const auto d = FixItPlanner::plan_next(i, resources(10.0, 1e8));
    FixItExecutionStatus receipt;
    receipt.state = FixItExecutionState::Interrupted;
    receipt.decision = d;
    receipt.elapsed_s = 1.0;
    const auto g = resolve_recovery_goal(i, resources(), d, &receipt);
    assert(g.state == RecoveryGoalState::Interrupted);
    assert(!g.goal_id.empty());
}

void test_no_goal_full_vs_damaged() {
    const ComponentIntegritySnapshot fully_restored{};
    assert(resolve_recovery_goal(fully_restored, resources(), std::nullopt).state ==
        RecoveryGoalState::FullyRestored);
    assert(!resolve_recovery_goal(fully_restored, resources(), std::nullopt).has_goal);
    assert(resolve_recovery_goal(awakening(), resources(), std::nullopt).state ==
        RecoveryGoalState::NoRecommendation);
}

void test_recommendations_are_not_fake_executable_repairs() {
    const ComponentIntegritySnapshot i{};
    auto r = resources(100.0, 1e8);
    r.evolution_design_available = true;
    const auto d = FixItPlanner::plan_next(i, r);
    assert(d.has_value());
    assert(d->stage == FixItStage::Evolution);
    const auto g = resolve_recovery_goal(i, r, d);
    assert(g.state == RecoveryGoalState::AdvisoryOnly);
}

void test_fabrication_unavailable_fails_closed() {
    auto i = awakening();
    i.sensors = 0.0;
    FixItDecision d;
    d.stage = FixItStage::Replacement;
    d.subsystem = PowerSubsystem::Sensors;
    d.target_integrity = 1.0;
    d.material_required_kg = 20.0;
    d.energy_required_j = 3e6;
    d.time_required_s = 600;
    const auto g = resolve_recovery_goal(i, resources(50.0, 1e8), d);
    assert(g.fabrication_unavailable);
    assert(g.state == RecoveryGoalState::Blocked);
    assert(g.missing_material_kg == 0.0);
}

void test_time_budget_blocks_if_explicitly_constrained() {
    const auto i = awakening();
    const auto d = FixItPlanner::plan_next(i, resources(10.0, 1e8));
    auto r = resources(10.0, 1e8);
    r.available_time_s = 0.1;
    const auto g = resolve_recovery_goal(i, r, d);
    assert(g.state == RecoveryGoalState::Blocked);
    assert(g.missing_time_s > 0.0);
}

void test_actual_executor_resources_and_transition_to_next_goal() {
    auto rt = DamageAwareProbeRuntime::make_canonical_ev0001();
    const auto i = awakening();
    rt.set_subsystem_integrity(PowerSubsystem::Computation, i.computation);
    rt.set_subsystem_integrity(PowerSubsystem::Thermal, i.thermal);
    rt.set_subsystem_integrity(PowerSubsystem::Sensors, i.sensors);
    rt.set_subsystem_integrity(PowerSubsystem::Propulsion, i.propulsion);
    rt.add_stored_material_kg(10.0);
    auto available = resources(rt.snapshot().storage_used_kg, rt.snapshot().stored_energy_j);
    const auto d = FixItPlanner::plan_next(rt.component_integrity(), available);
    assert(d->subsystem == PowerSubsystem::Computation);
    FixItRepairExecutor executor;
    executor.start(*d);
    const auto energy_before = rt.snapshot().stored_energy_j;
    const auto storage_before = rt.snapshot().storage_used_kg;
    executor.advance(rt, d->time_required_s);
    const auto completion = resolve_recovery_goal(rt.component_integrity(),
        available, d, &executor.status());
    assert(completion.state == RecoveryGoalState::Completed);
    assert(rt.snapshot().stored_energy_j < energy_before);
    assert(rt.snapshot().storage_used_kg < storage_before);
    const auto next = FixItPlanner::plan_next(rt.component_integrity(), resources(
        rt.snapshot().storage_used_kg, rt.snapshot().stored_energy_j));
    assert(next.has_value());
    assert(next->subsystem == PowerSubsystem::Thermal);
    const auto projection = resolve_recovery_goal(rt.component_integrity(),
        resources(rt.snapshot().storage_used_kg, rt.snapshot().stored_energy_j), next);
    assert(projection.goal_id != completion.goal_id);
}

void test_bounded_planner_only_repair_sequence_not_level_solvability() {
    // This validates the *repair planner's* termination with ample synthetic
    // resources, NOT mineability, recharge, reachable starting resources,
    // input timing, save/load of active executors or whole-level solvability.
    auto i = awakening();
    unsigned steps = 0;
    while (steps < 40) {
        const auto d = FixItPlanner::plan_next(i, resources(1000.0, 1e9));
        if (!d) break;
        assert(d->stage == FixItStage::Repair);
        assert(d->target_integrity > recovery_integrity(i, d->subsystem));
        set_integrity(i, d->subsystem, d->target_integrity);
        ++steps;
    }
    assert(steps < 40);
    assert(!FixItPlanner::plan_next(i, resources(1000.0, 1e9)).has_value());
    assert(resolve_recovery_goal(i, resources(), std::nullopt).state ==
        RecoveryGoalState::FullyRestored);
}

} // namespace

int main() {
    test_awakening_survival_first_goal_and_true_cost();
    test_stable_id_across_inventory_and_partial_recovery();
    test_material_only_and_energy_only_blockers();
    test_running_receipt_outweighs_new_planner_priority();
    test_completion_must_have_receipt_and_verified_integrity();
    test_interruption_does_not_fake_success();
    test_no_goal_full_vs_damaged();
    test_recommendations_are_not_fake_executable_repairs();
    test_fabrication_unavailable_fails_closed();
    test_time_budget_blocks_if_explicitly_constrained();
    test_actual_executor_resources_and_transition_to_next_goal();
    test_bounded_planner_only_repair_sequence_not_level_solvability();
    std::puts("Everward recovery goal tests passed (pure simulation contracts only)");
}
