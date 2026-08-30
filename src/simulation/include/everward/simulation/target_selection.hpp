#pragma once

#include "everward/simulation/compound_contact.hpp"
#include "everward/simulation/types.hpp"

#include <algorithm>
#include <cmath>
#include <optional>
#include <string>
#include <vector>

namespace everward::simulation {

// Slice 7 (PHASE2_VERTICAL_SLICE_PLAN.md) foundation. The
// detect -> select -> approach -> scan -> reach -> grasp -> move -> release
// loop's first two minimum interactions are "select a nearby physical
// target" and "display range/relative motion". This module implements both
// as pure, engine-independent read-side math over the same registered
// StaticSphereBody list and probe pose software_policy.hpp's swept contact
// solver already consumes -- no authoritative state is mutated here, and no
// behavior currently awaiting Product Reality (contact resolution,
// manipulator/hull collision) is assumed correct or depended upon, so this
// qualifies for the parallel-safe lane. Wiring this into ProbeRuntime's
// tick/telemetry and an Unreal HUD/selection input is deliberately left to
// a follow-up slice, mirroring how compound_contact.hpp's geometry landed
// before software_policy.hpp's solver wiring. Vector math is intentionally
// not re-derived here: contact_subtract/contact_scale/contact_dot already
// exist in compound_contact.hpp.

struct TargetRangeTelemetry {
    std::string body_id;
    double surface_range_m{0.0};
    double closing_speed_mps{0.0};
};

// Surface-to-surface distance from a point to a sphere, clamped to zero
// once the point is at or inside the surface rather than reporting a
// negative range.
[[nodiscard]] inline double surface_range_to_body(
    Vector3d point_m,
    const StaticSphereBody& body) noexcept {
    const Vector3d offset = contact_subtract(point_m, body.center_m);
    const double center_distance = std::sqrt(contact_dot(offset, offset));
    return std::max(0.0, center_distance - body.radius_m);
}

// Positive when `point_m`, moving with `point_velocity_mps`, is closing on
// a stationary body's center; negative when opening. A point sitting
// exactly on the body's center has no defined direction of approach and
// reports zero rather than fabricating one.
[[nodiscard]] inline double closing_speed_to_body(
    Vector3d point_m,
    Vector3d point_velocity_mps,
    const StaticSphereBody& body) noexcept {
    const Vector3d to_body = contact_subtract(body.center_m, point_m);
    const double distance = std::sqrt(contact_dot(to_body, to_body));
    if (distance <= 1e-9) {
        return 0.0;
    }
    const Vector3d unit_to_body = contact_scale(to_body, 1.0 / distance);
    return contact_dot(point_velocity_mps, unit_to_body);
}

// Finds the nearest registered body within `max_selection_range_m` of
// `probe_position_m`. Ties break toward whichever body appears earliest in
// `bodies` (stable, deterministic). Returns std::nullopt when no body is
// registered or none is within range rather than guessing a selection.
// The returned telemetry's closing_speed_mps is always 0.0 -- callers that
// need closing speed for a specific selection should use
// select_target_telemetry, which has a probe velocity to compute it from.
[[nodiscard]] inline std::optional<TargetRangeTelemetry> find_nearest_selectable_target(
    Vector3d probe_position_m,
    const std::vector<StaticSphereBody>& bodies,
    double max_selection_range_m) noexcept {
    std::optional<TargetRangeTelemetry> nearest;
    for (const StaticSphereBody& body : bodies) {
        const double range = surface_range_to_body(probe_position_m, body);
        if (range > max_selection_range_m) {
            continue;
        }
        if (!nearest.has_value() || range < nearest->surface_range_m) {
            nearest = TargetRangeTelemetry{body.body_id, range, 0.0};
        }
    }
    return nearest;
}

// Validates an explicitly requested selection against the live registry and
// reports its current range/closing-speed telemetry. Returns std::nullopt
// when `requested_body_id` is empty or does not match any currently
// registered body -- a destroyed or never-registered target is rejected
// rather than silently treated as still selected, matching this codebase's
// fail-closed handling of registered-body lookups elsewhere (see
// manipulator_hull_contact.hpp).
[[nodiscard]] inline std::optional<TargetRangeTelemetry> select_target_telemetry(
    const std::string& requested_body_id,
    const std::vector<StaticSphereBody>& bodies,
    Vector3d probe_position_m,
    Vector3d probe_velocity_mps) noexcept {
    if (requested_body_id.empty()) {
        return std::nullopt;
    }
    const auto found = std::find_if(
        bodies.begin(),
        bodies.end(),
        [&requested_body_id](const StaticSphereBody& body) {
            return body.body_id == requested_body_id;
        });
    if (found == bodies.end()) {
        return std::nullopt;
    }

    TargetRangeTelemetry telemetry;
    telemetry.body_id = found->body_id;
    telemetry.surface_range_m = surface_range_to_body(probe_position_m, *found);
    telemetry.closing_speed_mps =
        closing_speed_to_body(probe_position_m, probe_velocity_mps, *found);
    return telemetry;
}

} // namespace everward::simulation
