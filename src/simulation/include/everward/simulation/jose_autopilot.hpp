#pragma once

#include "everward/simulation/compound_contact.hpp"
#include "everward/simulation/target_selection.hpp"
#include "everward/simulation/types.hpp"

#include <algorithm>
#include <cmath>
#include <optional>
#include <string>
#include <vector>

namespace everward::simulation {

// docs/JOSE_TAKE_THE_WHEEL.md "Phase-2 first playable implementation". The
// guidance law itself (approach speed shaping and arrival detection) was
// originally written directly in Unreal's EverwardPlayerControllerAutopilot.cpp,
// making the actual navigation decision untestable without launching Unreal
// and inconsistent with every other Slice 7 sub-slice (target_selection.hpp,
// manipulator_reach.hpp, manipulator_move.hpp, manipulator_release.hpp), each
// of which keeps its decision math pure and engine-independent behind the
// adapter boundary. This module extracts that same math with no intended
// behavior change; Unreal keeps only the engage/cancel session state machine
// and on-screen messaging. Reuses target_selection.hpp's surface_range_to_body
// for the arrival metric -- no second range formula is invented.
struct JoseAutopilotConfig {
    double cruise_speed_mps{6.0};
    double arrival_surface_standoff_m{20.0};
    double arrival_tolerance_m{0.5};
    double approach_gain_per_second{0.35};
    double minimum_approach_speed_mps{0.25};
};

enum class JoseGuidanceOutcome { Continue, Arrived, DestinationUnresolved };

struct JoseGuidanceCommand {
    JoseGuidanceOutcome outcome{JoseGuidanceOutcome::DestinationUnresolved};
    Vector3d command_velocity_mps{};
    // Surface range to the destination minus the arrival standoff. Meaningful
    // (and used for the arrival test) on every outcome except
    // DestinationUnresolved, where the probe/destination are coincident.
    double remaining_surface_range_m{0.0};
};

// Fails to DestinationUnresolved (never fabricates a heading) when the
// destination is coincident with the probe position, matching the original
// Unreal FVector::IsNearlyZero() guard on the delta vector.
[[nodiscard]] inline JoseGuidanceCommand jose_guidance_command(
    Vector3d probe_position_m,
    Vector3d destination_position_m,
    double destination_surface_range_m,
    const JoseAutopilotConfig& config = {}) noexcept {
    JoseGuidanceCommand result;
    result.remaining_surface_range_m = destination_surface_range_m - config.arrival_surface_standoff_m;
    if (result.remaining_surface_range_m <= config.arrival_tolerance_m) {
        result.outcome = JoseGuidanceOutcome::Arrived;
        return result;
    }

    const Vector3d delta = contact_subtract(destination_position_m, probe_position_m);
    const double distance_m = std::sqrt(contact_dot(delta, delta));
    if (distance_m <= 1e-4) {
        result.outcome = JoseGuidanceOutcome::DestinationUnresolved;
        return result;
    }

    // JoseCruiseSpeedMetersPerSecond is an independently EditAnywhere-tunable
    // property (ClampMin 0.1) and can therefore be set below
    // minimum_approach_speed_mps (0.25); std::clamp's [lo, hi] bounds are
    // undefined behavior if lo > hi, so the effective upper bound is raised
    // to never fall below the floor rather than trusting the two values to
    // already be ordered. A cruise speed configured below the floor simply
    // pins José to the floor speed instead of invoking undefined behavior.
    const double effective_cruise_speed_mps = std::max(config.cruise_speed_mps, config.minimum_approach_speed_mps);
    const double approach_speed_mps = std::clamp(
        result.remaining_surface_range_m * config.approach_gain_per_second,
        config.minimum_approach_speed_mps,
        effective_cruise_speed_mps);
    result.outcome = JoseGuidanceOutcome::Continue;
    result.command_velocity_mps = contact_scale(delta, approach_speed_mps / distance_m);
    return result;
}

// Convenience overload matching manipulator_reach_status()'s StaticSphereBody
// lookup pattern: fails to nullopt when the destination body is not (or no
// longer) registered, the same "destination is no longer available" case
// the adapter/Unreal side must otherwise handle, but keeping the fail-closed
// contract in the engine-independent layer rather than only at the boundary.
[[nodiscard]] inline std::optional<JoseGuidanceCommand> jose_guidance_command_for_body(
    Vector3d probe_position_m,
    const std::vector<StaticSphereBody>& bodies,
    const std::string& destination_body_id,
    const JoseAutopilotConfig& config = {}) noexcept {
    const auto destination = std::find_if(bodies.begin(), bodies.end(), [&destination_body_id](const StaticSphereBody& body) {
        return body.body_id == destination_body_id;
    });
    if (destination == bodies.end()) return std::nullopt;
    const double surface_range_m = surface_range_to_body(probe_position_m, *destination);
    return jose_guidance_command(probe_position_m, destination->center_m, surface_range_m, config);
}

} // namespace everward::simulation
