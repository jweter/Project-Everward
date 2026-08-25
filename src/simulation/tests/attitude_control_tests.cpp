#include "everward/simulation/core.hpp"

#undef NDEBUG
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

using everward::simulation::DomainEventType;
using everward::simulation::EulerAttitudeDegrees;
using everward::simulation::PowerSubsystem;
using everward::simulation::SimulationCore;
using everward::simulation::Vector3d;

namespace {
bool nearly_equal(double left, double right, double tolerance = 1e-9) {
    return std::fabs(left - right) <= tolerance;
}
}

int main() {
    // EV-0001 begins aligned with world +X while attitude remains explicit
    // authoritative state rather than an Unreal-only presentation rotation.
    {
        SimulationCore core;
        const auto& attitude = core.snapshot().attitude_degrees;
        assert(nearly_equal(attitude.yaw, 0.0));
        assert(nearly_equal(attitude.pitch, 0.0));
        assert(nearly_equal(attitude.roll, 0.0));
    }

    // Yaw rotates local forward into world +Y using the same convention as
    // Unreal FRotator(Pitch, Yaw, Roll).
    {
        SimulationCore core;
        core.adjust_attitude_degrees(EulerAttitudeDegrees{90.0, 0.0, 0.0});
        core.adjust_local_velocity_mps(Vector3d{1.0, 0.0, 0.0});
        const auto& velocity = core.snapshot().velocity_mps;
        assert(nearly_equal(velocity.x, 0.0));
        assert(nearly_equal(velocity.y, 1.0));
        assert(nearly_equal(velocity.z, 0.0));

        const auto events = core.drain_events();
        assert(events.size() == 2);
        assert(events[0].type == DomainEventType::AttitudeChanged);
        assert(events[1].type == DomainEventType::ManeuverStarted);
    }

    // Positive pitch sends local forward toward world +Z.
    {
        SimulationCore core;
        core.adjust_attitude_degrees(EulerAttitudeDegrees{0.0, 90.0, 0.0});
        core.adjust_local_velocity_mps(Vector3d{2.0, 0.0, 0.0});
        const auto& velocity = core.snapshot().velocity_mps;
        assert(nearly_equal(velocity.x, 0.0));
        assert(nearly_equal(velocity.y, 0.0));
        assert(nearly_equal(velocity.z, 2.0));
    }

    // Positive roll rotates local right toward world -Z.
    {
        SimulationCore core;
        core.adjust_attitude_degrees(EulerAttitudeDegrees{0.0, 0.0, 90.0});
        core.adjust_local_velocity_mps(Vector3d{0.0, 1.0, 0.0});
        const auto& velocity = core.snapshot().velocity_mps;
        assert(nearly_equal(velocity.x, 0.0));
        assert(nearly_equal(velocity.y, 0.0));
        assert(nearly_equal(velocity.z, -1.0));
    }

    // Repeated trims are deterministic, additive, and normalized to the
    // compact signed-degree range used by telemetry.
    {
        SimulationCore core;
        core.adjust_attitude_degrees(EulerAttitudeDegrees{200.0, -190.0, 540.0});
        const auto& attitude = core.snapshot().attitude_degrees;
        assert(nearly_equal(attitude.yaw, -160.0));
        assert(nearly_equal(attitude.pitch, 170.0));
        assert(nearly_equal(attitude.roll, -180.0));

        core.adjust_local_velocity_mps(Vector3d{0.5, 0.0, 0.0});
        const Vector3d first = core.snapshot().velocity_mps;
        core.adjust_local_velocity_mps(Vector3d{0.5, 0.0, 0.0});
        const Vector3d second = core.snapshot().velocity_mps;
        assert(nearly_equal(second.x, first.x * 2.0));
        assert(nearly_equal(second.y, first.y * 2.0));
        assert(nearly_equal(second.z, first.z * 2.0));
    }

    // Full stop remains the existing absolute authoritative command.
    {
        SimulationCore core;
        core.adjust_attitude_degrees(EulerAttitudeDegrees{45.0, 10.0, -5.0});
        core.adjust_local_velocity_mps(Vector3d{3.0, 2.0, 1.0});
        core.set_velocity_mps(Vector3d{});
        const auto& velocity = core.snapshot().velocity_mps;
        assert(nearly_equal(velocity.x, 0.0));
        assert(nearly_equal(velocity.y, 0.0));
        assert(nearly_equal(velocity.z, 0.0));
    }

    // Attitude and local translation use the same propulsion availability
    // gate as the pre-existing world-space velocity command.
    {
        SimulationCore core;
        core.set_subsystem_operational(PowerSubsystem::Propulsion, false);

        bool attitude_rejected = false;
        try {
            core.adjust_attitude_degrees(EulerAttitudeDegrees{1.0, 0.0, 0.0});
        } catch (const std::runtime_error&) {
            attitude_rejected = true;
        }
        assert(attitude_rejected);

        bool movement_rejected = false;
        try {
            core.adjust_local_velocity_mps(Vector3d{1.0, 0.0, 0.0});
        } catch (const std::runtime_error&) {
            movement_rejected = true;
        }
        assert(movement_rejected);
    }

    // Non-finite commands cannot poison deterministic persisted state.
    {
        SimulationCore core;
        bool invalid_attitude_rejected = false;
        try {
            core.adjust_attitude_degrees(EulerAttitudeDegrees{
                std::numeric_limits<double>::infinity(), 0.0, 0.0});
        } catch (const std::invalid_argument&) {
            invalid_attitude_rejected = true;
        }
        assert(invalid_attitude_rejected);

        bool invalid_velocity_rejected = false;
        try {
            core.adjust_local_velocity_mps(Vector3d{
                0.0, std::numeric_limits<double>::quiet_NaN(), 0.0});
        } catch (const std::invalid_argument&) {
            invalid_velocity_rejected = true;
        }
        assert(invalid_velocity_rejected);
    }

    std::cout << "Everward attitude control tests passed\n";
    return 0;
}
