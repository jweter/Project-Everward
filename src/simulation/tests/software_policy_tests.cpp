#include "everward/simulation/software_policy.hpp"

#undef NDEBUG
#include <cassert>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using everward::simulation::ApproachMotionState;
using everward::simulation::DomainEvent;
using everward::simulation::DomainEventType;
using everward::simulation::PolicyActionKind;
using everward::simulation::PolicyConditionKind;
using everward::simulation::PowerSubsystem;
using everward::simulation::ProbeRuntime;
using everward::simulation::SimulationClock;
using everward::simulation::SoftwarePolicy;
using everward::simulation::SoftwarePolicyRule;
using everward::simulation::StaticSphereBody;
using everward::simulation::TargetSelectionStatus;

static bool nearly_equal_local(double a, double b, double eps = 1e-6) {
    return std::fabs(a - b) <= eps;
}

static bool has_event(const std::vector<DomainEvent>& events, DomainEventType type) {
    for (const auto& event : events) {
        if (event.type == type) {
            return true;
        }
    }
    return false;
}

static SoftwarePolicy make_sensor_conservation_policy() {
    SoftwarePolicy policy;
    policy.id = "conserve_sensor_power";
    policy.rules = {
        SoftwarePolicyRule{
            "shed_sensors_below_60_percent",
            PolicyConditionKind::EnergyFractionBelow,
            0.60,
            PolicyActionKind::SetPowerAllocation,
            PowerSubsystem::Sensors,
            0.0,
        },
    };
    return policy;
}

int main() {
    // Generation 1 intentionally supports only a tiny rule set. This is a
    // capability/computation constraint, not a UI-only restriction.
    {
        ProbeRuntime runtime;
        SoftwarePolicy too_complex;
        too_complex.id = "too_complex";
        too_complex.rules = {
            {"one", PolicyConditionKind::EnergyFractionBelow, 0.5, PolicyActionKind::SetPowerAllocation, PowerSubsystem::Sensors, 0.0},
            {"two", PolicyConditionKind::TemperatureAboveKelvin, 350.0, PolicyActionKind::SetPowerAllocation, PowerSubsystem::Propulsion, 0.0},
            {"three", PolicyConditionKind::EnergyFractionAbove, 0.9, PolicyActionKind::SetPowerAllocation, PowerSubsystem::Thermal, 10.0},
        };

        bool threw = false;
        try {
            runtime.install_policy(too_complex);
        } catch (const std::invalid_argument&) {
            threw = true;
        }
        assert(threw);
        assert(!runtime.policy_status().installed);
    }

    // Installing a valid policy is observable, but the policy does not run
    // until the physical computation subsystem has enough allocated power.
    {
        ProbeRuntime runtime;
        runtime.allocate_power(PowerSubsystem::Sensors, 100.0);
        runtime.install_policy(make_sensor_conservation_policy());
        auto install_events = runtime.drain_events();
        assert(has_event(install_events, DomainEventType::PolicyChanged));

        runtime.advance_wall_ticks(SimulationClock::TicksPerSecond);
        assert(runtime.snapshot().power_allocated_sensors_w == 100.0);
        assert(!runtime.policy_status().executor_available);
    }

    // Once computation is powered, automation observes authoritative state
    // and uses the exact same SimulationCore::allocate_power command as the
    // manual path. EV-0001 starts at 50% stored energy, so the <60% rule fires.
    {
        ProbeRuntime runtime;
        runtime.allocate_power(PowerSubsystem::Sensors, 100.0);
        runtime.allocate_power(
            PowerSubsystem::Computation,
            ProbeRuntime::kGeneration1MinimumPolicyComputationPowerW);
        runtime.install_policy(make_sensor_conservation_policy());
        (void)runtime.drain_events();

        runtime.advance_wall_ticks(SimulationClock::TicksPerSecond);
        assert(runtime.snapshot().power_allocated_sensors_w == 0.0);

        const auto events = runtime.drain_events();
        assert(has_event(events, DomainEventType::PowerAllocationChanged));
        assert(has_event(events, DomainEventType::PolicyRuleTriggered));
    }

    // Manual control and automation genuinely share mechanics: the player can
    // manually restore sensor power, but on the next evaluation the active
    // policy sees the same state and issues the same authoritative command to
    // shed it again. No parallel automation-only power state exists.
    {
        ProbeRuntime runtime;
        runtime.allocate_power(
            PowerSubsystem::Computation,
            ProbeRuntime::kGeneration1MinimumPolicyComputationPowerW);
        runtime.install_policy(make_sensor_conservation_policy());
        runtime.advance_wall_ticks(SimulationClock::TicksPerSecond);
        (void)runtime.drain_events();

        runtime.allocate_power(PowerSubsystem::Sensors, 80.0);
        assert(runtime.snapshot().power_allocated_sensors_w == 80.0);
        (void)runtime.drain_events();

        runtime.advance_wall_ticks(SimulationClock::TicksPerSecond);
        assert(runtime.snapshot().power_allocated_sensors_w == 0.0);
        const auto events = runtime.drain_events();
        assert(has_event(events, DomainEventType::PolicyRuleTriggered));
    }

    // Computation failure disables policy execution even if power had been
    // assigned. Restoring hardware is not enough by itself because subsystem
    // failure correctly sheds its allocation; the player must repower compute.
    {
        ProbeRuntime runtime;
        runtime.allocate_power(PowerSubsystem::Sensors, 100.0);
        runtime.allocate_power(
            PowerSubsystem::Computation,
            ProbeRuntime::kGeneration1MinimumPolicyComputationPowerW);
        runtime.install_policy(make_sensor_conservation_policy());
        (void)runtime.drain_events();

        runtime.set_subsystem_operational(PowerSubsystem::Computation, false);
        assert(!runtime.policy_status().executor_available);
        runtime.advance_wall_ticks(SimulationClock::TicksPerSecond);
        assert(runtime.snapshot().power_allocated_sensors_w == 100.0);

        runtime.set_subsystem_operational(PowerSubsystem::Computation, true);
        assert(!runtime.policy_status().executor_available);
        runtime.allocate_power(
            PowerSubsystem::Computation,
            ProbeRuntime::kGeneration1MinimumPolicyComputationPowerW);
        assert(runtime.policy_status().executor_available);
        runtime.advance_wall_ticks(SimulationClock::TicksPerSecond);
        assert(runtime.snapshot().power_allocated_sensors_w == 0.0);
    }

    // A primitive rule is allowed to shut off its own computation budget.
    // This intentionally makes Generation-1 automation clunky: once compute
    // reaches zero the remaining rule cannot execute until manually repowered.
    {
        ProbeRuntime runtime;
        runtime.allocate_power(PowerSubsystem::Sensors, 100.0);
        runtime.allocate_power(
            PowerSubsystem::Computation,
            ProbeRuntime::kGeneration1MinimumPolicyComputationPowerW);

        SoftwarePolicy self_disabling;
        self_disabling.id = "primitive_self_disable";
        self_disabling.rules = {
            {"drop_compute", PolicyConditionKind::EnergyFractionBelow, 0.60, PolicyActionKind::SetPowerAllocation, PowerSubsystem::Computation, 0.0},
            {"drop_sensors", PolicyConditionKind::EnergyFractionBelow, 0.60, PolicyActionKind::SetPowerAllocation, PowerSubsystem::Sensors, 0.0},
        };
        runtime.install_policy(self_disabling);
        (void)runtime.drain_events();

        runtime.advance_wall_ticks(SimulationClock::TicksPerSecond);
        assert(runtime.snapshot().power_allocated_computation_w == 0.0);
        assert(runtime.snapshot().power_allocated_sensors_w == 100.0);
        assert(!runtime.policy_status().executor_available);
    }

    // Clearing the policy is deterministic and leaves manual state alone.
    {
        ProbeRuntime runtime;
        runtime.install_policy(make_sensor_conservation_policy());
        runtime.clear_policy();
        assert(!runtime.policy_status().installed);
        assert(runtime.active_policy() == nullptr);
        const auto events = runtime.drain_events();
        assert(has_event(events, DomainEventType::PolicyChanged));
    }

    // Slice 7 runtime/telemetry wiring: no registered bodies means no
    // selection, matching find_nearest_selectable_target's no-guessing
    // contract rather than reporting a fabricated target.
    {
        ProbeRuntime runtime;
        runtime.select_nearest_target(1000.0);
        const TargetSelectionStatus status = runtime.selected_target_status();
        assert(!status.has_selection);
        assert(status.body_id.empty());
    }

    // Nearest-target selection picks the closer of two in-range bodies, and
    // reports live surface range/closing speed for it.
    {
        ProbeRuntime runtime;
        runtime.add_static_sphere_body(StaticSphereBody{"far-rock", {100.0, 0.0, 0.0}, 5.0});
        runtime.add_static_sphere_body(StaticSphereBody{"near-rock", {20.0, 0.0, 0.0}, 5.0});
        runtime.select_nearest_target(1000.0);

        const TargetSelectionStatus status = runtime.selected_target_status();
        assert(status.has_selection);
        assert(status.body_id == "near-rock");
        assert(nearly_equal_local(status.surface_range_m, 15.0));
        assert(nearly_equal_local(status.closing_speed_mps, 0.0));
        assert(status.approach_motion == ApproachMotionState::HoldingRange);
    }

    // A max selection range excludes bodies beyond it rather than selecting
    // the nearest body regardless of distance.
    {
        ProbeRuntime runtime;
        runtime.add_static_sphere_body(StaticSphereBody{"distant", {500.0, 0.0, 0.0}, 5.0});
        runtime.select_nearest_target(50.0);
        assert(!runtime.selected_target_status().has_selection);
    }

    // Closing speed is positive while approaching a stationary body head-on.
    {
        ProbeRuntime runtime;
        runtime.add_static_sphere_body(StaticSphereBody{"ahead", {100.0, 0.0, 0.0}, 5.0});
        runtime.set_velocity_mps({10.0, 0.0, 0.0});
        runtime.select_target("ahead");

        const TargetSelectionStatus status = runtime.selected_target_status();
        assert(status.has_selection);
        assert(nearly_equal_local(status.closing_speed_mps, 10.0));
        assert(status.approach_motion == ApproachMotionState::Closing);
    }

    // Closing speed is negative, and the motion classification reads
    // Opening, while receding from a stationary body head-on.
    {
        ProbeRuntime runtime;
        runtime.add_static_sphere_body(StaticSphereBody{"behind", {-100.0, 0.0, 0.0}, 5.0});
        runtime.set_velocity_mps({10.0, 0.0, 0.0});
        runtime.select_target("behind");

        const TargetSelectionStatus status = runtime.selected_target_status();
        assert(status.has_selection);
        assert(status.closing_speed_mps < 0.0);
        assert(status.approach_motion == ApproachMotionState::Opening);
    }

    // Explicit selection of an unknown or empty id fails closed: no stale
    // selection is kept.
    {
        ProbeRuntime runtime;
        runtime.add_static_sphere_body(StaticSphereBody{"known", {10.0, 0.0, 0.0}, 2.0});
        runtime.select_target("known");
        assert(runtime.selected_target_status().has_selection);

        runtime.select_target("unregistered-id");
        assert(!runtime.selected_target_status().has_selection);

        runtime.select_target("known");
        assert(runtime.selected_target_status().has_selection);
        runtime.select_target("");
        assert(!runtime.selected_target_status().has_selection);
    }

    // A selection survives until explicitly cleared, and a deregistered
    // selection reports unselected without needing clear_target_selection().
    {
        ProbeRuntime runtime;
        runtime.add_static_sphere_body(StaticSphereBody{"body-a", {10.0, 0.0, 0.0}, 2.0});
        runtime.select_target("body-a");
        assert(runtime.selected_target_status().has_selection);

        runtime.clear_target_selection();
        assert(!runtime.selected_target_status().has_selection);

        runtime.select_target("body-a");
        runtime.clear_static_bodies();
        assert(!runtime.selected_target_status().has_selection);
    }

    std::cout << "Generation-1 software policy tests passed\n";
    return 0;
}
