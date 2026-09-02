#pragma once

#include "everward/simulation/compound_contact.hpp"
#include "everward/simulation/impact_damage.hpp"
#include "everward/simulation/manipulator.hpp"
#include "everward/simulation/manipulator_hull_contact.hpp"
#include "everward/simulation/types.hpp"

#include <optional>
#include <string>

namespace everward::simulation {

// Slice 7 (PHASE2_VERTICAL_SLICE_PLAN.md) minimum interaction: "move". This
// is the pure, engine-independent math foundation for the sub-slice that
// follows grasp (manipulator_grasp.hpp) -- the same incremental pattern
// target_selection.hpp and manipulator_reach.hpp already used: a read-only
// telemetry foundation lands first, then a later sub-slice wires it into
// authoritative runtime state, the adapter, and the Unreal-side actor.
//
// ManipulatorArmState::grasped_target_body_id already records which body an
// arm holds (manipulator_grasp.hpp). This module only computes, read-only,
// where that held body should currently be reported as being: the grasping
// arm's wrist world position, using the exact same forward-kinematics and
// local-to-world convention manipulator_reach_status() and
// manipulator_hull_contact.hpp's environment-collision guard already
// establish -- no second placement convention is invented here.
//
// This does NOT yet update the registered StaticSphereBody's own
// authoritative center_m. Until the next sub-slice wires that (and moves the
// Unreal-side actor to visually follow the wrist each tick), a grasped
// body's contact/target-selection/scan telemetry still reads its pre-grasp
// registered position; only this function's own result reflects the move.

struct GraspedTargetPosition {
    std::string body_id;
    Vector3d world_position_m{};
};

// Returns nullopt whenever the queried arm is not currently holding
// anything -- never fabricates a position for an empty grasp. Whenever this
// returns a value, the arm is guaranteed deployed and not mid-stow: those
// are ManipulatorRig::begin_grasp's/its stow guard's own preconditions for a
// non-empty grasped_target_body_id, so they are not re-checked here.
[[nodiscard]] inline std::optional<GraspedTargetPosition> grasped_target_position(
    ManipulatorArmId id,
    const ManipulatorArmState& arm_state,
    ProbeWorldPose probe_pose) noexcept {
    if (arm_state.grasped_target_body_id.empty()) return std::nullopt;

    const ManipulatorArmContactSamples samples =
        manipulator_arm_contact_samples(id, arm_state.deployment_fraction, arm_state.angles);
    const Vector3d wrist_world_m = contact_add(
        probe_pose.position_m,
        rotate_local_contact_offset(samples.wrist.center_m, probe_pose.attitude_degrees));

    return GraspedTargetPosition{arm_state.grasped_target_body_id, wrist_world_m};
}

// Runtime convenience overload mirroring manipulator_reach_status's
// DamageAwareProbeRuntime overload: reads live pose rather than requiring
// the caller to unpack it first.
[[nodiscard]] inline std::optional<GraspedTargetPosition> grasped_target_position(
    const DamageAwareProbeRuntime& runtime,
    ManipulatorArmId id,
    const ManipulatorArmState& arm_state) noexcept {
    const ProbeStateSnapshot& state = runtime.snapshot();
    return grasped_target_position(id, arm_state, ProbeWorldPose{state.position_m, state.attitude_degrees});
}

} // namespace everward::simulation
