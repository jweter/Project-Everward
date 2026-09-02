#include "everward/simulation/manipulator_move.hpp"

#include "everward/simulation/manipulator_grasp.hpp"

#undef NDEBUG
#include <cassert>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace {

using everward::simulation::DamageAwareProbeRuntime;
using everward::simulation::GraspedTargetPosition;
using everward::simulation::ManipulatorArmContactSamples;
using everward::simulation::ManipulatorArmId;
using everward::simulation::ManipulatorRig;
using everward::simulation::ProbeWorldPose;
using everward::simulation::StaticSphereBody;
using everward::simulation::Vector3d;
using everward::simulation::attempt_grasp_selected_target;
using everward::simulation::contact_add;
using everward::simulation::grasped_target_position;
using everward::simulation::manipulator_arm_contact_samples;

bool nearly_equal(double a, double b, double epsilon = 1e-9) {
    return std::fabs(a - b) <= epsilon;
}

bool nearly_equal(Vector3d a, Vector3d b) {
    return nearly_equal(a.x, b.x) && nearly_equal(a.y, b.y) && nearly_equal(a.z, b.z);
}

ManipulatorRig deployed_rig(ManipulatorArmId id) {
    ManipulatorRig rig;
    rig.begin_deploy(id);
    rig.advance(ManipulatorRig::kDeployStowDurationS);
    return rig;
}

void test_empty_grasp_returns_nullopt() {
    ManipulatorRig rig = deployed_rig(ManipulatorArmId::Port);
    const auto result = grasped_target_position(ManipulatorArmId::Port, rig.arm(ManipulatorArmId::Port), ProbeWorldPose{});
    assert(!result.has_value());
}

void test_grasped_target_reports_wrist_world_position_at_probe_origin() {
    ManipulatorRig rig = deployed_rig(ManipulatorArmId::Port);
    const ManipulatorArmContactSamples samples = manipulator_arm_contact_samples(ManipulatorArmId::Port, 1.0, {});
    const std::vector<StaticSphereBody> bodies{StaticSphereBody{"rock", samples.wrist.center_m, 0.5}};

    assert(attempt_grasp_selected_target(rig, ManipulatorArmId::Port, ProbeWorldPose{}, bodies, "rock"));

    const auto result = grasped_target_position(ManipulatorArmId::Port, rig.arm(ManipulatorArmId::Port), ProbeWorldPose{});
    assert(result.has_value());
    assert(result->body_id == "rock");
    assert(nearly_equal(result->world_position_m, samples.wrist.center_m));
}

void test_grasped_target_follows_probe_translation() {
    ManipulatorRig rig = deployed_rig(ManipulatorArmId::Port);
    const ManipulatorArmContactSamples samples = manipulator_arm_contact_samples(ManipulatorArmId::Port, 1.0, {});
    const std::vector<StaticSphereBody> bodies{StaticSphereBody{"rock", samples.wrist.center_m, 0.5}};
    assert(attempt_grasp_selected_target(rig, ManipulatorArmId::Port, ProbeWorldPose{}, bodies, "rock"));

    const ProbeWorldPose moved_pose{Vector3d{100.0, -25.0, 5.0}, {}};
    const auto result = grasped_target_position(ManipulatorArmId::Port, rig.arm(ManipulatorArmId::Port), moved_pose);
    assert(result.has_value());
    assert(nearly_equal(result->world_position_m, contact_add(moved_pose.position_m, samples.wrist.center_m)));
}

void test_move_query_is_scoped_to_the_holding_arm() {
    ManipulatorRig rig = deployed_rig(ManipulatorArmId::Port);
    const ManipulatorArmContactSamples samples = manipulator_arm_contact_samples(ManipulatorArmId::Port, 1.0, {});
    const std::vector<StaticSphereBody> bodies{StaticSphereBody{"rock", samples.wrist.center_m, 0.5}};
    assert(attempt_grasp_selected_target(rig, ManipulatorArmId::Port, ProbeWorldPose{}, bodies, "rock"));

    // Starboard never grasped anything, so it reports no held position even
    // though Port is actively holding "rock".
    const auto starboard_result =
        grasped_target_position(ManipulatorArmId::Starboard, rig.arm(ManipulatorArmId::Starboard), ProbeWorldPose{});
    assert(!starboard_result.has_value());
}

void test_release_clears_the_move_result() {
    ManipulatorRig rig = deployed_rig(ManipulatorArmId::Port);
    const ManipulatorArmContactSamples samples = manipulator_arm_contact_samples(ManipulatorArmId::Port, 1.0, {});
    const std::vector<StaticSphereBody> bodies{StaticSphereBody{"rock", samples.wrist.center_m, 0.5}};
    assert(attempt_grasp_selected_target(rig, ManipulatorArmId::Port, ProbeWorldPose{}, bodies, "rock"));
    assert(grasped_target_position(ManipulatorArmId::Port, rig.arm(ManipulatorArmId::Port), ProbeWorldPose{}).has_value());

    rig.release_grasp(ManipulatorArmId::Port);
    const auto result = grasped_target_position(ManipulatorArmId::Port, rig.arm(ManipulatorArmId::Port), ProbeWorldPose{});
    assert(!result.has_value());
}

void test_runtime_overload_matches_free_function() {
    DamageAwareProbeRuntime runtime = DamageAwareProbeRuntime::make_canonical_ev0001();

    ManipulatorRig rig;
    rig.begin_deploy(ManipulatorArmId::Port);
    rig.advance(ManipulatorRig::kDeployStowDurationS);

    const ManipulatorArmContactSamples samples = manipulator_arm_contact_samples(ManipulatorArmId::Port, 1.0, {});
    const Vector3d world_wrist = contact_add(runtime.snapshot().position_m, samples.wrist.center_m);
    runtime.add_static_sphere_body({"rock", world_wrist, 0.05});
    runtime.select_target("rock");
    assert(attempt_grasp_selected_target(rig, runtime, ManipulatorArmId::Port));

    const auto runtime_result = grasped_target_position(runtime, ManipulatorArmId::Port, rig.arm(ManipulatorArmId::Port));
    const auto free_function_result = grasped_target_position(
        ManipulatorArmId::Port,
        rig.arm(ManipulatorArmId::Port),
        ProbeWorldPose{runtime.snapshot().position_m, runtime.snapshot().attitude_degrees});

    assert(runtime_result.has_value());
    assert(free_function_result.has_value());
    assert(runtime_result->body_id == free_function_result->body_id);
    assert(nearly_equal(runtime_result->world_position_m, free_function_result->world_position_m));
}

} // namespace

int main() {
    test_empty_grasp_returns_nullopt();
    test_grasped_target_reports_wrist_world_position_at_probe_origin();
    test_grasped_target_follows_probe_translation();
    test_move_query_is_scoped_to_the_holding_arm();
    test_release_clears_the_move_result();
    test_runtime_overload_matches_free_function();

    std::puts("manipulator_move_tests: all tests passed");
    return 0;
}
