#pragma once

#include "everward/simulation/manipulator.hpp"
#include "everward/simulation/manipulator_reach.hpp"
#include "everward/simulation/target_selection.hpp"
#include "everward/simulation/types.hpp"

#include <string>
#include <vector>

namespace everward::simulation {

// Slice 7 (PHASE2_VERTICAL_SLICE_PLAN.md) minimum interaction: "grasp or
// dock with a simple object". This is the first sub-slice past read-only
// reach telemetry: it introduces exactly one new piece of authoritative
// state (ManipulatorArmState::grasped_target_body_id) and nothing else --
// no move/reposition-with-the-probe mechanics exist yet, so a grasped
// object does not currently follow the probe or the arm.
//
// The proximity gate reuses manipulator_reach_status() unchanged: a grasp
// attempt succeeds exactly when the same REACH row the manipulator HUD page
// already reports would read "IN REACH", so this never introduces a second,
// possibly inconsistent, notion of "close enough". Every other invariant
// (must be fully deployed, at most one held object, cannot stow while
// holding something) is enforced by ManipulatorRig::begin_grasp itself.

// Fails closed (false, no exception) whenever manipulator_reach_status
// would fail closed or report out-of-reach -- no selected target, a
// deregistered selection, an arm that is not fully deployed, or a target
// still beyond the fixed reach envelope. Only once genuinely in reach does
// this mutate rig state via begin_grasp, which still throws for its own
// already-grasping edge case (that is a caller/programming error, not an
// expected "not yet" outcome, so it is not folded into the false return).
[[nodiscard]] inline bool attempt_grasp_selected_target(
    ManipulatorRig& rig,
    ManipulatorArmId id,
    ProbeWorldPose probe_pose,
    const std::vector<StaticSphereBody>& bodies,
    const std::string& selected_target_body_id) {
    const auto reach = manipulator_reach_status(id, rig.arm(id), probe_pose, bodies, selected_target_body_id);
    if (!reach.has_value() || !reach->in_reach) return false;
    rig.begin_grasp(id, selected_target_body_id);
    return true;
}

// Runtime convenience overload mirroring manipulator_reach_status's
// DamageAwareProbeRuntime overload: reads live pose, registered bodies, and
// the current target selection rather than requiring the caller to unpack
// them first.
[[nodiscard]] inline bool attempt_grasp_selected_target(
    ManipulatorRig& rig,
    const DamageAwareProbeRuntime& runtime,
    ManipulatorArmId id) {
    const ProbeStateSnapshot& state = runtime.snapshot();
    const TargetSelectionStatus selection = runtime.selected_target_status();
    return attempt_grasp_selected_target(
        rig,
        id,
        ProbeWorldPose{state.position_m, state.attitude_degrees},
        runtime.static_bodies(),
        selection.has_selection ? selection.body_id : std::string{});
}

} // namespace everward::simulation
