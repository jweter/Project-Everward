#pragma once

#include "everward/simulation/compound_contact.hpp"
#include "everward/simulation/impact_damage.hpp"
#include "everward/simulation/manipulator.hpp"
#include "everward/simulation/manipulator_hull_contact.hpp"
#include "everward/simulation/target_selection.hpp"
#include "everward/simulation/types.hpp"

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

namespace everward::simulation {

// Slice 7 (PHASE2_VERTICAL_SLICE_PLAN.md) minimum interaction: "align a
// manipulator". This is read-only telemetry only -- it reports whether a
// deployed arm's wrist is currently within a fixed reach envelope of the
// selected physical target's surface, and how far away it still is. It
// authors no grasp/attach/dock state; that remains a later, not-yet-attempted
// Slice 7 sub-slice.
//
// Reuses manipulator_hull_contact.hpp's manipulator_arm_contact_samples()
// for the wrist's probe-local position and rotate_local_contact_offset (via
// compound_contact.hpp, which manipulator_hull_contact.hpp already pulls in)
// for the same local-to-world placement convention software_policy.hpp's
// probe-vs-body contact and manipulator_hull_contact.hpp's own
// environment-collision guard already use -- no second local-to-world
// convention is invented here. Range-to-surface reuses
// target_selection.hpp's surface_range_to_body against the same registered
// StaticSphereBody list the target-selection telemetry already reads.

struct ManipulatorReachStatus {
    bool in_reach{false};
    double wrist_range_to_surface_m{0.0};
    // 0 once in_reach is true; otherwise how much closer the wrist still
    // needs to get.
    double remaining_distance_m{0.0};
};

// Fixed Generation-1 reach envelope. The wrist-to-tool-tip extension is
// 0.79 m (manipulator_tool_contact.hpp's kTipOffsetFromWristM) and the total
// shoulder-to-wrist arm length is 3.30 m
// (ManipulatorArmLayoutMeters::kElbowOffsetFromShoulderM +
// kWristOffsetFromElbowM). 2.0 m keeps the envelope comfortably inside full
// arm extension while leaving slack for the tool tip and final approach/
// alignment maneuvers, rather than requiring the wrist itself to touch the
// target surface.
struct ManipulatorReachEnvelopeMeters {
    static constexpr double kMaxWristRangeToSurfaceM = 2.0;
};

// Fails closed (nullopt) -- never fabricates a result -- when there is no
// selected target, the selected target has since been deregistered, or the
// queried arm is not in the same fully-deployed/not-mid-transition steady
// state command_joint_target_degrees itself requires before accepting a
// joint command. Matches this codebase's existing no-guessing contract (see
// target_selection.hpp's select_target_telemetry and
// manipulator.hpp's command_joint_target_degrees).
[[nodiscard]] inline std::optional<ManipulatorReachStatus> manipulator_reach_status(
    ManipulatorArmId id,
    const ManipulatorArmState& arm_state,
    ProbeWorldPose probe_pose,
    const std::vector<StaticSphereBody>& bodies,
    const std::string& selected_target_body_id) noexcept {
    if (selected_target_body_id.empty()) return std::nullopt;
    if (!arm_state.is_deployed || arm_state.is_stowing || arm_state.is_deploying) return std::nullopt;

    const auto target = std::find_if(bodies.begin(), bodies.end(), [&selected_target_body_id](const StaticSphereBody& body) {
        return body.body_id == selected_target_body_id;
    });
    if (target == bodies.end()) return std::nullopt;

    const ManipulatorArmContactSamples samples =
        manipulator_arm_contact_samples(id, arm_state.deployment_fraction, arm_state.angles);
    const Vector3d wrist_world_m = contact_add(
        probe_pose.position_m,
        rotate_local_contact_offset(samples.wrist.center_m, probe_pose.attitude_degrees));

    const double range_m = surface_range_to_body(wrist_world_m, *target);

    ManipulatorReachStatus status;
    status.wrist_range_to_surface_m = range_m;
    status.in_reach = range_m <= ManipulatorReachEnvelopeMeters::kMaxWristRangeToSurfaceM;
    status.remaining_distance_m = std::max(0.0, range_m - ManipulatorReachEnvelopeMeters::kMaxWristRangeToSurfaceM);
    return status;
}

// Runtime convenience overload mirroring target_cycle_runtime.hpp's
// cycle_next_target_selection(): reads DamageAwareProbeRuntime's live pose,
// registered bodies, and selected-target id rather than requiring the
// caller to unpack them first. Recomputed fresh from live state on every
// call, exactly like selected_target_status() and GetManipulatorArmStates(),
// never cached.
[[nodiscard]] inline std::optional<ManipulatorReachStatus> manipulator_reach_status(
    const DamageAwareProbeRuntime& runtime,
    ManipulatorArmId id,
    const ManipulatorArmState& arm_state) noexcept {
    const ProbeStateSnapshot& state = runtime.snapshot();
    const TargetSelectionStatus selection = runtime.selected_target_status();
    return manipulator_reach_status(
        id,
        arm_state,
        ProbeWorldPose{state.position_m, state.attitude_degrees},
        runtime.static_bodies(),
        selection.has_selection ? selection.body_id : std::string{});
}

} // namespace everward::simulation
