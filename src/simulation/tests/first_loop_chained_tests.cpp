// Chained first-loop proof (engine-independent): wake damaged -> scan -> mine
// (collect) -> Fix_It repair consuming stored material/energy -> starter
// restoration, in ONE deterministic simulation run. No state is injected to
// skip a step: the only direct state writes are the documented damaged-wake
// integrity values; material arrives only via mining, and restoration is
// reached only through Fix_It executor consumption.
#include "everward/simulation/fix_it.hpp"
#include "everward/simulation/mining.hpp"
#include "everward/simulation/starter_restoration.hpp"

#undef NDEBUG
#include <cassert>
#include <cmath>
#include <iostream>
#include <string>

using namespace everward::simulation;

namespace {

struct LoopOutcome {
    bool restored{false};
    int mining_cycles{0};
    int repair_steps{0};
    double mined_kg{0.0};
    double material_consumed_kg{0.0};
    double energy_consumed_j{0.0};
    double final_storage_kg{0.0};
    double final_energy_j{0.0};
    std::int64_t final_tick{0};
    double final_integrity[4]{};
};

LoopOutcome run_first_loop() {
    LoopOutcome out;
    const std::string target = "chain-rock";

    // 1. Wake: canonical probe with the documented damaged-awakening integrity.
    DamageAwareProbeRuntime rt = DamageAwareProbeRuntime::make_canonical_ev0001();
    rt.set_subsystem_integrity(PowerSubsystem::Sensors, 0.35);
    rt.set_subsystem_integrity(PowerSubsystem::Propulsion, 0.40);
    rt.set_subsystem_integrity(PowerSubsystem::Computation, 0.18);
    rt.set_subsystem_integrity(PowerSubsystem::Thermal, 0.22);
    assert(!evaluate_starter_restoration(rt).satisfied);
    assert(rt.snapshot().storage_used_kg == 0.0);
    const double wake_energy_j = rt.snapshot().stored_energy_j;

    // Arm + deposit (placed within reach of the deployed port tool tip).
    ManipulatorRig rig;
    rig.begin_deploy(ManipulatorArmId::Port);
    rig.advance(ManipulatorRig::kDeployStowDurationS);
    rig.attach_tool(ManipulatorArmId::Port);
    const ManipulatorArmState& arm = rig.arm(ManipulatorArmId::Port);
    const ManipulatorArmSample tip = manipulator_tool_tip_contact_sample(
        ManipulatorArmId::Port, arm.deployment_fraction, arm.angles);
    ResourceDeposit deposit;
    deposit.body.body_id = target;
    deposit.body.radius_m = 0.50;
    deposit.body.center_m = {tip.center_m.x + deposit.body.radius_m + tip.radius_m + 0.20,
                             tip.center_m.y, tip.center_m.z};
    deposit.material_id = "iron_regolith";
    deposit.display_name = "Iron-bearing regolith";
    deposit.remaining_kg = 200.0;
    deposit.extraction_kg_per_cycle = 5.0;
    MiningSystem mining;
    mining.add_deposit(deposit);

    // Collection is refused before a genuine scan completes.
    assert(!mining.mine_once(target, rig, ProbeWorldPose{}, 0.0, rt.snapshot().storage_capacity_kg).accepted);

    // 2. Scan: a real runtime scan, completed by simulation ticks (damaged
    // sensors elongate it); the ScanCompleted event earns the survey.
    rt.start_scan(target, 2.0);
    bool scan_completed = false;
    for (int i = 0; i < 20 && !scan_completed; ++i) {
        rt.advance_wall_ticks(SimulationClock::TicksPerSecond);
        for (const DomainEvent& e : rt.drain_events()) {
            if (e.type == DomainEventType::ScanCompleted &&
                e.detail == "scan complete: " + target) {
                scan_completed = true;
            }
        }
    }
    assert(scan_completed);
    assert(rt.target_knowledge_state(target).has_value());
    mining.mark_surveyed(target);

    // 3. Collect: mine, routing mass into the authoritative inventory.
    auto mine_until = [&](double wanted_kg) {
        while (rt.snapshot().storage_used_kg < wanted_kg) {
            const MiningAttemptResult r = mining.mine_once(
                target, rig, ProbeWorldPose{}, rt.snapshot().storage_used_kg,
                rt.snapshot().storage_capacity_kg);
            assert(r.accepted);
            rt.add_stored_material_kg(r.extracted_kg, r.material_id);
            out.mined_kg += r.extracted_kg;
            assert(++out.mining_cycles < 200);
        }
    };
    mine_until(10.0);
    assert(!evaluate_starter_restoration(rt).satisfied);  // collecting alone restores nothing

    // 4. Repair: Fix_It plans from live state and consumes stored material/energy.
    while (!evaluate_starter_restoration(rt).satisfied) {
        const FixItResources res{rt.snapshot().storage_used_kg, rt.snapshot().stored_energy_j,
                                 1.0e9, false, false, false, false};
        const auto decision = FixItPlanner::plan_next(rt.component_integrity(), res);
        assert(decision.has_value() && decision->stage == FixItStage::Repair);
        mine_until(decision->material_required_kg);  // collect what this step needs
        FixItRepairExecutor executor;
        executor.start(*decision);
        executor.advance(rt, decision->time_required_s);
        assert(executor.status().state == FixItExecutionState::Completed);
        out.material_consumed_kg += executor.status().material_consumed_kg;
        out.energy_consumed_j += executor.status().energy_consumed_j;
        assert(++out.repair_steps < 100);
    }

    // 5. Starter restoration, with real consumption of what was collected.
    assert(evaluate_starter_restoration(rt).satisfied);
    assert(out.material_consumed_kg > 0.0 && out.material_consumed_kg <= out.mined_kg + 1e-9);
    assert(std::fabs(rt.snapshot().storage_used_kg - (out.mined_kg - out.material_consumed_kg)) < 1e-6);
    assert(rt.snapshot().stored_energy_j < wake_energy_j);
    assert(out.energy_consumed_j > 0.0);

    out.restored = true;
    out.final_storage_kg = rt.snapshot().storage_used_kg;
    out.final_energy_j = rt.snapshot().stored_energy_j;
    out.final_tick = rt.tick();
    const PowerSubsystem order[4] = {PowerSubsystem::Sensors, PowerSubsystem::Propulsion,
                                     PowerSubsystem::Computation, PowerSubsystem::Thermal};
    for (int i = 0; i < 4; ++i) out.final_integrity[i] = rt.subsystem_integrity(order[i]);
    return out;
}

} // namespace

int main() {
    const LoopOutcome a = run_first_loop();
    const LoopOutcome b = run_first_loop();
    assert(a.restored && b.restored);
    // Deterministic across repeated runs (exact equality, same inputs).
    assert(a.mining_cycles == b.mining_cycles && a.repair_steps == b.repair_steps);
    assert(a.mined_kg == b.mined_kg && a.material_consumed_kg == b.material_consumed_kg);
    assert(a.energy_consumed_j == b.energy_consumed_j);
    assert(a.final_storage_kg == b.final_storage_kg && a.final_energy_j == b.final_energy_j);
    assert(a.final_tick == b.final_tick);
    for (int i = 0; i < 4; ++i) assert(a.final_integrity[i] == b.final_integrity[i]);
    std::cout << "first_loop_chained_tests passed (mining_cycles=" << a.mining_cycles
              << ", repair_steps=" << a.repair_steps << ")\n";
    return 0;
}
