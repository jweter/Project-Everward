#pragma once

#include "everward/simulation/compound_contact.hpp"
#include "everward/simulation/types.hpp"

#include <algorithm>
#include <cmath>

namespace everward::simulation {

// Solar generation for a surface-bound or coasting probe (#378, owner direction 2026-11-28: "Big solar panels to
// catch the energy to live").
//
// The probe's energy balance already exists: integrate_energy_balance() applies energy_generation_w against the
// allocated draw every fixed step, and a depleted reserve locks the probe out. What did not exist was any reason
// for that generation figure to change. The comment on energy_generation_w says as much -- it models a constant
// RTG supply because "solar generation would need the star-distance/irradiance model that does not exist yet".
//
// A probe standing on a surface needs far less than an orbital model to make the panels matter: an irradiance
// figure for the body and a direction to the star. That is enough for orientation, damage, shadow and panel area
// to each change the number, which is what turns power into the clock the opening runs on.
//
// This header is deliberately pure: it reads inputs and returns watts. It never moves the probe, never writes
// state, and has no opinion about what spends the energy.

// Defaults are INERT on purpose: no panels, no star. energy_generation_w defaults to 0.0 for exactly this reason
// -- a bare, engine-neutral core has to keep an exact generation-free energy baseline, and every existing timing
// test depends on it. Fitting a probe with panels is something a scenario does deliberately, with the Mark 1
// figures below, not something that happens because a struct was default-constructed.
struct SolarArray {
    double area_m2{0.0};        // total illuminated area with the panels deployed
    double efficiency{0.0};     // 0..1 fraction of incident power converted
    double integrity{1.0};      // 0..1 damage state, repaired like any other component
    Vector3d local_normal{0.0, 0.0, 1.0};   // the face the panels present, in the probe's own frame (+z is up)
    bool deployed{false};       // stowed panels catch nothing
};

struct SolarEnvironment {
    double irradiance_w_per_m2{0.0};        // no star in reach until a scenario places one
    Vector3d sun_direction{0.0, 0.0, 1.0};  // unit vector FROM the probe TOWARD the star
    double occlusion{0.0};                  // 0 clear .. 1 fully shadowed (night, terrain shadow, dust)
};

// The Generation-1 probe as drawn on the canon sheets: two large rigid panels. These are the figures a scenario
// fits to the probe, and the ones an upgrade raises. Area has no ceiling; efficiency approaches one and stops,
// which is why more power means bigger panels rather than better ones.
inline constexpr double kMark1SolarAreaM2 = 6.0;
inline constexpr double kMark1SolarEfficiency = 0.22;
inline constexpr double kSolarConstantWPerM2 = 1361.0;   // at 1 AU

[[nodiscard]] inline SolarArray mark1_solar_array() noexcept {
    SolarArray array{};
    array.area_m2 = kMark1SolarAreaM2;
    array.efficiency = kMark1SolarEfficiency;
    array.deployed = true;
    return array;
}

// Cosine of the angle between the panel face and the star, in world space; 0 when the panels face away.
// Returns 0 rather than a negative number: a panel presented edge-on or backwards generates nothing, it does not
// consume. Non-finite inputs also give 0, because inventing power from bad data is the one unsafe failure here.
[[nodiscard]] inline double solar_incidence_cosine(
    const SolarArray& array,
    const SolarEnvironment& environment,
    EulerAttitudeDegrees attitude) noexcept {
    const Vector3d n = array.local_normal;
    const Vector3d s = environment.sun_direction;
    if (!std::isfinite(n.x) || !std::isfinite(n.y) || !std::isfinite(n.z) ||
        !std::isfinite(s.x) || !std::isfinite(s.y) || !std::isfinite(s.z) ||
        !std::isfinite(attitude.yaw) || !std::isfinite(attitude.pitch) || !std::isfinite(attitude.roll)) {
        return 0.0;
    }
    const double normal_length = std::sqrt(contact_dot(n, n));
    const double sun_length = std::sqrt(contact_dot(s, s));
    if (normal_length <= 1e-12 || sun_length <= 1e-12) {
        return 0.0;
    }
    // The panel face turns with the probe, using the same roll(X)/pitch(Y)/yaw(Z) convention as the hull samples,
    // so a probe lying on its back in shadow is not quietly treated as facing the star.
    const Vector3d world_normal = rotate_local_contact_offset(
        contact_scale(n, 1.0 / normal_length), attitude);
    const double cosine = contact_dot(world_normal, contact_scale(s, 1.0 / sun_length));
    if (!std::isfinite(cosine)) {
        return 0.0;
    }
    return std::max(0.0, std::min(1.0, cosine));
}

// Watts the array delivers right now. Every factor can independently take it to zero, which is the point: a
// stowed, destroyed, shadowed, edge-on or starless array is worth nothing, and the player can tell which.
[[nodiscard]] inline double solar_generation_w(
    const SolarArray& array,
    const SolarEnvironment& environment,
    EulerAttitudeDegrees attitude) noexcept {
    if (!array.deployed) {
        return 0.0;
    }
    if (!std::isfinite(array.area_m2) || !std::isfinite(array.efficiency) || !std::isfinite(array.integrity) ||
        !std::isfinite(environment.irradiance_w_per_m2) || !std::isfinite(environment.occlusion)) {
        return 0.0;
    }
    const double area = std::max(0.0, array.area_m2);
    // Efficiency is the one trait with a real ceiling; area, capacity and count are the ones that scale without
    // limit, which is exactly why "bigger panels" is the answer to needing more power.
    const double efficiency = std::clamp(array.efficiency, 0.0, 1.0);
    const double integrity = std::clamp(array.integrity, 0.0, 1.0);
    const double irradiance = std::max(0.0, environment.irradiance_w_per_m2);
    const double clear = 1.0 - std::clamp(environment.occlusion, 0.0, 1.0);
    const double cosine = solar_incidence_cosine(array, environment, attitude);
    const double watts = irradiance * area * efficiency * integrity * cosine * clear;
    return std::isfinite(watts) ? std::max(0.0, watts) : 0.0;
}

// The array's best possible output as it currently stands: undamaged, square to the star, nothing in the way.
// This is what the interface compares the live figure against, so a player can see the difference between a panel
// that is broken and one that is merely pointed the wrong way.
[[nodiscard]] inline double solar_capacity_w(
    const SolarArray& array,
    const SolarEnvironment& environment) noexcept {
    if (!std::isfinite(array.area_m2) || !std::isfinite(array.efficiency) ||
        !std::isfinite(environment.irradiance_w_per_m2)) {
        return 0.0;
    }
    const double watts = std::max(0.0, environment.irradiance_w_per_m2) * std::max(0.0, array.area_m2)
        * std::clamp(array.efficiency, 0.0, 1.0);
    return std::isfinite(watts) ? std::max(0.0, watts) : 0.0;
}

}  // namespace everward::simulation
