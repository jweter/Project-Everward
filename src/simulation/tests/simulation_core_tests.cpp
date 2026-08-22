#include "everward/simulation/core.hpp"

// This test file is entirely `assert()`-based, and .github/workflows/foundation.yml
// configures this target's build with `-DCMAKE_BUILD_TYPE=Release`, which
// defines NDEBUG and would otherwise compile every assert() below into a
// silent no-op: the test binary would still run to completion and CTest
// would report success regardless of whether any assertion actually held.
// `assert.h`/`<cassert>` intentionally has no include guard so it can be
// re-included after `#undef NDEBUG` to force real, active assertions in this
// translation unit no matter what NDEBUG state the build type otherwise
// defines. See ERROR_RESOLUTION_LEDGER.md, 2026-08-22, for how this was
// found and why it matters: it applied to every simulation-core CTest since
// PR #68, not just this file's own tests.
#undef NDEBUG
#include <cassert>
#include <cmath>
#include <iostream>

using everward::simulation::SimulationClock;
using everward::simulation::SimulationCore;
using everward::simulation::Vector3d;

static bool nearly_equal(double a, double b, double eps = 1e-9) {
    return std::fabs(a - b) <= eps;
}

int main() {
    SimulationCore core;
    assert(core.tick() == 0);
    assert(core.snapshot().probe_id == "EV-0001");

    core.set_velocity_mps(Vector3d{10.0, -2.0, 0.5});
    core.advance_wall_ticks(SimulationClock::TicksPerSecond / 2);
    assert(core.tick() == 500000);
    assert(nearly_equal(core.snapshot().position_m.x, 5.0));
    assert(nearly_equal(core.snapshot().position_m.y, -1.0));
    assert(nearly_equal(core.snapshot().position_m.z, 0.25));

    core.advance_wall_ticks(SimulationClock::TicksPerSecond / 2);
    assert(core.tick() == 1000000);
    assert(nearly_equal(core.snapshot().position_m.x, 10.0));
    assert(nearly_equal(core.snapshot().position_m.y, -2.0));
    assert(nearly_equal(core.snapshot().position_m.z, 0.5));

    auto events = core.drain_events();
    assert(events.size() == 3);
    assert(core.drain_events().empty());

    bool threw = false;
    try {
        core.advance_wall_ticks(-1);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    assert(threw);

    // ScanCommand: validation.
    {
        SimulationCore scan_core;

        bool empty_target_threw = false;
        try {
            scan_core.start_scan("", 60.0);
        } catch (const std::invalid_argument&) {
            empty_target_threw = true;
        }
        assert(empty_target_threw);

        bool non_positive_duration_threw = false;
        try {
            scan_core.start_scan("asteroid-1", 0.0);
        } catch (const std::invalid_argument&) {
            non_positive_duration_threw = true;
        }
        assert(non_positive_duration_threw);

        bool negative_duration_threw = false;
        try {
            scan_core.start_scan("asteroid-1", -5.0);
        } catch (const std::invalid_argument&) {
            negative_duration_threw = true;
        }
        assert(negative_duration_threw);

        assert(!scan_core.snapshot().is_scanning);
    }

    // ScanCommand: happy path emits scan_started then scan_complete after the
    // scan duration elapses, and blocks a second concurrent scan.
    {
        SimulationCore scan_core;
        scan_core.start_scan("asteroid-1", 2.0);

        assert(scan_core.snapshot().is_scanning);
        assert(scan_core.snapshot().active_scan_target_id == "asteroid-1");
        assert(nearly_equal(scan_core.snapshot().scan_remaining_s, 2.0));

        auto started_events = scan_core.drain_events();
        assert(started_events.size() == 1);
        assert(started_events.front().type == everward::simulation::DomainEventType::ScanStarted);

        bool already_scanning_threw = false;
        try {
            scan_core.start_scan("asteroid-2", 5.0);
        } catch (const std::runtime_error&) {
            already_scanning_threw = true;
        }
        assert(already_scanning_threw);

        // First half-second: still scanning, no completion event yet.
        scan_core.advance_wall_ticks(SimulationClock::TicksPerSecond / 2);
        assert(scan_core.snapshot().is_scanning);
        auto mid_events = scan_core.drain_events();
        assert(mid_events.size() == 1);
        assert(mid_events.front().type == everward::simulation::DomainEventType::SimulationAdvanced);

        // Remaining 1.5 seconds crosses the 2.0 second scan duration.
        scan_core.advance_wall_ticks(SimulationClock::TicksPerSecond + SimulationClock::TicksPerSecond / 2);
        assert(!scan_core.snapshot().is_scanning);
        assert(scan_core.snapshot().active_scan_target_id.empty());
        assert(nearly_equal(scan_core.snapshot().scan_remaining_s, 0.0));

        auto completion_events = scan_core.drain_events();
        assert(completion_events.size() == 2);
        assert(completion_events.front().type == everward::simulation::DomainEventType::ScanCompleted);
        assert(completion_events.back().type == everward::simulation::DomainEventType::SimulationAdvanced);

        // Scanning again after completion is allowed.
        scan_core.start_scan("asteroid-2", 1.0);
        assert(scan_core.snapshot().is_scanning);
    }

    // PowerAllocationCommand: validation.
    {
        SimulationCore power_core;

        bool negative_watts_threw = false;
        try {
            power_core.allocate_power(everward::simulation::PowerSubsystem::Sensors, -1.0);
        } catch (const std::invalid_argument&) {
            negative_watts_threw = true;
        }
        assert(negative_watts_threw);

        bool over_capacity_threw = false;
        try {
            power_core.allocate_power(everward::simulation::PowerSubsystem::Propulsion,
                                       power_core.snapshot().power_capacity_w + 1.0);
        } catch (const std::runtime_error&) {
            over_capacity_threw = true;
        }
        assert(over_capacity_threw);

        assert(nearly_equal(power_core.total_power_allocated_w(), 0.0));
    }

    // PowerAllocationCommand: happy path sets per-subsystem allocation, emits
    // PowerAllocationChanged, allows reallocation, and rejects a combined
    // request that would exceed total capacity.
    {
        SimulationCore power_core;
        const double capacity = power_core.snapshot().power_capacity_w;

        power_core.allocate_power(everward::simulation::PowerSubsystem::Sensors, 100.0);
        assert(nearly_equal(power_core.snapshot().power_allocated_sensors_w, 100.0));

        auto sensor_events = power_core.drain_events();
        assert(sensor_events.size() == 1);
        assert(sensor_events.front().type == everward::simulation::DomainEventType::PowerAllocationChanged);

        power_core.allocate_power(everward::simulation::PowerSubsystem::Propulsion, 200.0);
        power_core.allocate_power(everward::simulation::PowerSubsystem::Computation, 50.0);
        power_core.allocate_power(everward::simulation::PowerSubsystem::Thermal, 25.0);
        assert(nearly_equal(power_core.total_power_allocated_w(), 375.0));
        auto discarded_events = power_core.drain_events();
        assert(discarded_events.size() == 3);

        // Reallocating an already-allocated subsystem replaces its share
        // rather than accumulating on top of it.
        power_core.allocate_power(everward::simulation::PowerSubsystem::Sensors, 40.0);
        assert(nearly_equal(power_core.snapshot().power_allocated_sensors_w, 40.0));
        assert(nearly_equal(power_core.total_power_allocated_w(), 315.0));

        // A request that would push the combined allocation past capacity is
        // rejected and leaves existing allocations untouched.
        bool exceeds_capacity_threw = false;
        try {
            power_core.allocate_power(everward::simulation::PowerSubsystem::Propulsion, capacity);
        } catch (const std::runtime_error&) {
            exceeds_capacity_threw = true;
        }
        assert(exceeds_capacity_threw);
        assert(nearly_equal(power_core.snapshot().power_allocated_propulsion_w, 200.0));
        assert(nearly_equal(power_core.total_power_allocated_w(), 315.0));

        // Allocating exactly the remaining headroom is allowed.
        const double remaining = capacity - power_core.total_power_allocated_w();
        power_core.allocate_power(everward::simulation::PowerSubsystem::Thermal, 25.0 + remaining);
        assert(nearly_equal(power_core.total_power_allocated_w(), capacity));
    }

    // EnergyConsumption: allocating power with no elapsed time does not
    // touch stored energy, and zero allocated power draws nothing even as
    // time advances (mirrors allocate_power's own no-op-when-invalid guard
    // style: no allocation, no consumption, no event).
    {
        SimulationCore energy_core;
        const double initial_energy = energy_core.snapshot().stored_energy_j;

        energy_core.advance_wall_ticks(SimulationClock::TicksPerSecond * 10);
        assert(nearly_equal(energy_core.snapshot().stored_energy_j, initial_energy));

        auto events = energy_core.drain_events();
        for (const auto& event : events) {
            assert(event.type != everward::simulation::DomainEventType::EnergyDepleted);
        }
    }

    // EnergyConsumption: happy path. Allocated power draws down stored
    // energy at watts * elapsed seconds on the same fixed-step integration
    // path as movement and scanning, without depleting it.
    {
        SimulationCore energy_core;
        const double initial_energy = energy_core.snapshot().stored_energy_j;

        energy_core.allocate_power(everward::simulation::PowerSubsystem::Sensors, 100.0);
        energy_core.allocate_power(everward::simulation::PowerSubsystem::Propulsion, 150.0);
        (void)energy_core.drain_events();

        energy_core.advance_wall_ticks(SimulationClock::TicksPerSecond * 4);
        const double expected_energy = initial_energy - 250.0 * 4.0;
        assert(nearly_equal(energy_core.snapshot().stored_energy_j, expected_energy));
        assert(energy_core.snapshot().stored_energy_j > 0.0);

        auto events = energy_core.drain_events();
        assert(events.size() == 1);
        assert(events.front().type == everward::simulation::DomainEventType::SimulationAdvanced);
    }

    // EnergyConsumption: boundary case. Advancing exactly enough time to
    // draw down all stored energy lands it precisely at zero and emits
    // EnergyDepleted exactly once, alongside the routine SimulationAdvanced
    // event for that same fixed step.
    //
    // Deliberately allocates a modest 100 W here rather than the full
    // power_capacity_w: at 100 W the thermal equilibrium
    // (ambient_temperature_k + 100 / passive_cooling_w_per_k = 343.15 K)
    // stays below max_operating_temperature_k (373.15 K), so temperature_k
    // asymptotically approaches but never reaches the overheat threshold no
    // matter how long this test runs, keeping this test isolated to the
    // energy-depletion transition alone. The previous version of this test
    // allocated the full power_capacity_w (750 W, whose 668.15 K equilibrium
    // is well past the threshold) and depended on depletion (~666,667 s)
    // happening before the probe also crossed max_operating_temperature_k
    // (~300,250 s at that wattage) — it did not, so this test was actually
    // asserting a stale two-event expectation against a state that, once
    // integrate_overheat_response() was added, also carried an
    // OverheatStarted event. `-DCMAKE_BUILD_TYPE=Release` in CI defines
    // NDEBUG, which silently compiled every assert() in this file into a
    // no-op, so CTest never actually caught this; see the NDEBUG fix at the
    // top of this file and ERROR_RESOLUTION_LEDGER.md, 2026-08-22.
    {
        SimulationCore energy_core;
        energy_core.allocate_power(everward::simulation::PowerSubsystem::Computation, 100.0);
        (void)energy_core.drain_events();

        const double capacity_w = 100.0;
        const double initial_energy = energy_core.snapshot().stored_energy_j;
        const double exact_depletion_seconds = initial_energy / capacity_w;
        // Round the tick count up (not truncate) so the whole-tick elapsed
        // time this test actually advances by is guaranteed to reach or
        // exceed exact_depletion_seconds. Truncating here previously left a
        // sub-tick fractional remainder of simulated time unaccounted for,
        // so stored_energy_j landed a fraction of a joule above zero instead
        // of at or below it, and EnergyDepleted never fired.
        const auto exact_depletion_ticks =
            static_cast<std::int64_t>(std::ceil(exact_depletion_seconds * SimulationClock::TicksPerSecond));

        energy_core.advance_wall_ticks(exact_depletion_ticks);
        assert(nearly_equal(energy_core.snapshot().stored_energy_j, 0.0, 1e-3));
        // Confirms this test stays isolated from the overheat threshold, as
        // explained above, so the two-event expectation below holds.
        assert(!energy_core.snapshot().is_overheated);

        auto depletion_events = energy_core.drain_events();
        assert(depletion_events.size() == 2);
        assert(depletion_events.front().type == everward::simulation::DomainEventType::EnergyDepleted);
        assert(depletion_events.back().type == everward::simulation::DomainEventType::SimulationAdvanced);

        // Further advancing while fully depleted clamps at zero rather than
        // going negative, and does not re-emit EnergyDepleted since the
        // event marks the >0 -> 0 transition, not a steady depleted state.
        energy_core.advance_wall_ticks(SimulationClock::TicksPerSecond);
        assert(nearly_equal(energy_core.snapshot().stored_energy_j, 0.0));
        auto steady_state_events = energy_core.drain_events();
        for (const auto& event : steady_state_events) {
            assert(event.type != everward::simulation::DomainEventType::EnergyDepleted);
        }
    }

    // ThermalLoad: no-op case. With no allocated power and the probe already
    // at its default ambient baseline, temperature_k does not move even as
    // simulated time advances: zero waste heat and zero displacement from
    // ambient both mean zero net passive-cooling flow (mirrors
    // EnergyConsumption's own no-op guard case).
    {
        SimulationCore thermal_core;
        const double initial_temperature = thermal_core.snapshot().temperature_k;
        assert(nearly_equal(initial_temperature, thermal_core.snapshot().ambient_temperature_k));

        thermal_core.advance_wall_ticks(SimulationClock::TicksPerSecond * 10);
        assert(nearly_equal(thermal_core.snapshot().temperature_k, initial_temperature));
    }

    // ThermalLoad: happy path under sustained allocated power. Allocated
    // power is treated as waste heat dissipated into the probe's thermal
    // mass while passive cooling proportional to (temperature - ambient)
    // pulls back toward equilibrium, following:
    //   T(t) = T_eq + (T0 - T_eq) * exp(-(cooling_w_per_k / thermal_capacity_j_per_k) * t)
    //   T_eq = ambient_temperature_k + heating_w / cooling_w_per_k
    // computed independently here from the documented probe defaults rather
    // than by re-deriving the implementation's own code.
    {
        SimulationCore thermal_core;
        const double initial_temperature = thermal_core.snapshot().temperature_k;
        const double thermal_capacity = thermal_core.snapshot().thermal_capacity_j_per_k;
        const double ambient = thermal_core.snapshot().ambient_temperature_k;
        const double cooling_w_per_k = thermal_core.snapshot().passive_cooling_w_per_k;

        thermal_core.allocate_power(everward::simulation::PowerSubsystem::Sensors, 100.0);
        thermal_core.allocate_power(everward::simulation::PowerSubsystem::Propulsion, 150.0);
        (void)thermal_core.drain_events();

        const double heating_w = 250.0;
        const double elapsed_s = 4.0;
        thermal_core.advance_wall_ticks(SimulationClock::TicksPerSecond * 4);

        const double equilibrium_k = ambient + heating_w / cooling_w_per_k;
        const double decay_rate_per_s = cooling_w_per_k / thermal_capacity;
        const double expected_temperature =
            equilibrium_k + (initial_temperature - equilibrium_k) * std::exp(-decay_rate_per_s * elapsed_s);
        assert(nearly_equal(thermal_core.snapshot().temperature_k, expected_temperature));
        // Under sustained heating the probe warms toward, but has not yet
        // reached, its equilibrium temperature.
        assert(thermal_core.snapshot().temperature_k > initial_temperature);
        assert(thermal_core.snapshot().temperature_k < equilibrium_k);
    }

    // ThermalLoad: accumulates across multiple fixed steps rather than only
    // reflecting the most recent one, tracks stored-energy consumption (both
    // driven by the same total_power_allocated_w() draw) without one
    // affecting the other's own state, and is step-size independent: two
    // steps totalling 5 seconds land at the same temperature as one 5-second
    // step, because the underlying integration is solved exactly rather than
    // by fixed-size Euler updates.
    {
        SimulationCore split_steps;
        SimulationCore single_step;
        const double initial_energy = split_steps.snapshot().stored_energy_j;

        split_steps.allocate_power(everward::simulation::PowerSubsystem::Computation, 60.0);
        single_step.allocate_power(everward::simulation::PowerSubsystem::Computation, 60.0);
        (void)split_steps.drain_events();
        (void)single_step.drain_events();

        split_steps.advance_wall_ticks(SimulationClock::TicksPerSecond * 2);
        split_steps.advance_wall_ticks(SimulationClock::TicksPerSecond * 3);
        single_step.advance_wall_ticks(SimulationClock::TicksPerSecond * 5);

        assert(nearly_equal(split_steps.snapshot().temperature_k, single_step.snapshot().temperature_k, 1e-6));
        assert(split_steps.snapshot().temperature_k > split_steps.snapshot().ambient_temperature_k);
        assert(nearly_equal(split_steps.snapshot().stored_energy_j, initial_energy - 60.0 * 5.0));
    }

    // ThermalLoad: passive cooling with zero allocated power. A probe
    // displaced above ambient (e.g. left over from prior heating) cools back
    // toward ambient_temperature_k over time even while drawing no power at
    // all, since Newtonian cooling depends only on the temperature
    // difference, not on any active heat source.
    {
        SimulationCore cooling_core;
        cooling_core.allocate_power(everward::simulation::PowerSubsystem::Propulsion, 400.0);
        (void)cooling_core.drain_events();
        cooling_core.advance_wall_ticks(SimulationClock::TicksPerSecond * 3600);
        cooling_core.allocate_power(everward::simulation::PowerSubsystem::Propulsion, 0.0);
        (void)cooling_core.drain_events();

        const double heated_temperature = cooling_core.snapshot().temperature_k;
        const double ambient = cooling_core.snapshot().ambient_temperature_k;
        assert(heated_temperature > ambient);

        cooling_core.advance_wall_ticks(SimulationClock::TicksPerSecond * 3600);
        const double cooled_temperature = cooling_core.snapshot().temperature_k;

        // Temperature moved strictly closer to ambient, and never overshoots
        // past it, even though this is an explicit-formula update rather
        // than a small fixed-size step.
        assert(cooled_temperature < heated_temperature);
        assert(cooled_temperature >= ambient);
    }

    // OverheatResponse: no-op case. With no allocated power, temperature_k
    // never approaches max_operating_temperature_k, so is_overheated stays
    // false and can_scan/can_thrust remain available.
    {
        SimulationCore overheat_core;
        overheat_core.advance_wall_ticks(SimulationClock::TicksPerSecond * 100000);
        assert(!overheat_core.snapshot().is_overheated);
        assert(overheat_core.snapshot().can_scan);
        assert(overheat_core.snapshot().can_thrust);

        auto events = overheat_core.drain_events();
        for (const auto& event : events) {
            assert(event.type != everward::simulation::DomainEventType::OverheatStarted);
        }
    }

    // OverheatResponse: happy path. Sustained maximum allocated power drives
    // temperature_k toward an equilibrium well above
    // max_operating_temperature_k. Advancing exactly to the analytically
    // computed crossing time (from the same closed-form cooling equation
    // integrate_thermal_load itself uses, rounded up to a whole tick as
    // EnergyConsumption's own exact-boundary test does) lands temperature_k
    // at or just past the limit and triggers exactly one OverheatStarted
    // transition that locks out can_scan/can_thrust, matching the
    // ScanCommand/allocate_power capability-gating pattern.
    {
        SimulationCore overheat_core;
        const double capacity_w = overheat_core.snapshot().power_capacity_w;
        const double thermal_capacity = overheat_core.snapshot().thermal_capacity_j_per_k;
        const double ambient = overheat_core.snapshot().ambient_temperature_k;
        const double cooling_w_per_k = overheat_core.snapshot().passive_cooling_w_per_k;
        const double max_temp = overheat_core.snapshot().max_operating_temperature_k;

        overheat_core.allocate_power(everward::simulation::PowerSubsystem::Propulsion, capacity_w);
        (void)overheat_core.drain_events();

        const double equilibrium_k = ambient + capacity_w / cooling_w_per_k;
        const double decay_rate_per_s = cooling_w_per_k / thermal_capacity;
        const double crossing_seconds =
            -std::log((max_temp - equilibrium_k) / (ambient - equilibrium_k)) / decay_rate_per_s;
        const auto crossing_ticks =
            static_cast<std::int64_t>(std::ceil(crossing_seconds * SimulationClock::TicksPerSecond));

        assert(overheat_core.snapshot().can_scan);
        assert(overheat_core.snapshot().can_thrust);

        overheat_core.advance_wall_ticks(crossing_ticks);
        assert(overheat_core.snapshot().temperature_k >= max_temp);
        assert(overheat_core.snapshot().is_overheated);
        assert(!overheat_core.snapshot().can_scan);
        assert(!overheat_core.snapshot().can_thrust);

        auto crossing_events = overheat_core.drain_events();
        std::size_t overheat_started_count = 0;
        for (const auto& event : crossing_events) {
            if (event.type == everward::simulation::DomainEventType::OverheatStarted) {
                ++overheat_started_count;
            }
        }
        assert(overheat_started_count == 1);

        // While overheated, scanning and thrust commands are rejected just
        // like any other capability-gated command.
        bool scan_rejected = false;
        try {
            overheat_core.start_scan("asteroid-1", 10.0);
        } catch (const std::runtime_error&) {
            scan_rejected = true;
        }
        assert(scan_rejected);

        bool thrust_rejected = false;
        try {
            overheat_core.set_velocity_mps(Vector3d{1.0, 0.0, 0.0});
        } catch (const std::runtime_error&) {
            thrust_rejected = true;
        }
        assert(thrust_rejected);

        // Remaining above the limit does not re-emit OverheatStarted: the
        // event marks the crossing transition, not a steady overheated state.
        overheat_core.advance_wall_ticks(SimulationClock::TicksPerSecond);
        auto steady_state_events = overheat_core.drain_events();
        for (const auto& event : steady_state_events) {
            assert(event.type != everward::simulation::DomainEventType::OverheatStarted);
        }
    }

    // OverheatResponse: recovery. Deallocating power lets temperature_k decay
    // back toward ambient; once it drops back below
    // max_operating_temperature_k, is_overheated clears exactly once via
    // OverheatEnded and can_scan/can_thrust are restored.
    {
        SimulationCore overheat_core;
        overheat_core.allocate_power(everward::simulation::PowerSubsystem::Propulsion,
                                      overheat_core.snapshot().power_capacity_w);
        overheat_core.advance_wall_ticks(SimulationClock::TicksPerSecond * 400000);
        assert(overheat_core.snapshot().is_overheated);
        (void)overheat_core.drain_events();

        overheat_core.allocate_power(everward::simulation::PowerSubsystem::Propulsion, 0.0);
        (void)overheat_core.drain_events();

        overheat_core.advance_wall_ticks(SimulationClock::TicksPerSecond * 400000);
        assert(overheat_core.snapshot().temperature_k < overheat_core.snapshot().max_operating_temperature_k);
        assert(!overheat_core.snapshot().is_overheated);
        assert(overheat_core.snapshot().can_scan);
        assert(overheat_core.snapshot().can_thrust);

        auto events = overheat_core.drain_events();
        std::size_t overheat_ended_count = 0;
        for (const auto& event : events) {
            if (event.type == everward::simulation::DomainEventType::OverheatEnded) {
                ++overheat_ended_count;
            }
        }
        assert(overheat_ended_count == 1);
    }

    // EnergyDepletionResponse: no-op case. With no allocated power,
    // stored_energy_j never reaches zero, so is_energy_depleted stays false
    // and can_scan/can_thrust remain available (mirrors OverheatResponse's
    // own no-op case).
    {
        SimulationCore depletion_core;
        depletion_core.advance_wall_ticks(SimulationClock::TicksPerSecond * 10);
        assert(!depletion_core.snapshot().is_energy_depleted);
        assert(depletion_core.snapshot().can_scan);
        assert(depletion_core.snapshot().can_thrust);
    }

    // EnergyDepletionResponse: happy path. Advancing exactly to the point
    // stored_energy_j is drawn down to zero (the same closed-form
    // watts * seconds accounting EnergyConsumption's own boundary test
    // uses, rounded up to a whole tick) sets is_energy_depleted, locks out
    // can_scan/can_thrust via the same capability derivation the overheat
    // response uses, and causes scan/thrust commands to be rejected, even
    // though temperature_k has not come anywhere near
    // max_operating_temperature_k in this scenario. Deliberately allocates
    // a modest 100 W (not the full power_capacity_w) so this test stays
    // isolated to the energy-depletion transition alone, for the same
    // equilibrium-below-threshold reason documented on EnergyConsumption's
    // boundary case above.
    {
        SimulationCore depletion_core;
        depletion_core.allocate_power(everward::simulation::PowerSubsystem::Computation, 100.0);
        (void)depletion_core.drain_events();

        const double capacity_w = 100.0;
        const double initial_energy = depletion_core.snapshot().stored_energy_j;
        const double exact_depletion_seconds = initial_energy / capacity_w;
        const auto exact_depletion_ticks =
            static_cast<std::int64_t>(std::ceil(exact_depletion_seconds * SimulationClock::TicksPerSecond));

        assert(depletion_core.snapshot().can_scan);
        assert(depletion_core.snapshot().can_thrust);

        depletion_core.advance_wall_ticks(exact_depletion_ticks);
        assert(nearly_equal(depletion_core.snapshot().stored_energy_j, 0.0, 1e-3));
        assert(depletion_core.snapshot().is_energy_depleted);
        assert(!depletion_core.snapshot().can_scan);
        assert(!depletion_core.snapshot().can_thrust);
        assert(!depletion_core.snapshot().is_overheated);

        bool scan_rejected = false;
        try {
            depletion_core.start_scan("asteroid-1", 10.0);
        } catch (const std::runtime_error&) {
            scan_rejected = true;
        }
        assert(scan_rejected);

        bool thrust_rejected = false;
        try {
            depletion_core.set_velocity_mps(Vector3d{1.0, 0.0, 0.0});
        } catch (const std::runtime_error&) {
            thrust_rejected = true;
        }
        assert(thrust_rejected);
    }

    // EnergyDepletionResponse / OverheatResponse interaction: when both
    // lockout causes are active at once and only one clears, capabilities
    // must stay locked rather than being wrongly restored by the recovering
    // cause. Sustained maximum allocated power drives the probe into both
    // is_overheated and is_energy_depleted; deallocating power then lets
    // temperature_k decay back below max_operating_temperature_k (firing
    // OverheatEnded) while stored_energy_j remains permanently at zero
    // (this probe's default energy_generation_w is 0.0, so there is no
    // active recharge source in this scenario — see the dedicated
    // EnergyGeneration recovery test above for the case where one exists),
    // so can_scan/can_thrust must remain false throughout.
    {
        SimulationCore combined_core;
        combined_core.allocate_power(everward::simulation::PowerSubsystem::Propulsion,
                                      combined_core.snapshot().power_capacity_w);
        // 700,000s comfortably exceeds both the ~666,667s energy-depletion
        // time (5.0e8 J / 750 W) and the much shorter overheat-crossing time
        // at this same sustained maximum wattage, so both lockout causes are
        // active by the time this call returns.
        combined_core.advance_wall_ticks(SimulationClock::TicksPerSecond * 700000);
        assert(combined_core.snapshot().is_overheated);
        assert(combined_core.snapshot().is_energy_depleted);
        assert(!combined_core.snapshot().can_scan);
        assert(!combined_core.snapshot().can_thrust);
        (void)combined_core.drain_events();

        combined_core.allocate_power(everward::simulation::PowerSubsystem::Propulsion, 0.0);
        (void)combined_core.drain_events();

        // 1,000,000s of passive cooling from here comfortably decays
        // temperature_k back below max_operating_temperature_k (the
        // analogous OverheatResponse recovery test only needed 400,000s
        // starting from just above the threshold; this scenario starts much
        // hotter, closer to the sustained-power equilibrium, so it needs
        // more cooling time to cross back down).
        combined_core.advance_wall_ticks(SimulationClock::TicksPerSecond * 1000000);
        assert(combined_core.snapshot().temperature_k < combined_core.snapshot().max_operating_temperature_k);
        assert(!combined_core.snapshot().is_overheated);

        // The overheat side recovered (and fired OverheatEnded exactly once,
        // matching OverheatResponse's own recovery test), but energy
        // depletion is still active, so capabilities must remain locked.
        auto events = combined_core.drain_events();
        std::size_t overheat_ended_count = 0;
        for (const auto& event : events) {
            assert(event.type != everward::simulation::DomainEventType::EnergyDepleted);
            if (event.type == everward::simulation::DomainEventType::OverheatEnded) {
                ++overheat_ended_count;
            }
        }
        assert(overheat_ended_count == 1);

        assert(combined_core.snapshot().is_energy_depleted);
        assert(nearly_equal(combined_core.snapshot().stored_energy_j, 0.0));
        assert(!combined_core.snapshot().can_scan);
        assert(!combined_core.snapshot().can_thrust);

        bool scan_still_rejected = false;
        try {
            combined_core.start_scan("asteroid-1", 10.0);
        } catch (const std::runtime_error&) {
            scan_still_rejected = true;
        }
        assert(scan_still_rejected);
    }

    // EnergyGeneration: validation. Negative generation is rejected,
    // mirroring allocate_power's own non-negative watts validation, and
    // leaves energy_generation_w at its default 0.0.
    {
        SimulationCore generation_core;

        bool negative_watts_threw = false;
        try {
            generation_core.set_energy_generation_w(-1.0);
        } catch (const std::invalid_argument&) {
            negative_watts_threw = true;
        }
        assert(negative_watts_threw);
        assert(nearly_equal(generation_core.snapshot().energy_generation_w, 0.0));
    }

    // EnergyGeneration: exact no-op when generation exactly offsets allocated
    // consumption. The net rate is precisely zero, so stored_energy_j does
    // not move even as simulated time advances, and neither EnergyDepleted
    // nor EnergyRestored fires.
    {
        SimulationCore balanced_core;
        balanced_core.set_energy_generation_w(100.0);
        balanced_core.allocate_power(everward::simulation::PowerSubsystem::Computation, 100.0);
        (void)balanced_core.drain_events();
        const double initial_energy = balanced_core.snapshot().stored_energy_j;

        balanced_core.advance_wall_ticks(SimulationClock::TicksPerSecond * 10);
        assert(nearly_equal(balanced_core.snapshot().stored_energy_j, initial_energy));

        auto events = balanced_core.drain_events();
        for (const auto& event : events) {
            assert(event.type != everward::simulation::DomainEventType::EnergyDepleted);
            assert(event.type != everward::simulation::DomainEventType::EnergyRestored);
        }
    }

    // EnergyGeneration: recovery. Draws stored_energy_j to zero with
    // generation below consumption (mirrors EnergyConsumption's own boundary
    // case, net-drain accounted), locking out can_scan/can_thrust via
    // is_energy_depleted, then drops consumption to zero so the same passive
    // generation that could not keep up now recharges stored_energy_j back
    // above zero: is_energy_depleted clears, can_scan/can_thrust are
    // restored, and EnergyRestored fires exactly once. This is the recovery
    // path the prior slice's EnergyDepletionResponse interaction test
    // explicitly named as not existing yet.
    {
        SimulationCore recovery_core;
        recovery_core.set_energy_generation_w(20.0);
        recovery_core.allocate_power(everward::simulation::PowerSubsystem::Computation, 100.0);
        (void)recovery_core.drain_events();

        const double net_drain_w = 100.0 - 20.0;
        const double initial_energy = recovery_core.snapshot().stored_energy_j;
        const double exact_depletion_seconds = initial_energy / net_drain_w;
        const auto exact_depletion_ticks =
            static_cast<std::int64_t>(std::ceil(exact_depletion_seconds * SimulationClock::TicksPerSecond));

        recovery_core.advance_wall_ticks(exact_depletion_ticks);
        assert(nearly_equal(recovery_core.snapshot().stored_energy_j, 0.0, 1e-3));
        assert(recovery_core.snapshot().is_energy_depleted);
        assert(!recovery_core.snapshot().can_scan);
        assert(!recovery_core.snapshot().can_thrust);

        auto depletion_events = recovery_core.drain_events();
        bool saw_energy_depleted = false;
        for (const auto& event : depletion_events) {
            assert(event.type != everward::simulation::DomainEventType::EnergyRestored);
            if (event.type == everward::simulation::DomainEventType::EnergyDepleted) {
                saw_energy_depleted = true;
            }
        }
        assert(saw_energy_depleted);

        // Dropping allocated power to zero leaves only the passive
        // generation source active: net rate is now +20 W, so
        // stored_energy_j recharges from the exact zero clamp above.
        recovery_core.allocate_power(everward::simulation::PowerSubsystem::Computation, 0.0);
        (void)recovery_core.drain_events();

        recovery_core.advance_wall_ticks(SimulationClock::TicksPerSecond);
        assert(nearly_equal(recovery_core.snapshot().stored_energy_j, 20.0));
        assert(!recovery_core.snapshot().is_energy_depleted);
        assert(recovery_core.snapshot().can_scan);
        assert(recovery_core.snapshot().can_thrust);

        auto recovery_events = recovery_core.drain_events();
        std::size_t energy_restored_count = 0;
        for (const auto& event : recovery_events) {
            if (event.type == everward::simulation::DomainEventType::EnergyRestored) {
                ++energy_restored_count;
            }
        }
        assert(energy_restored_count == 1);

        // Continuing to advance while net-positive does not re-emit
        // EnergyRestored: the event marks the 0 -> > 0 transition, not a
        // steady recharging state.
        recovery_core.advance_wall_ticks(SimulationClock::TicksPerSecond);
        auto steady_state_events = recovery_core.drain_events();
        for (const auto& event : steady_state_events) {
            assert(event.type != everward::simulation::DomainEventType::EnergyRestored);
        }
    }

    // EnergyGeneration: clamps at energy_capacity_j rather than charging past
    // it, mirroring the existing clamp-at-zero floor for depletion. An
    // absurdly high generation rate (equal to the whole energy_capacity_j,
    // i.e. a full recharge in under a second) makes the saturation
    // deterministic well within this test's short elapsed time.
    {
        SimulationCore capacity_core;
        const double capacity_j = capacity_core.snapshot().energy_capacity_j;
        capacity_core.set_energy_generation_w(capacity_j);

        capacity_core.advance_wall_ticks(SimulationClock::TicksPerSecond * 10);
        assert(nearly_equal(capacity_core.snapshot().stored_energy_j, capacity_j));
        assert(!capacity_core.snapshot().is_energy_depleted);

        // Remaining saturated across further steps does not overshoot past
        // energy_capacity_j or re-emit EnergyRestored (stored_energy_j was
        // already positive before this call, so no 0 -> > 0 transition
        // occurs).
        capacity_core.advance_wall_ticks(SimulationClock::TicksPerSecond * 10);
        assert(nearly_equal(capacity_core.snapshot().stored_energy_j, capacity_j));
        auto events = capacity_core.drain_events();
        for (const auto& event : events) {
            assert(event.type != everward::simulation::DomainEventType::EnergyRestored);
            assert(event.type != everward::simulation::DomainEventType::EnergyDepleted);
        }
    }

    // CanonicalProbe: make_canonical_ev0001() configures the canonical
    // EV-0001 probe's real hardware loadout (currently just its passive
    // generation source) without disturbing the bare SimulationCore()
    // default every other test in this file relies on.
    {
        // The bare default stays exactly generation-free.
        SimulationCore bare_core;
        assert(nearly_equal(bare_core.snapshot().energy_generation_w, 0.0));

        SimulationCore canonical_core = SimulationCore::make_canonical_ev0001();
        assert(canonical_core.snapshot().probe_id == "EV-0001");
        assert(canonical_core.tick() == 0);
        assert(nearly_equal(canonical_core.snapshot().energy_generation_w,
                             SimulationCore::kCanonicalEv0001EnergyGenerationW));
        assert(canonical_core.snapshot().energy_generation_w > 0.0);

        // The generation source is genuinely wired into the same energy
        // balance every other EnergyGeneration test exercises: with nothing
        // allocated, stored_energy_j rises by exactly generation_w * seconds.
        const double initial_energy = canonical_core.snapshot().stored_energy_j;
        canonical_core.advance_wall_ticks(SimulationClock::TicksPerSecond * 10);
        const double expected_energy =
            initial_energy + SimulationCore::kCanonicalEv0001EnergyGenerationW * 10.0;
        assert(nearly_equal(canonical_core.snapshot().stored_energy_j, expected_energy));

        auto events = canonical_core.drain_events();
        for (const auto& event : events) {
            assert(event.type != everward::simulation::DomainEventType::EnergyDepleted);
        }
    }

    std::cout << "Everward simulation core tests passed\n";
    return 0;
}
