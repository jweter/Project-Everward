#include "everward/simulation/manipulator_move.hpp"

#include "everward/simulation/manipulator_grasp.hpp"

#undef NDEBUG
#include <algorithm>
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

void test_update_static_sphere_body_position_moves_the_registered_body() {
    DamageAwareProbeRuntime runtime = DamageAwareProbeRuntime::make_canonical_ev0001();
    runtime.add_static_sphere_body({"rock", {10.0, 0.0, 0.0}, 0.5});

    runtime.update_static_sphere_body_position("rock", {20.0, -5.0, 3.0});

    const auto& bodies = runtime.static_bodies();
    const auto found = std::find_if(bodies.begin(), bodies.end(), [](const StaticSphereBody& body) {
        return body.body_id == "rock";
    });
    assert(found != bodies.end());
    assert(nearly_equal(found->center_m, Vector3d{20.0, -5.0, 3.0}));
    // Only center_m changes; identity/radius are untouched.
    assert(found->radius_m == 0.5);
}

void test_update_static_sphere_body_position_on_unregistered_id_is_a_no_op() {
    DamageAwareProbeRuntime runtime = DamageAwareProbeRuntime::make_canonical_ev0001();
    runtime.add_static_sphere_body({"rock", {10.0, 0.0, 0.0}, 0.5});

    // Body was cleared/never registered under this id -- must not throw or
    // fabricate a new registration.
    runtime.update_static_sphere_body_position("stale-id", {999.0, 999.0, 999.0});

    assert(runtime.static_bodies().size() == 1);
    assert(nearly_equal(runtime.static_bodies().front().center_m, Vector3d{10.0, 0.0, 0.0}));
}

void test_grasped_body_wired_into_authoritative_position_updates_downstream_telemetry() {
    // Exercises the exact sequence ProbeSimulationAdapter::TickComponent
    // performs each fixed step: compute grasped_target_position for the
    // holding arm, then feed it into update_static_sphere_body_position.
    // Proves target-selection telemetry -- a reader that never changed for
    // this slice -- automatically reflects the carried body's new position
    // once center_m itself moves, with no second position path introduced.
    DamageAwareProbeRuntime runtime = DamageAwareProbeRuntime::make_canonical_ev0001();
    ManipulatorRig rig = deployed_rig(ManipulatorArmId::Port);

    const ManipulatorArmContactSamples samples = manipulator_arm_contact_samples(ManipulatorArmId::Port, 1.0, {});
    const Vector3d world_wrist = contact_add(runtime.snapshot().position_m, samples.wrist.center_m);
    runtime.add_static_sphere_body({"rock", world_wrist, 0.05});
    runtime.select_target("rock");
    assert(attempt_grasp_selected_target(rig, runtime, ManipulatorArmId::Port));

    // Simulate one adapter TickComponent step's worth of probe translation
    // without exercising contact resolution, matching
    // test_grasped_target_follows_probe_translation's direct-pose approach.
    const ProbeWorldPose moved_pose{Vector3d{100.0, -25.0, 5.0}, {}};
    const auto moved = grasped_target_position(ManipulatorArmId::Port, rig.arm(ManipulatorArmId::Port), moved_pose);
    assert(moved.has_value());
    runtime.update_static_sphere_body_position(moved->body_id, moved->world_position_m);

    const auto selection = runtime.selected_target_status();
    assert(selection.has_selection);
    assert(selection.body_id == "rock");
    // The body's registered position now equals the wrist's current world
    // position, not its original pre-grasp registration point.
    const auto& bodies = runtime.static_bodies();
    const auto found = std::find_if(bodies.begin(), bodies.end(), [](const StaticSphereBody& body) {
        return body.body_id == "rock";
    });
    assert(found != bodies.end());
    assert(nearly_equal(found->center_m, moved->world_position_m));
    assert(!nearly_equal(found->center_m, world_wrist));
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
    test_update_static_sphere_body_position_moves_the_registered_body();
    test_update_static_sphere_body_position_on_unregistered_id_is_a_no_op();
    test_grasped_body_wired_into_authoritative_position_updates_downstream_telemetry();
    test_runtime_overload_matches_free_function();

    std::puts("manipulator_move_tests: all tests passed");
    return 0;
}
