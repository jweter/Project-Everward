#pragma once

#include "everward/simulation/impact_damage.hpp"
#include "everward/simulation/manipulator.hpp"
#include "everward/simulation/types.hpp"

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

namespace everward::simulation {

// Slice 12 (PHASE2_VERTICAL_SLICE_PLAN.md) "resource/sample loop" initial
// scope: "manipulator/tool acquisition" of a sampled object, as opposed to
// mining.hpp's repeated-cycle tool-beam extraction from a deposit. A sample
// is a bounded object collected once, in full, the moment it is grasped --
// there is no partial extraction, survey gate, or extraction-per-cycle
// concept here, unlike mining.
//
// This module intentionally only computes the eligibility check and the
// mutation of ManipulatorRig's own grasp state (rig.release_grasp), the same
// division of responsibility manipulator_release.hpp already established:
// removing the collected body from the registered-body list and crediting
// authoritative storage are a different owner's mutations
// (DamageAwareProbeRuntime::remove_static_sphere_body /
// add_stored_material_kg), composed by the caller from this function's
// result -- exactly how the Unreal adapter already composes
// grasped_target_position() (a different owner's read) with
// update_static_sphere_body_position() (the runtime's own mutation) for the
// "move" sub-slice. No second forward-kinematics, placement, or storage-
// crediting path is invented here.

struct SampleCollectionResult {
    std::string body_id;
    std::string material_id;
    double mass_kg{0.0};
};

// Fails closed (nullopt, no mutation) whenever the queried arm holds
// nothing, the held body is no longer registered (a deregistered grasp,
// mirroring attempt_release_grasped_target's own deregistered-grasp
// fail-closed case), or the held body has no positive sample_mass_kg -- a
// plain reference/navigation body or an ordinary mining deposit target is
// not manipulator-collectible. Only once genuinely eligible does this
// release the grasp (the collected object leaves the arm the same instant
// it leaves the world) and report what to credit.
[[nodiscard]] inline std::optional<SampleCollectionResult> attempt_collect_grasped_target(
    ManipulatorRig& rig,
    ManipulatorArmId id,
    const std::vector<StaticSphereBody>& bodies) {
    const std::string held_id = rig.arm(id).grasped_target_body_id;
    if (held_id.empty()) return std::nullopt;

    const auto found = std::find_if(bodies.begin(), bodies.end(), [&held_id](const StaticSphereBody& body) {
        return body.body_id == held_id;
    });
    if (found == bodies.end()) return std::nullopt;
    if (!(found->sample_mass_kg > 0.0)) return std::nullopt;

    const SampleCollectionResult result{found->body_id, found->material_id, found->sample_mass_kg};
    rig.release_grasp(id);
    return result;
}

// Runtime convenience overload mirroring attempt_release_grasped_target's
// DamageAwareProbeRuntime overload: reads the current registered-body list
// rather than requiring the caller to unpack it first. Still does not itself
// remove the body or credit storage -- see this module's header comment for
// why that composition stays at the caller (the runtime, not this
// engine-independent module, owns both of those mutation boundaries).
[[nodiscard]] inline std::optional<SampleCollectionResult> attempt_collect_grasped_target(
    ManipulatorRig& rig,
    const DamageAwareProbeRuntime& runtime,
    ManipulatorArmId id) {
    return attempt_collect_grasped_target(rig, id, runtime.static_bodies());
}

} // namespace everward::simulation
