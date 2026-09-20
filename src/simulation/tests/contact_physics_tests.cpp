#include "everward/simulation/software_policy.hpp"

#undef NDEBUG
#include <cassert>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

using everward::simulation::DomainEvent;
using everward::simulation::DomainEventType;
using everward::simulation::ProbeCompoundCollisionEnvelope;
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

        // A nose-first approach is stopped by the compound envelope's forward
        // hull sample, not a single oversized bounding sphere: the resolved
        // probe root sits one sample-radius plus that sample's forward local
        // offset away from the body surface, rather than one giant radius.
        const ProbeCompoundCollisionEnvelope envelope{};
        const auto& nose_sample = envelope.samples[0];
        const double expected_center_x =
            50.0 - 2.0 - nose_sample.radius_m - nose_sample.local_center_m.x;
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

    // A registered body's own motion must not be ignored by the swept
    // contact test: a fast body crossing all the way through a *stationary*
    // probe within one tick is still caught, not tunnelled through. Before
    // this fix the sweep tested the probe's own (here zero-length) path
    // against the body's already-advanced end-of-tick position only, so a
    // body that started 500 m away on one side and ended 500 m away on the
    // other side -- passing directly through the probe in between -- was
    // never detected at all.
    {
        ProbeRuntime runtime;
        runtime.add_static_sphere_body(
            StaticSphereBody{"ram", {-500.0, 0.0, 0.0}, 3.0, "", 0.0, {1000.0, 0.0, 0.0}});
        // Probe stays at rest; only the registered body moves this tick.
        (void)runtime.drain_events();

        runtime.advance_wall_ticks(SimulationClock::TicksPerSecond);
        const auto& state = runtime.snapshot();
        const auto events = runtime.drain_events();

        assert(state.has_contact_history);
        assert(state.last_contact_body_id == "ram");
        assert(has_event(events, DomainEventType::Contact));
        // Impact speed/relative velocity must come from the body's own
        // motion (the probe's own absolute velocity is zero throughout).
        assert(state.last_contact_normal_speed_mps > 900.0);
        assert(state.last_contact_relative_velocity_mps.x < -900.0);
        // The resolved probe position stays near where contact actually
        // happened (close to the origin the stationary probe never left),
        // not snapped next to the body's eventual resting place 500 m away
        // on the far side.
        assert(std::fabs(state.position_m.x) < 50.0);
    }

    // The companion failure mode the same defect produced: a body that is
    // *already* touching the probe at the start of a tick and moving
    // further in must still register as a genuine approach. The stale
    // implementation classified "already touching, moving away?" using only
    // the (here zero) probe velocity, so it always read as "moving away"
    // and silently discarded a real, ongoing collision.
    {
        ProbeRuntime runtime;
        // Exactly touching the aft hull sample (local {-5,0,0}, radius 1.50)
        // from the -x side: combined radius 1.0 + 1.50 = 2.50.
        runtime.add_static_sphere_body(
            StaticSphereBody{"charging", {-7.5, 0.0, 0.0}, 1.0, "", 0.0, {1000.0, 0.0, 0.0}});
        (void)runtime.drain_events();

        runtime.advance_wall_ticks(SimulationClock::TicksPerSecond);
        const auto& state = runtime.snapshot();
        const auto events = runtime.drain_events();

        assert(state.has_contact_history);
        assert(state.last_contact_body_id == "charging");
        assert(has_event(events, DomainEventType::Contact));
        assert(state.last_contact_normal_speed_mps > 900.0);
        assert(std::fabs(state.position_m.x) < 5.0);
    }

    // The reverse case proves the fix is not simply "always contact": a
    // body already touching the probe but pulling directly away faster than
    // the probe could ever be said to be catching it must not manufacture a
    // spurious contact. This also has to read the body's own motion, not
    // just the (here zero) probe velocity, to correctly classify separation.
    {
        ProbeRuntime runtime;
        runtime.add_static_sphere_body(
            StaticSphereBody{"retreating", {-7.5, 0.0, 0.0}, 1.0, "", 0.0, {-1000.0, 0.0, 0.0}});
        (void)runtime.drain_events();

        runtime.advance_wall_ticks(SimulationClock::TicksPerSecond);
        const auto events = runtime.drain_events();

        assert(!runtime.snapshot().has_contact_history);
        assert(!has_event(events, DomainEventType::Contact));
    }

    std::cout << "Physical contact foundation tests passed\n";
    return 0;
}
