#include "everward/simulation/manipulator_grasp.hpp"

#undef NDEBUG
#include <cassert>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using everward::simulation::DamageAwareProbeRuntime;
using everward::simulation::ManipulatorArmContactSamples;
using everward::simulation::ManipulatorArmId;
using everward::simulation::ManipulatorRig;
using everward::simulation::ProbeWorldPose;
using everward::simulation::StaticSphereBody;
using everward::simulation::Vector3d;
using everward::simulation::attempt_grasp_selected_target;
using everward::simulation::contact_add;
using everward::simulation::manipulator_arm_contact_samples;

ManipulatorRig deployed_rig(ManipulatorArmId id) {
    ManipulatorRig rig;
    rig.begin_deploy(id);
    rig.advance(ManipulatorRig::kDeployStowDurationS);
    return rig;
}

void test_no_target_selected_fails_closed_without_mutating_rig() {
    ManipulatorRig rig = deployed_rig(ManipulatorArmId::Port);
    const std::vector<StaticSphereBody> bodies{StaticSphereBody{"rock", {10.0, 0.0, 0.0}, 1.0}};

    const bool grasped = attempt_grasp_selected_target(rig, ManipulatorArmId::Port, ProbeWorldPose{}, bodies, "");
    assert(!grasped);
    assert(rig.arm(ManipulatorArmId::Port).grasped_target_body_id.empty());
}

void test_arm_not_deployed_fails_closed() {
    ManipulatorRig rig; // stowed
    const ManipulatorArmContactSamples samples =
        manipulator_arm_contact_samples(ManipulatorArmId::Port, 1.0, {});
    const std::vector<StaticSphereBody> bodies{StaticSphereBody{"rock", samples.wrist.center_m, 0.5}};

    const bool grasped = attempt_grasp_selected_target(rig, ManipulatorArmId::Port, ProbeWorldPose{}, bodies, "rock");
    assert(!grasped);
}

void test_out_of_reach_fails_closed_without_mutating_rig() {
    ManipulatorRig rig = deployed_rig(ManipulatorArmId::Port);
    const ManipulatorArmContactSamples samples =
        manipulator_arm_contact_samples(ManipulatorArmId::Port, 1.0, {});
    const Vector3d far_center = contact_add(samples.wrist.center_m, Vector3d{10.0, 0.0, 0.0});
    const std::vector<StaticSphereBody> bodies{StaticSphereBody{"rock", far_center, 1.0}};

    const bool grasped = attempt_grasp_selected_target(rig, ManipulatorArmId::Port, ProbeWorldPose{}, bodies, "rock");
    assert(!grasped);
    assert(rig.arm(ManipulatorArmId::Port).grasped_target_body_id.empty());
}

void test_in_reach_succeeds_and_matches_reach_status_exactly() {
    ManipulatorRig rig = deployed_rig(ManipulatorArmId::Port);
    const ManipulatorArmContactSamples samples =
        manipulator_arm_contact_samples(ManipulatorArmId::Port, 1.0, {});
    // Same construction manipulator_reach_tests.cpp uses for "in reach":
    // body surface sits exactly at the wrist center.
    const std::vector<StaticSphereBody> bodies{StaticSphereBody{"rock", samples.wrist.center_m, 0.5}};

    const bool grasped = attempt_grasp_selected_target(rig, ManipulatorArmId::Port, ProbeWorldPose{}, bodies, "rock");
    assert(grasped);
    assert(rig.arm(ManipulatorArmId::Port).grasped_target_body_id == "rock");
}

void test_grasp_deregistered_selection_fails_closed() {
    ManipulatorRig rig = deployed_rig(ManipulatorArmId::Port);
    const std::vector<StaticSphereBody> bodies{StaticSphereBody{"other-rock", {0.5, 0.0, 0.0}, 0.1}};

    const bool grasped =
        attempt_grasp_selected_target(rig, ManipulatorArmId::Port, ProbeWorldPose{}, bodies, "stale-id");
    assert(!grasped);
}

void test_grasp_gate_stays_scoped_to_the_queried_arm() {
    ManipulatorRig rig;
    rig.begin_deploy(ManipulatorArmId::Port);
    rig.advance(ManipulatorRig::kDeployStowDurationS);
    // Starboard stays stowed.
    const ManipulatorArmContactSamples starboard_samples =
        manipulator_arm_contact_samples(ManipulatorArmId::Starboard, 0.0, {});
    const std::vector<StaticSphereBody> bodies{StaticSphereBody{"rock", starboard_samples.wrist.center_m, 0.5}};

    const bool grasped =
        attempt_grasp_selected_target(rig, ManipulatorArmId::Starboard, ProbeWorldPose{}, bodies, "rock");
    assert(!grasped);
    assert(rig.arm(ManipulatorArmId::Starboard).grasped_target_body_id.empty());
}

void test_runtime_overload_matches_free_function() {
    DamageAwareProbeRuntime runtime = DamageAwareProbeRuntime::make_canonical_ev0001();

    ManipulatorRig rig;
    rig.begin_deploy(ManipulatorArmId::Port);
    rig.advance(ManipulatorRig::kDeployStowDurationS);

    const ManipulatorArmContactSamples samples = manipulator_arm_contact_samples(ManipulatorArmId::Port, 1.0, {});
    const Vector3d world_wrist = contact_add(runtime.snapshot().position_m, samples.wrist.center_m);
    runtime.add_static_sphere_body({"rock", world_wrist, 0.05});

    // No selection yet: fails closed even though a fully-deployed arm and a
    // registered body sitting right at the wrist both exist.
    assert(!attempt_grasp_selected_target(rig, runtime, ManipulatorArmId::Port));

    runtime.select_target("rock");
    assert(attempt_grasp_selected_target(rig, runtime, ManipulatorArmId::Port));
    assert(rig.arm(ManipulatorArmId::Port).grasped_target_body_id == "rock");
}

} // namespace

int main() {
    test_no_target_selected_fails_closed_without_mutating_rig();
    test_arm_not_deployed_fails_closed();
    test_out_of_reach_fails_closed_without_mutating_rig();
    test_in_reach_succeeds_and_matches_reach_status_exactly();
    test_grasp_deregistered_selection_fails_closed();
    test_grasp_gate_stays_scoped_to_the_queried_arm();
    test_runtime_overload_matches_free_function();

    std::puts("manipulator_grasp_tests: all tests passed");
    return 0;
}
