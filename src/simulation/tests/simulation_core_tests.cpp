#include "everward/simulation/core.hpp"

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

    std::cout << "Everward simulation core tests passed\n";
    return 0;
}
