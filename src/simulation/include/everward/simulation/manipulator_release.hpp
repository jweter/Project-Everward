#pragma once

#include "everward/simulation/compound_contact.hpp"
#include "everward/simulation/impact_damage.hpp"
#include "everward/simulation/manipulator.hpp"
#include "everward/simulation/manipulator_hull_contact.hpp"
#include "everward/simulation/types.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace everward::simulation {

// Slice 7 (PHASE2_VERTICAL_SLICE_PLAN.md) completion sub-slice:
// "release-with-consequence". Every prior Slice 7 sub-slice document
// (PHASE2_MANIPULATOR_GRASP_TEST.md, PHASE2_MANIPULATOR_MOVE_TEST.md) has
// explicitly named the same outstanding gap: ManipulatorRig::release_grasp
// unconditionally lets go of the held body wherever it currently is, with no
// re-collision against the probe's own hull. This closes exactly that one
// gap: a release attempt that would leave the released body overlapping the
// probe's own compound hull envelope -- the same five-sphere approximation
// software_policy.hpp's swept contact and manipulator_hull_contact.hpp's
// arm/hull guard already use -- fails closed instead of embedding the object
// in the probe body, mirroring the "is it safe" wrapper layer
// attempt_grasp_selected_target already established over begin_grasp for
// "is it close enough". release_grasp itself stays unconditional exactly as
// documented; this module is where the extra "is it safe to let go here"
// gate lives.
//
// Releasing over empty space, near another registered body, or into a
// mining/storage flow still has no consequence beyond this hull check --
// that stays intentionally out of scope, matching
// PHASE2_MANIPULATOR_MOVE_TEST.md's "explicitly not complete" list, which
// only ever named embedding-in-the-probe's-own-hull as a defect.
//
// The held body's position/radius are read directly from the already-
// authoritative registered-body list (kept current every tick by
// manipulator_move.hpp's wiring into update_static_sphere_body_position)
// rather than recomputed through a second placement formula.

[[nodiscard]] inline bool sphere_intersects_compound_hull(
    Vector3d body_world_position_m,
    double body_radius_m,
    ProbeWorldPose probe_pose,
    const ProbeCompoundCollisionEnvelope& hull) noexcept {
    for (const ProbeCollisionSphereSample& sample : hull.samples) {
        const Vector3d world_center = contact_add(
            probe_pose.position_m,
            rotate_local_contact_offset(sample.local_center_m, probe_pose.attitude_degrees));
        const Vector3d delta = contact_subtract(body_world_position_m, world_center);
        const double combined_radius = body_radius_m + sample.radius_m;
        if (contact_dot(delta, delta) < combined_radius * combined_radius) {
            return true;
        }
    }
    return false;
}

// Fails closed (false, no mutation) whenever the queried arm holds nothing,
// the held body is no longer registered (a deregistered grasp, mirroring
// attempt_grasp_selected_target's own deregistered-selection fail-closed
// case), or releasing now would leave that body overlapping the probe's own
// hull. Only once none of those hold does this call release_grasp.
[[nodiscard]] inline bool attempt_release_grasped_target(
    ManipulatorRig& rig,
    ManipulatorArmId id,
    ProbeWorldPose probe_pose,
    const std::vector<StaticSphereBody>& bodies) {
    const std::string held_id = rig.arm(id).grasped_target_body_id;
    if (held_id.empty()) return false;

    const auto found = std::find_if(bodies.begin(), bodies.end(), [&held_id](const StaticSphereBody& body) {
        return body.body_id == held_id;
    });
    if (found == bodies.end()) return false;

    if (sphere_intersects_compound_hull(
            found->center_m, found->radius_m, probe_pose, ProbeCompoundCollisionEnvelope{})) {
        return false;
    }
    rig.release_grasp(id);
    return true;
}

// Runtime convenience overload mirroring attempt_grasp_selected_target's
// DamageAwareProbeRuntime overload: reads live pose and the current
// registered-body list rather than requiring the caller to unpack them
// first.
[[nodiscard]] inline bool attempt_release_grasped_target(
    ManipulatorRig& rig,
    const DamageAwareProbeRuntime& runtime,
    ManipulatorArmId id) {
    const ProbeStateSnapshot& state = runtime.snapshot();
    return attempt_release_grasped_target(
        rig, id, ProbeWorldPose{state.position_m, state.attitude_degrees}, runtime.static_bodies());
}

} // namespace everward::simulation
