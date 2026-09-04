#pragma once

#include "everward/simulation/compound_contact.hpp"
#include "everward/simulation/types.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>
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
// solver already consumes -- no authoritative state is mutated here.

struct TargetRangeTelemetry {
    std::string body_id;
    double surface_range_m{0.0};
    double closing_speed_mps{0.0};
};

[[nodiscard]] inline double surface_range_to_body(Vector3d point_m, const StaticSphereBody& body) noexcept {
    const Vector3d offset = contact_subtract(point_m, body.center_m);
    const double center_distance = std::sqrt(contact_dot(offset, offset));
    return std::max(0.0, center_distance - body.radius_m);
}

[[nodiscard]] inline double closing_speed_to_body(Vector3d point_m, Vector3d point_velocity_mps, const StaticSphereBody& body) noexcept {
    const Vector3d to_body = contact_subtract(body.center_m, point_m);
    const double distance = std::sqrt(contact_dot(to_body, to_body));
    if (distance <= 1e-9) return 0.0;
    const Vector3d unit_to_body = contact_scale(to_body, 1.0 / distance);
    return contact_dot(point_velocity_mps, unit_to_body);
}

[[nodiscard]] inline std::optional<TargetRangeTelemetry> find_nearest_selectable_target(Vector3d probe_position_m, const std::vector<StaticSphereBody>& bodies, double max_selection_range_m) noexcept {
    std::optional<TargetRangeTelemetry> nearest;
    for (const StaticSphereBody& body : bodies) {
        const double range = surface_range_to_body(probe_position_m, body);
        if (range > max_selection_range_m) continue;
        if (!nearest.has_value() || range < nearest->surface_range_m) nearest = TargetRangeTelemetry{body.body_id, range, 0.0};
    }
    return nearest;
}

// Deterministic target cycling for Slice 7. Eligible bodies are ordered nearest
// to farthest; equal-range ties retain registration order. Unknown/stale current
// selections restart at the nearest eligible target, and the farthest wraps to
// the nearest. No eligible target returns nullopt instead of preserving stale state.
[[nodiscard]] inline std::optional<TargetRangeTelemetry> find_next_selectable_target(
    Vector3d probe_position_m,
    Vector3d probe_velocity_mps,
    const std::vector<StaticSphereBody>& bodies,
    double max_selection_range_m,
    const std::string& current_body_id) noexcept {
    struct RankedTarget {
        std::size_t registration_index{0};
        TargetRangeTelemetry telemetry;
    };
    std::vector<RankedTarget> ranked;
    ranked.reserve(bodies.size());
    for (std::size_t index = 0; index < bodies.size(); ++index) {
        const StaticSphereBody& body = bodies[index];
        const double range = surface_range_to_body(probe_position_m, body);
        if (range > max_selection_range_m) continue;
        ranked.push_back({index, {body.body_id, range, closing_speed_to_body(probe_position_m, probe_velocity_mps, body)}});
    }
    if (ranked.empty()) return std::nullopt;
    std::stable_sort(ranked.begin(), ranked.end(), [](const RankedTarget& left, const RankedTarget& right) {
        if (left.telemetry.surface_range_m == right.telemetry.surface_range_m) return left.registration_index < right.registration_index;
        return left.telemetry.surface_range_m < right.telemetry.surface_range_m;
    });
    const auto current = std::find_if(ranked.begin(), ranked.end(), [&current_body_id](const RankedTarget& target) {
        return target.telemetry.body_id == current_body_id;
    });
    if (current == ranked.end()) return ranked.front().telemetry;
    const auto next = std::next(current);
    if (next == ranked.end()) return ranked.front().telemetry;
    return next->telemetry;
}

// Slice 7 "approach" step: the loop's minimum interactions already surface
// range and closing speed as raw numbers (see TargetRangeTelemetry /
// TargetSelectionStatus below), but nothing previously classified that
// number into the qualitative closing/holding/opening motion state a player
// actually reads at a glance. This is presentation-classification only --
// it introduces no new physics, authoritative state, or player input; the
// probe still approaches purely through existing manual translation. A
// small deadband absorbs sensor/float noise near a stable hold so the label
// does not flicker between Closing and Opening at effectively zero closing
// speed.
enum class ApproachMotionState { Closing, HoldingRange, Opening };

inline constexpr double kApproachMotionDeadbandMps = 0.05;

[[nodiscard]] inline ApproachMotionState classify_approach_motion(
    double closing_speed_mps, double deadband_mps = kApproachMotionDeadbandMps) noexcept {
    if (closing_speed_mps > deadband_mps) return ApproachMotionState::Closing;
    if (closing_speed_mps < -deadband_mps) return ApproachMotionState::Opening;
    return ApproachMotionState::HoldingRange;
}

[[nodiscard]] inline std::optional<TargetRangeTelemetry> select_target_telemetry(
    const std::string& requested_body_id,
    const std::vector<StaticSphereBody>& bodies,
    Vector3d probe_position_m,
    Vector3d probe_velocity_mps) noexcept {
    if (requested_body_id.empty()) return std::nullopt;
    const auto found = std::find_if(bodies.begin(), bodies.end(), [&requested_body_id](const StaticSphereBody& body) { return body.body_id == requested_body_id; });
    if (found == bodies.end()) return std::nullopt;
    TargetRangeTelemetry telemetry;
    telemetry.body_id = found->body_id;
    telemetry.surface_range_m = surface_range_to_body(probe_position_m, *found);
    telemetry.closing_speed_mps = closing_speed_to_body(probe_position_m, probe_velocity_mps, *found);
    return telemetry;
}

} // namespace everward::simulation
