#pragma once

#include "everward/simulation/planetary_body.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>

namespace everward::simulation {

// Deterministic Slice 10 command shaping for near-surface operations.
// This helper does not model thruster authority or move the probe. It only
// constrains a requested inertial velocity to a body-relative controlled-
// descent envelope and, when configured, reduces descent speed as clearance
// is approached. At minimum clearance it prevents a commanded inward velocity;
// contact resolution remains authoritative for penetration correction.
struct SurfaceApproachVelocityCommand {
    Vector3d velocity_mps{};
    bool descent_rate_limited{false};
    bool tangential_rate_limited{false};
};

struct AltitudeAwareDescentProfile {
    double full_speed_altitude_m{25.0};
    double touchdown_descent_speed_mps{0.5};
};

struct SurfaceHoverProfile {
    double target_altitude_m{5.0};
    double altitude_gain_per_second{0.5};
    double max_vertical_correction_speed_mps{2.0};
};

[[nodiscard]] inline double altitude_limited_descent_speed_mps(
    Vector3d position_m,
    const SphericalPlanetaryBody& body,
    const ControlledDescentEnvelope& envelope,
    const AltitudeAwareDescentProfile& profile) noexcept {
    const double max_speed = std::fabs(envelope.max_descent_speed_mps);
    const double touchdown_speed = std::clamp(
        std::fabs(profile.touchdown_descent_speed_mps), 0.0, max_speed
    );
    const double clearance = std::max(0.0, envelope.minimum_clearance_m);
    const double altitude = altitude_above_reference_surface(position_m, body) - clearance;
    const double clearance_tolerance = std::max(
        1e-12,
        8.0 * std::numeric_limits<double>::epsilon()
            * std::max({1.0, std::fabs(body.radius_m), clearance})
    );
    if (altitude <= clearance_tolerance) {
        return 0.0;
    }
    const double full_speed_altitude = std::max(0.0, profile.full_speed_altitude_m);
    if (full_speed_altitude <= 1e-12 || altitude >= full_speed_altitude) {
        return max_speed;
    }
    const double blend = altitude / full_speed_altitude;
    return touchdown_speed + (max_speed - touchdown_speed) * blend;
}

[[nodiscard]] inline SurfaceApproachVelocityCommand constrain_surface_approach_velocity(
    Vector3d position_m,
    Vector3d requested_velocity_mps,
    const SphericalPlanetaryBody& body,
    const ControlledDescentEnvelope& envelope = {},
    const AltitudeAwareDescentProfile& profile = {}) noexcept {
    const Vector3d up = local_surface_normal(position_m, body);
    Vector3d relative = body_relative_velocity(requested_velocity_mps, body);

    double radial_speed_mps = planetary_dot(relative, up);
    Vector3d tangential_velocity_mps = planetary_subtract(
        relative,
        planetary_scale(up, radial_speed_mps)
    );

    bool descent_rate_limited = false;
    const double max_descent_speed_mps = altitude_limited_descent_speed_mps(
        position_m, body, envelope, profile
    );
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

// Deterministic hover command shaping for Slice 10. The caller still owns
// propulsion/thruster authority; this function only requests a body-relative
// radial correction toward a target altitude while preserving the caller's
// tangential translation request inside the same controlled-descent envelope.
[[nodiscard]] inline SurfaceApproachVelocityCommand constrain_surface_hover_velocity(
    Vector3d position_m,
    Vector3d requested_velocity_mps,
    const SphericalPlanetaryBody& body,
    const ControlledDescentEnvelope& envelope = {},
    const SurfaceHoverProfile& hover = {}) noexcept {
    const Vector3d up = local_surface_normal(position_m, body);
    const Vector3d relative = body_relative_velocity(requested_velocity_mps, body);
    Vector3d tangential_velocity_mps = planetary_subtract(
        relative,
        planetary_scale(up, planetary_dot(relative, up))
    );

    const double max_tangential_speed_mps = std::fabs(envelope.max_tangential_speed_mps);
    const double tangential_speed_mps = planetary_magnitude(tangential_velocity_mps);
    bool tangential_rate_limited = false;
    if (tangential_speed_mps > max_tangential_speed_mps && tangential_speed_mps > 1e-12) {
        tangential_velocity_mps = planetary_scale(
            tangential_velocity_mps,
            max_tangential_speed_mps / tangential_speed_mps
        );
        tangential_rate_limited = true;
    }

    const double target_altitude_m = std::max(
        std::max(0.0, envelope.minimum_clearance_m), hover.target_altitude_m
    );
    const double altitude_error_m = target_altitude_m
        - altitude_above_reference_surface(position_m, body);
    const double max_correction_mps = std::fabs(hover.max_vertical_correction_speed_mps);
    const double correction_gain = std::max(0.0, hover.altitude_gain_per_second);
    const double desired_radial_speed_mps = altitude_error_m * correction_gain;
    const double radial_speed_mps = std::clamp(
        desired_radial_speed_mps, -max_correction_mps, max_correction_mps
    );
    // Whether the proportional altitude correction was actually clamped to
    // max_vertical_correction_speed_mps -- not merely whether a correction
    // was applied at all, which would be true almost anywhere off the exact
    // target altitude. Matches constrain_surface_approach_velocity()'s
    // descent_rate_limited semantics (compares the requested value against
    // its post-clamp result), so a caller reading "limited" can rely on the
    // same meaning for both descent and hover.
    const bool vertical_correction_limited =
        std::fabs(radial_speed_mps - desired_radial_speed_mps) > 1e-12;

    const Vector3d constrained_relative{
        up.x * radial_speed_mps + tangential_velocity_mps.x,
        up.y * radial_speed_mps + tangential_velocity_mps.y,
        up.z * radial_speed_mps + tangential_velocity_mps.z,
    };
    return {
        {
            body.velocity_mps.x + constrained_relative.x,
            body.velocity_mps.y + constrained_relative.y,
            body.velocity_mps.z + constrained_relative.z,
        },
        vertical_correction_limited,
        tangential_rate_limited,
    };
}

[[nodiscard]] inline std::optional<SurfaceApproachVelocityCommand> controlled_hover_velocity_command(
    Vector3d position_m,
    Vector3d requested_velocity_mps,
    const std::optional<SphericalPlanetaryBody>& body,
    const ControlledDescentEnvelope& envelope = {},
    const SurfaceHoverProfile& hover = {}) noexcept {
    if (!body.has_value()) {
        return std::nullopt;
    }
    return constrain_surface_hover_velocity(position_m, requested_velocity_mps, *body, envelope, hover);
}

// Convenience overload matching jose_autopilot.hpp's jose_guidance_command_for_body
// fail-closed pattern: reasons about SimulationCore's optional registered
// planetary body (core_.planetary_body()) directly rather than requiring the
// caller to already have unwrapped it, so the adapter boundary that wires
// this into a player command can fail closed the same way GetJoseGuidanceCommand
// does for an unregistered destination, instead of fabricating a descent
// envelope with no planetary body to descend toward.
[[nodiscard]] inline std::optional<SurfaceApproachVelocityCommand> controlled_descent_velocity_command(
    Vector3d position_m,
    Vector3d requested_velocity_mps,
    const std::optional<SphericalPlanetaryBody>& body,
    const ControlledDescentEnvelope& envelope = {},
    const AltitudeAwareDescentProfile& profile = {}) noexcept {
    if (!body.has_value()) {
        return std::nullopt;
    }
    return constrain_surface_approach_velocity(position_m, requested_velocity_mps, *body, envelope, profile);
}

} // namespace everward::simulation
