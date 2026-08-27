#include "everward/simulation/software_policy.hpp"

#undef NDEBUG
#include <cassert>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

using everward::simulation::DomainEvent;
using everward::simulation::DomainEventType;
using everward::simulation::ProbeRuntime;
using everward::simulation::SimulationClock;
using everward::simulation::StaticSphereBody;
using everward::simulation::Vector3d;

static bool nearly_equal(double a, double b, double eps = 1e-6) {
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

int main() {
    // Physical-body registration validates stable identity and geometry.
    {
        ProbeRuntime runtime;
        runtime.add_static_sphere_body({"asteroid-a", {10.0, 0.0, 0.0}, 2.0});
        assert(runtime.static_bodies().size() == 1);

        bool duplicate_threw = false;
        try {
            runtime.add_static_sphere_body({"asteroid-a", {20.0, 0.0, 0.0}, 3.0});
        } catch (const std::invalid_argument&) {
            duplicate_threw = true;
        }
        assert(duplicate_threw);

        bool bad_radius_threw = false;
        try {
            runtime.add_static_sphere_body({"bad", {0.0, 0.0, 0.0}, 0.0});
        } catch (const std::invalid_argument&) {
            bad_radius_threw = true;
        }
        assert(bad_radius_threw);
    }

    // A fast direct approach uses a swept sphere test, so the probe cannot
    // tunnel through the body even when one simulation step would otherwise
    // carry it completely past the far side.
    {
        ProbeRuntime runtime;
        runtime.add_static_sphere_body({"contact-body", {50.0, 0.0, 0.0}, 2.0});
        runtime.set_velocity_mps({1000.0, 0.0, 0.0});
        (void)runtime.drain_events();

        runtime.advance_wall_ticks(SimulationClock::TicksPerSecond);
        const auto& state = runtime.snapshot();

        const double expected_center_x = 50.0 - 2.0 - state.collision_envelope_radius_m;
        assert(nearly_equal(state.position_m.x, expected_center_x, 2e-6));
        assert(nearly_equal(state.position_m.y, 0.0));
        assert(nearly_equal(state.velocity_mps.x, 0.0));
        assert(state.has_contact_history);
        assert(state.last_contact_body_id == "contact-body");
        assert(nearly_equal(state.last_contact_point_m.x, 48.0));
        assert(nearly_equal(state.last_contact_surface_normal.x, -1.0));
        assert(nearly_equal(state.last_contact_relative_velocity_mps.x, 1000.0));
        assert(nearly_equal(state.last_contact_normal_speed_mps, 1000.0));
        assert(state.last_contact_tick == runtime.tick());

        const auto events = runtime.drain_events();
        assert(has_event(events, DomainEventType::Contact));
    }

    // Contact removes only inward normal velocity. A glancing impact keeps a
    // tangential component so the foundation behaves like physical contact,
    // not an arbitrary full-stop trigger.
    {
        ProbeRuntime runtime;
        runtime.add_static_sphere_body({"glance", {10.0, 0.0, 0.0}, 2.0});
        runtime.set_velocity_mps({20.0, 4.0, 0.0});
        (void)runtime.drain_events();
        runtime.advance_wall_ticks(SimulationClock::TicksPerSecond);

        const auto& state = runtime.snapshot();
        assert(state.has_contact_history);
        assert(state.last_contact_body_id == "glance");
        assert(state.last_contact_normal_speed_mps > 0.0);
        assert(std::fabs(state.velocity_mps.y) > 0.01);
        const double residual_speed = std::sqrt(
            state.velocity_mps.x * state.velocity_mps.x +
            state.velocity_mps.y * state.velocity_mps.y +
            state.velocity_mps.z * state.velocity_mps.z);
        assert(residual_speed > 0.01);
        assert(residual_speed < std::sqrt(20.0 * 20.0 + 4.0 * 4.0));
    }

    // After a direct stop, commanding motion away from the surface succeeds
    // and does not generate a second false contact event.
    {
        ProbeRuntime runtime;
        runtime.add_static_sphere_body({"depart", {10.0, 0.0, 0.0}, 2.0});
        runtime.set_velocity_mps({20.0, 0.0, 0.0});
        (void)runtime.drain_events();
        runtime.advance_wall_ticks(SimulationClock::TicksPerSecond);
        (void)runtime.drain_events();

        const double contact_x = runtime.snapshot().position_m.x;
        runtime.set_velocity_mps({-5.0, 0.0, 0.0});
        (void)runtime.drain_events();
        runtime.advance_wall_ticks(SimulationClock::TicksPerSecond);
        const auto events = runtime.drain_events();

        assert(runtime.snapshot().position_m.x < contact_x - 4.9);
        assert(!has_event(events, DomainEventType::Contact));
    }

    std::cout << "Physical contact foundation tests passed\n";
    return 0;
}
