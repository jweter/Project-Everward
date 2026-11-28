#pragma once

// surface_descent_guidance.hpp owns AltitudeAwareDescentProfile and the sphere-based taper this one must agree
// with, so it is reused rather than a second profile type being invented alongside it.
#include "everward/simulation/planetary_body.hpp"
#include "everward/simulation/surface_descent_guidance.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>

namespace everward::simulation {

// Landing, resting and taking off again (#378; owner direction 2026-11-28, docs/LANDING_AND_STRANDED_START.md:
// "we need to be able to lan[d] the probe on the ground and take off again. Initial moment of awaking you are
// stranded, low power ... on top of a minable resource").
//
// Two things were missing rather than three. surface_descent_guidance.hpp already shapes a descent, tapering
// speed as clearance closes and refusing commanded inward velocity at the floor -- tested C++ that no engine has
// ever called. But it reaches altitude through a SphericalPlanetaryBody, and the Godot path flies over voxel
// terrain measured by a downward ray, with no sphere anywhere. So the taper could not be used as written.
//
// This header supplies the two pieces that were actually absent:
//   * the same descent taper expressed against a MEASURED altitude, so an engine with a ground ray can use it;
//   * "landed" as a decided state rather than an absence of motion, and the take-off rule that reads from it.
//
// Pure, like solar_power.hpp and angular_flight.hpp: it decides and returns, holds no state, and moves nothing.

struct LandedThresholds {
    double speed_mps{0.15};          // slow enough to exclude a slide or a bounce
    double settle_s{0.5};            // one touch is not a landing
    double takeoff_clearance_m{1.0}; // how far off the surface counts as flying again
};

struct LandedState {
    bool landed{false};
    double settled_s{0.0};   // how long the probe has been in contact and slow
};

// The take-off floor: below this propulsion integrity the drive cannot lift the hull at all, and the answer is
// Fix_It rather than the autopilot. Above it, climb authority scales with integrity like every other capability.
inline constexpr double kDefaultTakeoffMinimumIntegrity = 0.15;

// How fast a descent may be made at a measured altitude above the surface. Mirrors
// altitude_limited_descent_speed_mps() exactly, but takes the altitude an engine measured rather than deriving it
// from a sphere the Godot path does not have. At the floor the answer is zero: there is nowhere left to descend
// to, and contact resolution owns anything past that.
[[nodiscard]] inline double descent_speed_limit_mps(
    double altitude_above_surface_m,
    const ControlledDescentEnvelope& envelope = {},
    const AltitudeAwareDescentProfile& profile = {}) noexcept {
    if (!std::isfinite(altitude_above_surface_m) || !std::isfinite(envelope.max_descent_speed_mps)
        || !std::isfinite(envelope.minimum_clearance_m) || !std::isfinite(profile.full_speed_altitude_m)
        || !std::isfinite(profile.touchdown_descent_speed_mps)) {
        return 0.0;   // an unknown altitude is not permission to fall
    }
    const double max_speed = std::fabs(envelope.max_descent_speed_mps);
    const double touchdown = std::clamp(std::fabs(profile.touchdown_descent_speed_mps), 0.0, max_speed);
    const double clearance = std::max(0.0, envelope.minimum_clearance_m);
    const double altitude = altitude_above_surface_m - clearance;
    if (altitude <= 8.0 * std::numeric_limits<double>::epsilon() * std::max(1.0, clearance)) {
        return 0.0;
    }
    const double full_speed_altitude = std::max(0.0, profile.full_speed_altitude_m);
    if (full_speed_altitude <= 1e-12 || altitude >= full_speed_altitude) {
        return max_speed;
    }
    return touchdown + (max_speed - touchdown) * (altitude / full_speed_altitude);
}

// Landed is a decision held over time, not a single frame's stillness: a probe that clips the ground at speed and
// bounces was never landed, and one resting on a slope is. Contact plus slow plus held for settle_s.
[[nodiscard]] inline LandedState update_landed_state(
    LandedState previous,
    bool in_surface_contact,
    double body_relative_speed_mps,
    double altitude_above_surface_m,
    double seconds,
    const LandedThresholds& thresholds = {}) noexcept {
    if (!std::isfinite(body_relative_speed_mps) || !std::isfinite(seconds) || seconds < 0.0
        || !std::isfinite(thresholds.speed_mps) || !std::isfinite(thresholds.settle_s)) {
        return previous;   // never invent or revoke a landing from bad data
    }
    LandedState out = previous;
    // Clear air ends a landing outright, however slowly it was left: this is the one unambiguous signal, and it
    // is what makes take-off observable rather than inferred.
    if (std::isfinite(altitude_above_surface_m)
        && altitude_above_surface_m > std::max(0.0, thresholds.takeoff_clearance_m)) {
        return LandedState{false, 0.0};
    }
    if (!in_surface_contact) {
        // Off the ground but still within the clearance band: not flying yet, not landed either. The settle timer
        // restarts so a series of brief touches cannot accumulate into a landing.
        out.settled_s = 0.0;
        out.landed = false;
        return out;
    }
    if (std::fabs(body_relative_speed_mps) > std::max(0.0, thresholds.speed_mps)) {
        out.settled_s = 0.0;   // sliding or skidding along the surface is not resting on it
        out.landed = false;
        return out;
    }
    out.settled_s = previous.settled_s + seconds;
    out.landed = out.settled_s >= std::max(0.0, thresholds.settle_s);
    return out;
}

// Why take-off is refused, or "" when it is not. The string is the player-facing explanation: a probe that cannot
// fly must say so and say what to do, rather than ignoring the key (docs/CONTEXTUAL_GUIDANCE... "Explain blockers").
[[nodiscard]] inline std::string takeoff_refusal(
    double propulsion_integrity,
    double minimum_integrity = kDefaultTakeoffMinimumIntegrity) noexcept {
    if (!std::isfinite(propulsion_integrity)) {
        return "propulsion state unknown";
    }
    const double floor_value = std::isfinite(minimum_integrity) ? std::max(0.0, minimum_integrity) : 0.0;
    if (propulsion_integrity <= 0.0) {
        return "propulsion offline: repair it with Fix_It before flying";
    }
    if (propulsion_integrity < floor_value) {
        return "propulsion too damaged to lift off: repair it with Fix_It before flying";
    }
    return "";
}

// Climb authority, scaled by the propulsion the probe has left -- the same rule as thrust, attitude trim, the scan
// and the autopilot. Zero below the floor, so the refusal above and this figure can never disagree.
[[nodiscard]] inline double takeoff_climb_rate_mps(
    double nominal_climb_rate_mps,
    double propulsion_integrity,
    double minimum_integrity = kDefaultTakeoffMinimumIntegrity) noexcept {
    if (!std::isfinite(nominal_climb_rate_mps) || !std::isfinite(propulsion_integrity)) {
        return 0.0;
    }
    if (!takeoff_refusal(propulsion_integrity, minimum_integrity).empty()) {
        return 0.0;
    }
    const double rate = std::max(0.0, nominal_climb_rate_mps) * std::clamp(propulsion_integrity, 0.0, 1.0);
    return std::isfinite(rate) ? rate : 0.0;
}

}  // namespace everward::simulation
