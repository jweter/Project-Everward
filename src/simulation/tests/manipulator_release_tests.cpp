#include "everward/simulation/manipulator_release.hpp"

#include "everward/simulation/manipulator_grasp.hpp"

#undef NDEBUG
#include <cassert>
#include <cstdio>
#include <string>
#include <vector>

namespace {

using everward::simulation::DamageAwareProbeRuntime;
using everward::simulation::ManipulatorArmContactSamples;
using everward::simulation::ManipulatorArmId;
using everward::simulation::ManipulatorRig;
using everward::simulation::ProbeCompoundCollisionEnvelope;
using everward::simulation::ProbeWorldPose;
using everward::simulation::StaticSphereBody;
using everward::simulation::Vector3d;
using everward::simulation::attempt_grasp_selected_target;
using everward::simulation::attempt_release_grasped_target;
using everward::simulation::contact_add;
using everward::simulation::manipulator_arm_contact_samples;
using everward::simulation::sphere_intersects_compound_hull;
using everward::simulation::sphere_intersects_other_registered_body;

ManipulatorRig deployed_rig(ManipulatorArmId id) {
    ManipulatorRig rig;
    rig.begin_deploy(id);
    rig.advance(ManipulatorRig::kDeployStowDurationS);
    return rig;
}

// Grasps "rock" on the given arm at the arm's own current wrist position, so
// the held body starts out clear of the hull (the wrist reach envelope sits
// well outside the five-sphere hull approximation).
ManipulatorRig grasped_rig(ManipulatorArmId id, const std::string& body_id, std::vector<StaticSphereBody>& bodies) {
    ManipulatorRig rig = deployed_rig(id);
    const ManipulatorArmContactSamples samples = manipulator_arm_contact_samples(id, 1.0, {});
    bodies.push_back(StaticSphereBody{body_id, samples.wrist.center_m, 0.05});
    const bool grasped = attempt_grasp_selected_target(rig, id, ProbeWorldPose{}, bodies, body_id);
    assert(grasped);
    return rig;
}

void test_release_nothing_held_fails_closed() {
    ManipulatorRig rig = deployed_rig(ManipulatorArmId::Port);
    const std::vector<StaticSphereBody> bodies{};

    const bool released = attempt_release_grasped_target(rig, ManipulatorArmId::Port, ProbeWorldPose{}, bodies);
    assert(!released);
}

void test_release_clear_of_hull_succeeds() {
    std::vector<StaticSphereBody> bodies;
    ManipulatorRig rig = grasped_rig(ManipulatorArmId::Port, "rock", bodies);

    const bool released = attempt_release_grasped_target(rig, ManipulatorArmId::Port, ProbeWorldPose{}, bodies);
    assert(released);
    assert(rig.arm(ManipulatorArmId::Port).grasped_target_body_id.empty());
}

void test_release_into_hull_fails_closed_and_keeps_holding() {
    std::vector<StaticSphereBody> bodies;
    ManipulatorRig rig = grasped_rig(ManipulatorArmId::Port, "rock", bodies);

    // Overwrite the held body's registered position (as manipulator_move.hpp's
    // wiring would while carrying it toward the hull) to sit exactly at the
    // central hull sample, well inside its 1.60 m radius.
    bodies.front().center_m = ProbeCompoundCollisionEnvelope{}.samples[1].local_center_m;

    const bool released = attempt_release_grasped_target(rig, ManipulatorArmId::Port, ProbeWorldPose{}, bodies);
    assert(!released);
    assert(rig.arm(ManipulatorArmId::Port).grasped_target_body_id == "rock");
}

void test_release_near_another_registered_body_fails_closed_and_keeps_holding() {
    std::vector<StaticSphereBody> bodies;
    ManipulatorRig rig = grasped_rig(ManipulatorArmId::Port, "rock", bodies);

    // Register a second body overlapping exactly where "rock" would land if
    // released now (well clear of the probe's own hull, so only the new
    // other-body check can be responsible for the rejection).
    bodies.push_back(StaticSphereBody{"other-target", bodies.front().center_m, 0.05});

    const bool released = attempt_release_grasped_target(rig, ManipulatorArmId::Port, ProbeWorldPose{}, bodies);
    assert(!released);
    assert(rig.arm(ManipulatorArmId::Port).grasped_target_body_id == "rock");
}

void test_release_clear_of_other_registered_bodies_succeeds() {
    std::vector<StaticSphereBody> bodies;
    ManipulatorRig rig = grasped_rig(ManipulatorArmId::Port, "rock", bodies);

    // A second registered body far away must not block an otherwise-clear
    // release.
    bodies.push_back(StaticSphereBody{"other-target", Vector3d{1000.0, 1000.0, 1000.0}, 0.05});

    const bool released = attempt_release_grasped_target(rig, ManipulatorArmId::Port, ProbeWorldPose{}, bodies);
    assert(released);
    assert(rig.arm(ManipulatorArmId::Port).grasped_target_body_id.empty());
}

void test_sphere_intersects_other_registered_body_skips_the_held_body_itself() {
    std::vector<StaticSphereBody> bodies;
    bodies.push_back(StaticSphereBody{"rock", Vector3d{5.0, 0.0, 0.0}, 0.05});

    // The held body always geometrically "overlaps" its own recorded
    // position; the held id must be excluded or every release would fail.
    assert(!sphere_intersects_other_registered_body("rock", bodies.front().center_m, 0.05, bodies));

    bodies.push_back(StaticSphereBody{"other-target", bodies.front().center_m, 0.05});
    assert(sphere_intersects_other_registered_body("rock", bodies.front().center_m, 0.05, bodies));
}

void test_release_deregistered_body_fails_closed() {
    std::vector<StaticSphereBody> bodies;
    ManipulatorRig rig = grasped_rig(ManipulatorArmId::Port, "rock", bodies);
    bodies.clear(); // simulate the held body vanishing from the registry

    const bool released = attempt_release_grasped_target(rig, ManipulatorArmId::Port, ProbeWorldPose{}, bodies);
    assert(!released);
    assert(rig.arm(ManipulatorArmId::Port).grasped_target_body_id == "rock");
}

void test_release_gate_stays_scoped_to_the_queried_arm() {
    std::vector<StaticSphereBody> bodies;
    ManipulatorRig rig = grasped_rig(ManipulatorArmId::Port, "rock", bodies);
    rig.begin_deploy(ManipulatorArmId::Starboard);
    rig.advance(ManipulatorRig::kDeployStowDurationS);

    const bool released = attempt_release_grasped_target(rig, ManipulatorArmId::Starboard, ProbeWorldPose{}, bodies);
    assert(!released); // Starboard holds nothing, regardless of Port's grasp
    assert(rig.arm(ManipulatorArmId::Port).grasped_target_body_id == "rock");
}

void test_release_accounts_for_probe_pose_not_just_local_origin() {
    std::vector<StaticSphereBody> bodies;
    ManipulatorRig rig = deployed_rig(ManipulatorArmId::Port);
    const ManipulatorArmContactSamples samples = manipulator_arm_contact_samples(ManipulatorArmId::Port, 1.0, {});
    const ProbeWorldPose pose{Vector3d{500.0, -20.0, 8.0}, {}};
    const Vector3d wrist_world = contact_add(pose.position_m, samples.wrist.center_m);
    bodies.push_back(StaticSphereBody{"rock", wrist_world, 0.05});
    assert(attempt_grasp_selected_target(rig, ManipulatorArmId::Port, pose, bodies, "rock"));

    // Clear of the hull once the probe's own translated position is used...
    assert(attempt_release_grasped_target(rig, ManipulatorArmId::Port, pose, bodies));
}

void test_sphere_intersects_compound_hull_matches_direct_distance_check() {
    const ProbeCompoundCollisionEnvelope hull{};
    const auto& central = hull.samples[1]; // central computation/reactor hull, radius 1.60 m
    assert(sphere_intersects_compound_hull(central.local_center_m, 0.1, ProbeWorldPose{}, hull));

    const Vector3d far_away{1000.0, 1000.0, 1000.0};
    assert(!sphere_intersects_compound_hull(far_away, 0.1, ProbeWorldPose{}, hull));
}

void test_runtime_overload_matches_free_function() {
    DamageAwareProbeRuntime runtime = DamageAwareProbeRuntime::make_canonical_ev0001();
    ManipulatorRig rig = deployed_rig(ManipulatorArmId::Port);

    const ManipulatorArmContactSamples samples = manipulator_arm_contact_samples(ManipulatorArmId::Port, 1.0, {});
    const Vector3d world_wrist = contact_add(runtime.snapshot().position_m, samples.wrist.center_m);
    runtime.add_static_sphere_body({"rock", world_wrist, 0.05});
    runtime.select_target("rock");
    assert(attempt_grasp_selected_target(rig, runtime, ManipulatorArmId::Port));

    assert(attempt_release_grasped_target(rig, runtime, ManipulatorArmId::Port));
    assert(rig.arm(ManipulatorArmId::Port).grasped_target_body_id.empty());
}

} // namespace

int main() {
    test_release_nothing_held_fails_closed();
    test_release_clear_of_hull_succeeds();
    test_release_into_hull_fails_closed_and_keeps_holding();
    test_release_near_another_registered_body_fails_closed_and_keeps_holding();
    test_release_clear_of_other_registered_bodies_succeeds();
    test_sphere_intersects_other_registered_body_skips_the_held_body_itself();
    test_release_deregistered_body_fails_closed();
    test_release_gate_stays_scoped_to_the_queried_arm();
    test_release_accounts_for_probe_pose_not_just_local_origin();
    test_sphere_intersects_compound_hull_matches_direct_distance_check();
    test_runtime_overload_matches_free_function();

    std::puts("manipulator_release_tests: all tests passed");
    return 0;
}
