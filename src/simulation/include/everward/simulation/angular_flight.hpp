#pragma once

#include "everward/simulation/types.hpp"

#include <algorithm>
#include <cmath>

namespace everward::simulation {

// Rotation with rate, so the probe can feel like a flown object instead of a number being nudged
// (#378, owner direction 2026-11-28: "That had 3d space flight and was very good with it's controls setup").
//
// Today rotation is an instantaneous pose delta: input is multiplied by a constant and written straight into
// attitude_degrees. There is no angular velocity anywhere in the simulation, so the probe stops rotating the
// instant input stops and carries no spin. That is the actual reason the controls feel wrong, and no amount of
// rebinding fixes it.
//
// This module is the decision math, kept pure in the same style as solar_power.hpp and jose_autopilot.hpp: it
// takes the current angular rate, a command, and a time step, and returns the new rate and the attitude delta it
// produces. It holds no state, writes nothing, and moves nothing. The core integrates it; both engines inherit it,
// which is the mistake not to repeat after the autopilot (#461) ended up with two divergent implementations.

// Degrees per second about each body axis, in the same roll(X)/pitch(Y)/yaw(Z) convention the hull samples and
// attitude read model already use.
struct AngularRateDegPerS {
    // Declared yaw-first to match EulerAttitudeDegrees. An earlier draft put roll first, and a brace initialiser
    // then silently swapped roll and yaw: it compiled, ran, and mis-mapped two axes, which is precisely how
    // controls come to feel inexplicably wrong. Same order everywhere, designated initialisers below.
    double yaw{0.0};
    double pitch{0.0};
    double roll{0.0};
};

// What the player is asking for, each in -1..1. The scheme decides what the magnitude means.
struct RotationCommand {
    double yaw{0.0};
    double pitch{0.0};
    double roll{0.0};
};

enum class RotationModel {
    // Input is a pose delta: authority is degrees per unit of input, applied immediately and forgotten. This is
    // what the probe does today, kept expressible so a scheme can still choose it.
    Direct,
    // Input is an angular acceleration: authority is degrees per second squared, the rate persists, and damping
    // bleeds it off. Zero damping holds a spin forever, which is what a vacuum actually does.
    Rate
};

struct RotationAuthority {
    RotationModel model{RotationModel::Rate};
    double authority{90.0};        // deg/s^2 in Rate, deg per input unit in Direct
    double rate_max_deg_per_s{0.0};  // 0 means uncapped
    double damping_per_s{0.0};     // fraction of rate bled per second; 0 holds spin, high is snappy
    bool roll_enabled{true};
    // Every scheme is scaled by the propulsion the probe has left, so a damaged machine is sluggish in all of
    // them. No scheme may bypass the damage model: that is the hole the autopilot had.
    double propulsion_effectiveness{1.0};
};

struct RotationStep {
    AngularRateDegPerS rate{};            // the rate after this step
    EulerAttitudeDegrees attitude_delta{};  // how far the probe actually turned during it
    bool rate_limited{false};
    bool rejected{false};                 // non-finite input: nothing applied
};

namespace detail {

[[nodiscard]] inline bool finite_command(const RotationCommand& c) noexcept {
    return std::isfinite(c.roll) && std::isfinite(c.pitch) && std::isfinite(c.yaw);
}

[[nodiscard]] inline bool finite_rate(const AngularRateDegPerS& r) noexcept {
    return std::isfinite(r.roll) && std::isfinite(r.pitch) && std::isfinite(r.yaw);
}

[[nodiscard]] inline bool finite_authority(const RotationAuthority& a) noexcept {
    return std::isfinite(a.authority) && std::isfinite(a.rate_max_deg_per_s)
        && std::isfinite(a.damping_per_s) && std::isfinite(a.propulsion_effectiveness);
}

[[nodiscard]] inline double clamp_unit(double v) noexcept {
    return std::clamp(v, -1.0, 1.0);
}

// One axis of Rate integration: accelerate, damp, clamp. Damping is applied to the rate that already existed
// rather than to the new command, so holding an input still wins against damping instead of fighting itself.
[[nodiscard]] inline double step_rate_axis(
    double rate, double command, double accel_per_s2, double damping_per_s, double rate_max, double seconds,
    bool& limited) noexcept {
    const double damped = rate * std::max(0.0, 1.0 - std::max(0.0, damping_per_s) * seconds);
    double updated = damped + command * accel_per_s2 * seconds;
    if (rate_max > 0.0 && std::fabs(updated) > rate_max) {
        updated = updated > 0.0 ? rate_max : -rate_max;
        limited = true;
    }
    return updated;
}

}  // namespace detail

// Advance rotation by one step. Returns the new rate and the attitude change it produced.
[[nodiscard]] inline RotationStep advance_rotation(
    AngularRateDegPerS rate,
    RotationCommand command,
    const RotationAuthority& authority,
    double seconds) noexcept {
    RotationStep out{};
    if (!detail::finite_rate(rate) || !detail::finite_command(command)
        || !detail::finite_authority(authority) || !std::isfinite(seconds) || seconds < 0.0) {
        out.rejected = true;
        out.rate = detail::finite_rate(rate) ? rate : AngularRateDegPerS{};
        return out;
    }
    // Nothing happens in no time, and that has to be decided BEFORE anything is modified. An earlier version
    // checked it only inside the Rate branch, so a zero-length step in Direct mode still applied a full
    // commanded turn -- a 90-degree yaw for no elapsed time -- and a roll-disabled scheme still cleared an
    // inherited roll rate. A fixed-step engine passes 0 at the edges of a frame, so that was a real path.
    if (seconds == 0.0) {
        out.rate = rate;
        return out;
    }
    if (!authority.roll_enabled) {
        command.roll = 0.0;
        rate.roll = 0.0;   // a scheme without roll must not leave an old spin running
    }
    const double effectiveness = std::clamp(authority.propulsion_effectiveness, 0.0, 1.0);
    const double strength = std::max(0.0, authority.authority) * effectiveness;
    const double roll_cmd = detail::clamp_unit(command.roll);
    const double pitch_cmd = detail::clamp_unit(command.pitch);
    const double yaw_cmd = detail::clamp_unit(command.yaw);

    if (authority.model == RotationModel::Direct) {
        // The behaviour the probe has today: a pose delta, no rate kept. Still scaled by propulsion, so Direct is
        // not a way around the damage model either.
        out.rate = AngularRateDegPerS{};
        out.attitude_delta = EulerAttitudeDegrees{
            .yaw = yaw_cmd * strength, .pitch = pitch_cmd * strength, .roll = roll_cmd * strength};
        return out;
    }

    const double cap = std::max(0.0, authority.rate_max_deg_per_s) * effectiveness;
    bool limited = false;
    const double roll_rate = detail::step_rate_axis(
        rate.roll, roll_cmd, strength, authority.damping_per_s, cap, seconds, limited);
    const double pitch_rate = detail::step_rate_axis(
        rate.pitch, pitch_cmd, strength, authority.damping_per_s, cap, seconds, limited);
    const double yaw_rate = detail::step_rate_axis(
        rate.yaw, yaw_cmd, strength, authority.damping_per_s, cap, seconds, limited);

    out.rate = AngularRateDegPerS{.yaw = yaw_rate, .pitch = pitch_rate, .roll = roll_rate};
    out.rate_limited = limited;
    // The turn taken over the step uses the average of the old and new rate, so a step is not biased by where in
    // the acceleration it happened to land. At a fixed 60 Hz step this matters less than it will at 240.
    out.attitude_delta = EulerAttitudeDegrees{
        .yaw = 0.5 * (rate.yaw + yaw_rate) * seconds,
        .pitch = 0.5 * (rate.pitch + pitch_rate) * seconds,
        .roll = 0.5 * (rate.roll + roll_rate) * seconds};
    if (!std::isfinite(out.attitude_delta.roll) || !std::isfinite(out.attitude_delta.pitch)
        || !std::isfinite(out.attitude_delta.yaw)) {
        return RotationStep{AngularRateDegPerS{}, EulerAttitudeDegrees{}, false, true};
    }
    return out;
}

// Bring a spin to rest deliberately: what a stability assist does when the player lets go, and what a scheme with
// no roll needs for an inherited spin. Not the same as damping, which is continuous and partial.
[[nodiscard]] inline AngularRateDegPerS arrest_rotation(
    AngularRateDegPerS rate, double max_deceleration_deg_per_s2, double seconds) noexcept {
    if (!detail::finite_rate(rate) || !std::isfinite(max_deceleration_deg_per_s2)
        || !std::isfinite(seconds) || seconds <= 0.0 || max_deceleration_deg_per_s2 <= 0.0) {
        return detail::finite_rate(rate) ? rate : AngularRateDegPerS{};
    }
    const double step = max_deceleration_deg_per_s2 * seconds;
    const auto bleed = [step](double v) noexcept {
        if (std::fabs(v) <= step) {
            return 0.0;
        }
        return v > 0.0 ? v - step : v + step;
    };
    return AngularRateDegPerS{.yaw = bleed(rate.yaw), .pitch = bleed(rate.pitch), .roll = bleed(rate.roll)};
}

[[nodiscard]] inline double angular_speed_deg_per_s(const AngularRateDegPerS& rate) noexcept {
    if (!detail::finite_rate(rate)) {
        return 0.0;
    }
    return std::sqrt(rate.roll * rate.roll + rate.pitch * rate.pitch + rate.yaw * rate.yaw);
}

}  // namespace everward::simulation
