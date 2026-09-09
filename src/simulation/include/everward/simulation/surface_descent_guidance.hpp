#pragma once

#include "everward/simulation/planetary_body.hpp"

#include <algorithm>
#include <cmath>

namespace everward::simulation {

// Deterministic Slice 10 command shaping for near-surface operations.
// This helper does not model thruster authority or move the probe. It only
// constrains a requested inertial velocity to the body-relative controlled-
// descent envelope already used by classify_surface_approach().
struct SurfaceApproachVelocityCommand {
    Vector3d velocity_mps{};
    bool descent_rate_limited{false};
    bool tangential_rate_limited{false};
};

[[nodiscard]] inline SurfaceApproachVelocityCommand constrain_surface_approach_velocity(
    Vector3d position_m,
    Vector3d requested_velocity_mps,
    const SphericalPlanetaryBody& body,
    const ControlledDescentEnvelope& envelope = {}) noexcept {
    const Vector3d up = local_surface_normal(position_m, body);
    Vector3d relative = body_relative_velocity(requested_velocity_mps, body);

    double radial_speed_mps = planetary_dot(relative, up);
    Vector3d tangential_velocity_mps = planetary_subtract(
        relative,
        planetary_scale(up, radial_speed_mps)
    );

    bool descent_rate_limited = false;
    const double max_descent_speed_mps = std::fabs(envelope.max_descent_speed_mps);
    if (radial_speed_mps < -max_descent_speed_mps) {
        radial_speed_mps = -max_descent_speed_mps;
        descent_rate_limited = true;
    }

    bool tangential_rate_limited = false;
    const double max_tangential_speed_mps = std::fabs(envelope.max_tangential_speed_mps);
    const double tangential_speed_mps = planetary_magnitude(tangential_velocity_mps);
    if (tangential_speed_mps > max_tangential_speed_mps && tangential_speed_mps > 1e-12) {
        tangential_velocity_mps = planetary_scale(
            tangential_velocity_mps,
            max_tangential_speed_mps / tangential_speed_mps
        );
        tangential_rate_limited = true;
    }

    const Vector3d constrained_relative{
        up.x * radial_speed_mps + tangential_velocity_mps.x,
        up.y * radial_speed_mps + tangential_velocity_mps.y,
        up.z * radial_speed_mps + tangential_velocity_mps.z,
    };
    const Vector3d constrained_velocity{
        body.velocity_mps.x + constrained_relative.x,
        body.velocity_mps.y + constrained_relative.y,
        body.velocity_mps.z + constrained_relative.z,
    };

    return {
        constrained_velocity,
        descent_rate_limited,
        tangential_rate_limited,
    };
}

} // namespace everward::simulation
