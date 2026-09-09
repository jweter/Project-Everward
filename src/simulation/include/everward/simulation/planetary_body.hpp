#pragma once

#include "everward/simulation/types.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace everward::simulation {

// Phase 2 Slice 9 spherical-body foundation. This module is deliberately
// engine-independent: it defines the geometric read model needed for altitude,
// local up/horizon orientation, and body-relative motion without allowing
// Unreal presentation code to own planetary mechanical truth.
struct SphericalPlanetaryBody {
    std::string body_id;
    Vector3d center_m{};
    double radius_m{1.0};
    Vector3d velocity_mps{};
    // Standard gravitational parameter GM in m^3/s^2. Zero preserves the
    // existing non-gravitating test-body behavior for current aggregate
    // initializers and presentation-only reference bodies.
    double gravitational_parameter_m3_s2{0.0};
};

struct PlanetaryLocalFrame {
    Vector3d up{};
    Vector3d east{};
    Vector3d north{};
};

struct OrbitalContext {
    double radius_from_center_m{0.0};
    double radial_speed_mps{0.0};
    double tangential_speed_mps{0.0};
    double circular_orbit_speed_mps{0.0};
    bool gravity_enabled{false};
};

// Phase 2 Slice 10 read model for near-surface operations. Keeping the
// decomposition in the deterministic simulation layer lets presentation code
// consume vertical and tangential motion without redefining planetary truth.
struct SurfaceRelativeMotion {
    Vector3d body_relative_velocity_mps{};
    Vector3d vertical_velocity_mps{};
    Vector3d tangential_velocity_mps{};
    double vertical_speed_mps{0.0};
    double tangential_speed_mps{0.0};
};

struct ControlledDescentEnvelope {
    double max_descent_speed_mps{5.0};
    double max_tangential_speed_mps{2.0};
    double minimum_clearance_m{0.0};
};

struct SurfaceContactResolution {
    Vector3d position_m{};
    Vector3d velocity_mps{};
    bool corrected{false};
    double penetration_depth_m{0.0};
};

enum class SurfaceApproachState {
    Clear,
    ControlledDescent,
    ExcessiveDescentRate,
    ExcessiveTangentialRate,
    SurfacePenetration,
};

[[nodiscard]] inline Vector3d planetary_subtract(Vector3d a, Vector3d b) noexcept {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

[[nodiscard]] inline Vector3d planetary_scale(Vector3d value, double scalar) noexcept {
    return {value.x * scalar, value.y * scalar, value.z * scalar};
}

[[nodiscard]] inline double planetary_dot(Vector3d a, Vector3d b) noexcept {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

[[nodiscard]] inline Vector3d planetary_cross(Vector3d a, Vector3d b) noexcept {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x,
    };
}

[[nodiscard]] inline double planetary_magnitude(Vector3d value) noexcept {
    return std::sqrt(planetary_dot(value, value));
}

[[nodiscard]] inline Vector3d planetary_normalized_or_x(Vector3d value) noexcept {
    const double magnitude = planetary_magnitude(value);
    if (magnitude <= 1e-12) {
        return {1.0, 0.0, 0.0};
    }
    return planetary_scale(value, 1.0 / magnitude);
}

[[nodiscard]] inline double altitude_above_reference_surface(
    Vector3d position_m,
    const SphericalPlanetaryBody& body) noexcept {
    return planetary_magnitude(planetary_subtract(position_m, body.center_m)) - body.radius_m;
}

[[nodiscard]] inline Vector3d local_surface_normal(
    Vector3d position_m,
    const SphericalPlanetaryBody& body) noexcept {
    return planetary_normalized_or_x(planetary_subtract(position_m, body.center_m));
}

[[nodiscard]] inline PlanetaryLocalFrame local_horizon_frame(
    Vector3d position_m,
    const SphericalPlanetaryBody& body) noexcept {
    const Vector3d up = local_surface_normal(position_m, body);

    // Project the global +Z axis into the tangent plane. This keeps the basis
    // continuous at all non-polar latitudes instead of switching reference
    // axes at an arbitrary threshold. Only the true polar singularity uses a
    // deterministic +Y fallback.
    const Vector3d global_z{0.0, 0.0, 1.0};
    const Vector3d north_candidate = planetary_subtract(
        global_z,
        planetary_scale(up, planetary_dot(global_z, up)));
    const double north_magnitude = planetary_magnitude(north_candidate);
    const Vector3d north = north_magnitude <= 1e-12
        ? Vector3d{0.0, up.z >= 0.0 ? 1.0 : -1.0, 0.0}
        : planetary_scale(north_candidate, 1.0 / north_magnitude);
    const Vector3d east = planetary_normalized_or_x(planetary_cross(north, up));
    return {up, east, north};
}

[[nodiscard]] inline Vector3d body_relative_velocity(
    Vector3d object_velocity_mps,
    const SphericalPlanetaryBody& body) noexcept {
    return planetary_subtract(object_velocity_mps, body.velocity_mps);
}

// Deterministic point-mass gravity outside the body's center. This is a
// read-model calculation only; callers decide if/when an authoritative
// integration step applies the acceleration. Non-positive/non-finite GM and
// the exact center fail safely to zero rather than manufacturing a direction.
[[nodiscard]] inline Vector3d gravitational_acceleration(
    Vector3d position_m,
    const SphericalPlanetaryBody& body) noexcept {
    const double mu = body.gravitational_parameter_m3_s2;
    if (!std::isfinite(mu) || mu <= 0.0) {
        return {};
    }
    const Vector3d toward_center = planetary_subtract(body.center_m, position_m);
    const double radius_squared = planetary_dot(toward_center, toward_center);
    if (radius_squared <= 1e-12) {
        return {};
    }
    const double radius = std::sqrt(radius_squared);
    return planetary_scale(toward_center, mu / (radius_squared * radius));
}

[[nodiscard]] inline SurfaceRelativeMotion surface_relative_motion(
    Vector3d position_m,
    Vector3d object_velocity_mps,
    const SphericalPlanetaryBody& body) noexcept {
    const Vector3d relative = body_relative_velocity(object_velocity_mps, body);
    const Vector3d up = local_surface_normal(position_m, body);
    const double vertical_speed = planetary_dot(relative, up);
    const Vector3d vertical = planetary_scale(up, vertical_speed);
    const Vector3d tangential = planetary_subtract(relative, vertical);
    return {
        relative,
        vertical,
        tangential,
        vertical_speed,
        planetary_magnitude(tangential),
    };
}

[[nodiscard]] inline OrbitalContext orbital_context(
    Vector3d position_m,
    Vector3d object_velocity_mps,
    const SphericalPlanetaryBody& body) noexcept {
    const Vector3d offset = planetary_subtract(position_m, body.center_m);
    const double radius = planetary_magnitude(offset);
    const SurfaceRelativeMotion motion = surface_relative_motion(position_m, object_velocity_mps, body);
    const double mu = body.gravitational_parameter_m3_s2;
    const bool gravity_enabled = std::isfinite(mu) && mu > 0.0 && radius > 1e-12;
    const double circular_speed = gravity_enabled ? std::sqrt(mu / radius) : 0.0;
    return {
        radius,
        motion.vertical_speed_mps,
        motion.tangential_speed_mps,
        circular_speed,
        gravity_enabled,
    };
}

[[nodiscard]] inline SurfaceContactResolution resolve_surface_contact(
    Vector3d position_m, Vector3d velocity_mps,
    const SphericalPlanetaryBody& body, double minimum_clearance_m = 0.0) noexcept {
    const double clearance_m = std::max(0.0, minimum_clearance_m);
    const double altitude_m = altitude_above_reference_surface(position_m, body);
    if (altitude_m >= clearance_m) return {position_m, velocity_mps, false, 0.0};
    const Vector3d normal = local_surface_normal(position_m, body);
    const double radius_m = body.radius_m + clearance_m;
    position_m = {body.center_m.x + normal.x * radius_m, body.center_m.y + normal.y * radius_m, body.center_m.z + normal.z * radius_m};
    Vector3d relative = body_relative_velocity(velocity_mps, body);
    const double normal_speed = planetary_dot(relative, normal);
    if (normal_speed < 0.0) relative = planetary_subtract(relative, planetary_scale(normal, normal_speed));
    velocity_mps = {body.velocity_mps.x + relative.x, body.velocity_mps.y + relative.y, body.velocity_mps.z + relative.z};
    return {position_m, velocity_mps, true, clearance_m - altitude_m};
}

[[nodiscard]] inline SurfaceApproachState classify_surface_approach(
    Vector3d position_m,
    Vector3d object_velocity_mps,
    const SphericalPlanetaryBody& body,
    const ControlledDescentEnvelope& envelope = {}) noexcept {
    const double altitude = altitude_above_reference_surface(position_m, body);
    if (altitude < envelope.minimum_clearance_m) {
        return SurfaceApproachState::SurfacePenetration;
    }

    const SurfaceRelativeMotion motion = surface_relative_motion(position_m, object_velocity_mps, body);
    if (motion.vertical_speed_mps < -std::fabs(envelope.max_descent_speed_mps)) {
        return SurfaceApproachState::ExcessiveDescentRate;
    }
    if (motion.tangential_speed_mps > std::fabs(envelope.max_tangential_speed_mps)) {
        return SurfaceApproachState::ExcessiveTangentialRate;
    }
    if (motion.vertical_speed_mps < 0.0) {
        return SurfaceApproachState::ControlledDescent;
    }
    return SurfaceApproachState::Clear;
}

[[nodiscard]] inline bool is_below_reference_surface(
    Vector3d position_m,
    const SphericalPlanetaryBody& body) noexcept {
    return altitude_above_reference_surface(position_m, body) < 0.0;
}

} // namespace everward::simulation
