#pragma once

#include "everward/simulation/types.hpp"

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
};

struct PlanetaryLocalFrame {
    Vector3d up{};
    Vector3d east{};
    Vector3d north{};
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

[[nodiscard]] inline bool is_below_reference_surface(
    Vector3d position_m,
    const SphericalPlanetaryBody& body) noexcept {
    return altitude_above_reference_surface(position_m, body) < 0.0;
}

} // namespace everward::simulation