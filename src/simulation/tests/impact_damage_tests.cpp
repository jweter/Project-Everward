#include "everward/simulation/impact_damage.hpp"

#undef NDEBUG
#include <cassert>
#include <cmath>
#include <iostream>
#include <stdexcept>

using everward::simulation::DamageAwareProbeRuntime;
using everward::simulation::ImpactDamageModel;
using everward::simulation::ImpactSeverity;
using everward::simulation::IntegrityBand;
using everward::simulation::PowerSubsystem;
using everward::simulation::SimulationClock;

static bool nearly_equal(double a, double b, double eps = 1e-6) {
    return std::fabs(a - b) <= eps;
}

int main() {
    // Severity is derived from physically meaningful normal-impact energy.
    {
        assert(ImpactDamageModel::classify_impact(5'000.0) == ImpactSeverity::Contact);
        assert(ImpactDamageModel::classify_impact(31'250.0) == ImpactSeverity::Light);
        assert(ImpactDamageModel::classify_impact(125'000.0) == ImpactSeverity::Damaging);
        assert(ImpactDamageModel::classify_impact(1'125'000.0) == ImpactSeverity::Severe);
        assert(ImpactDamageModel::classify_impact(3'125'000.0) == ImpactSeverity::Catastrophic);

        bool invalid_threw = false;
        try {
            (void)ImpactDamageModel::classify_impact(-1.0);
        } catch (const std::invalid_argument&) {
            invalid_threw = true;
        }
        assert(invalid_threw);
    }

    // Integrity bands explicitly allow a badly damaged but still functional
    // system, which is required for the canonical awakening/self-repair loop.
    {
        assert(ImpactDamageModel::integrity_band(0.0) == IntegrityBand::Offline);
        assert(ImpactDamageModel::integrity_band(0.05) == IntegrityBand::Critical);
        assert(ImpactDamageModel::integrity_band(0.50) == IntegrityBand::Degraded);
        assert(ImpactDamageModel::integrity_band(0.90) == IntegrityBand::Operational);
        assert(ImpactDamageModel::integrity_band(1.0) == IntegrityBand::Nominal);
        assert(!ImpactDamageModel::is_functional(0.0));
        assert(ImpactDamageModel::is_functional(0.05));
    }

    // A forward impact against an immovable body uses 0.5*m*v_normal^2 and
    // damages the forward sensor zone while preserving a still-functional
    // subsystem when the loss is partial.
    {
        DamageAwareProbeRuntime runtime;
        runtime.add_static_sphere_body({"forward-rock", {10.0, 0.0, 0.0}, 2.0});
        runtime.set_velocity_mps({10.0, 0.0, 0.0});
        (void)runtime.drain_events();
        runtime.advance_wall_ticks(SimulationClock::TicksPerSecond);

        const auto records = runtime.drain_damage_records();
        assert(records.size() == 1);
        const auto& record = records.front();
        assert(record.body_id == "forward-rock");
        assert(record.severity == ImpactSeverity::Damaging);
        assert(record.affected_subsystem == PowerSubsystem::Sensors);
        assert(nearly_equal(record.normal_speed_mps, 10.0));
        assert(nearly_equal(record.impact_energy_j, 125'000.0));
        assert(nearly_equal(record.integrity_before, 1.0));
        assert(nearly_equal(record.integrity_after, 0.95));
        assert(runtime.snapshot().sensors_operational);
        assert(runtime.subsystem_integrity_band(PowerSubsystem::Sensors) == IntegrityBand::Operational);

        // Reading the same contact again cannot double-apply damage.
        runtime.advance_wall_ticks(0);
        assert(runtime.drain_damage_records().empty());
        assert(nearly_equal(runtime.subsystem_integrity(PowerSubsystem::Sensors), 0.95));
    }

    // Coarse Phase-2 component zones follow probe-local orientation rather
    // than fixed world axes: aft -> propulsion, lateral -> computation,
    // dorsal/ventral -> thermal.
    {
        DamageAwareProbeRuntime aft;
        aft.add_static_sphere_body({"aft-rock", {-10.0, 0.0, 0.0}, 2.0});
        aft.set_velocity_mps({-10.0, 0.0, 0.0});
        aft.advance_wall_ticks(SimulationClock::TicksPerSecond);
        assert(aft.drain_damage_records().front().affected_subsystem == PowerSubsystem::Propulsion);

        DamageAwareProbeRuntime lateral;
        lateral.add_static_sphere_body({"side-rock", {0.0, 10.0, 0.0}, 2.0});
        lateral.set_velocity_mps({0.0, 10.0, 0.0});
        lateral.advance_wall_ticks(SimulationClock::TicksPerSecond);
        assert(lateral.drain_damage_records().front().affected_subsystem == PowerSubsystem::Computation);

        DamageAwareProbeRuntime dorsal;
        dorsal.add_static_sphere_body({"top-rock", {0.0, 0.0, 10.0}, 2.0});
        dorsal.set_velocity_mps({0.0, 0.0, 10.0});
        dorsal.advance_wall_ticks(SimulationClock::TicksPerSecond);
        assert(dorsal.drain_damage_records().front().affected_subsystem == PowerSubsystem::Thermal);
    }

    // Gentle contact records an assessed impact but does not invent damage.
    // The target sits 1.5 m beyond the forward hull sample (not exactly on
    // top of it) so the compound-envelope contact normal is well-defined
    // rather than the degenerate zero-distance case.
    {
        DamageAwareProbeRuntime runtime;
        runtime.add_static_sphere_body({"gentle", {6.5, 0.0, 0.0}, 2.0});
        runtime.set_velocity_mps({2.0, 0.0, 0.0});
        runtime.advance_wall_ticks(2 * SimulationClock::TicksPerSecond);
        const auto records = runtime.drain_damage_records();
        assert(records.size() == 1);
        assert(records.front().severity == ImpactSeverity::Contact);
        assert(nearly_equal(records.front().integrity_before, 1.0));
        assert(nearly_equal(records.front().integrity_after, 1.0));
    }

    // A catastrophic forward impact can destroy the affected component and
    // propagates into the existing subsystem-operational consequences.
    {
        DamageAwareProbeRuntime runtime;
        runtime.add_static_sphere_body({"catastrophic", {20.0, 0.0, 0.0}, 2.0});
        runtime.set_velocity_mps({50.0, 0.0, 0.0});
        runtime.advance_wall_ticks(SimulationClock::TicksPerSecond);
        const auto records = runtime.drain_damage_records();
        assert(records.size() == 1);
        assert(records.front().severity == ImpactSeverity::Catastrophic);
        assert(records.front().affected_subsystem == PowerSubsystem::Sensors);
        assert(nearly_equal(records.front().integrity_after, 0.0));
        assert(!runtime.snapshot().sensors_operational);
        assert(!runtime.snapshot().can_scan);
        assert(runtime.subsystem_integrity_band(PowerSubsystem::Sensors) == IntegrityBand::Offline);
    }

    // The same integrity API can initialize the future damaged awakening and
    // restore a system above zero without creating a separate tutorial-only
    // health model. Partial integrity also causes real partial performance.
    {
        DamageAwareProbeRuntime runtime = DamageAwareProbeRuntime::make_canonical_ev0001();
        runtime.set_subsystem_integrity(PowerSubsystem::Propulsion, 0.05);
        assert(runtime.snapshot().propulsion_operational);
        assert(runtime.snapshot().can_thrust);
        assert(runtime.subsystem_integrity_band(PowerSubsystem::Propulsion) == IntegrityBand::Critical);

        runtime.adjust_local_velocity_mps({10.0, 0.0, 0.0});
        assert(nearly_equal(runtime.snapshot().velocity_mps.x, 0.5));
        runtime.adjust_attitude_degrees({20.0, 0.0, 0.0});
        assert(nearly_equal(runtime.snapshot().attitude_degrees.yaw, 1.0));

        runtime.set_subsystem_integrity(PowerSubsystem::Sensors, 0.50);
        runtime.start_scan("degraded-sensor-test", 10.0);
        assert(nearly_equal(runtime.snapshot().scan_remaining_s, 20.0));
        runtime.cancel_scan();

        runtime.set_subsystem_integrity(PowerSubsystem::Propulsion, 0.0);
        assert(!runtime.snapshot().propulsion_operational);
        assert(!runtime.snapshot().can_thrust);

        runtime.set_subsystem_integrity(PowerSubsystem::Propulsion, 0.10);
        assert(runtime.snapshot().propulsion_operational);
        assert(runtime.snapshot().can_thrust);
    }

    // DamageAwareProbeRuntime forwards Slice 7 target selection to the
    // wrapped ProbeRuntime rather than duplicating the registry/telemetry.
    {
        DamageAwareProbeRuntime runtime = DamageAwareProbeRuntime::make_canonical_ev0001();
        runtime.add_static_sphere_body({"survey-target", {30.0, 0.0, 0.0}, 4.0});

        runtime.select_nearest_target(1000.0);
        assert(runtime.selected_target_status().has_selection);
        assert(runtime.selected_target_status().body_id == "survey-target");

        runtime.clear_target_selection();
        assert(!runtime.selected_target_status().has_selection);

        runtime.select_target("survey-target");
        assert(runtime.selected_target_status().has_selection);
        runtime.select_target("nonexistent");
        assert(!runtime.selected_target_status().has_selection);
    }

    std::cout << "Impact severity and component damage foundation tests passed\n";
    return 0;
}
