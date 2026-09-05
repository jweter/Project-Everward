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
// Eleventh parallel-safe sub-slice: PHASE2_MANIPULATOR_RELEASE_TEST.md's
// "explicitly not complete" list named releasing near another registered
// body as the first still-open release-with-consequence gap (only the
// probe's own hull was checked). This closes that one: a release attempt
// that would leave the released body overlapping any other currently
// registered physical body -- not just the probe's own hull -- also fails
// closed, reusing the same sphere-overlap test already used against the
// hull's spheres rather than inventing a second formula.
//
// A "place"/"drop toward a target location" mechanic, automatic hand-off
// into the mining/storage flow, and released-object velocity/momentum still
// have no consequence beyond these two overlap checks -- that stays
// intentionally out of scope, matching PHASE2_MANIPULATOR_MOVE_TEST.md's
// "explicitly not complete" list.
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

// Mirrors sphere_intersects_compound_hull's own overlap test against every
// other currently registered body instead of the probe's hull spheres,
// skipping the held body itself (it is always "overlapping" its own prior
// position and is not a collision target for its own release).
[[nodiscard]] inline bool sphere_intersects_other_registered_body(
    const std::string& held_body_id,
    Vector3d body_world_position_m,
    double body_radius_m,
    const std::vector<StaticSphereBody>& bodies) noexcept {
    for (const StaticSphereBody& other : bodies) {
        if (other.body_id == held_body_id) continue;
        const Vector3d delta = contact_subtract(body_world_position_m, other.center_m);
        const double combined_radius = body_radius_m + other.radius_m;
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
// hull or any other registered physical body. Only once none of those hold
// does this call release_grasp.
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
    if (sphere_intersects_other_registered_body(held_id, found->center_m, found->radius_m, bodies)) {
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
