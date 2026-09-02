#include "everward/simulation/manipulator_reach.hpp"

#undef NDEBUG
#include <cassert>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace {

using everward::simulation::DamageAwareProbeRuntime;
using everward::simulation::ManipulatorArmContactSamples;
using everward::simulation::ManipulatorArmId;
using everward::simulation::ManipulatorArmState;
using everward::simulation::ManipulatorReachEnvelopeMeters;
using everward::simulation::ManipulatorRig;
using everward::simulation::ProbeWorldPose;
using everward::simulation::StaticSphereBody;
using everward::simulation::Vector3d;
using everward::simulation::contact_add;
using everward::simulation::manipulator_arm_contact_samples;
using everward::simulation::manipulator_reach_status;

bool nearly_equal(double a, double b, double epsilon = 1e-6) {
    return std::fabs(a - b) <= epsilon;
}

ManipulatorArmState deployed_idle_arm_state() {
    ManipulatorArmState state;
    state.is_deployed = true;
    state.deployment_fraction = 1.0;
    return state;
}

void test_no_target_selected_returns_nullopt() {
    const ManipulatorArmState arm_state = deployed_idle_arm_state();
    const std::vector<StaticSphereBody> bodies{StaticSphereBody{"rock", {10.0, 0.0, 0.0}, 1.0}};

    const auto result = manipulator_reach_status(
        ManipulatorArmId::Starboard, arm_state, ProbeWorldPose{}, bodies, std::string{});
    assert(!result.has_value());
}

void test_arm_not_deployed_returns_nullopt() {
    ManipulatorArmState arm_state; // defaults: stowed, not deployed.
    const std::vector<StaticSphereBody> bodies{StaticSphereBody{"rock", {0.5, 0.0, 0.0}, 0.1}};

    const auto result = manipulator_reach_status(
        ManipulatorArmId::Starboard, arm_state, ProbeWorldPose{}, bodies, "rock");
    assert(!result.has_value());
}

void test_arm_mid_stow_returns_nullopt() {
    ManipulatorArmState arm_state = deployed_idle_arm_state();
    arm_state.is_stowing = true;
    const std::vector<StaticSphereBody> bodies{StaticSphereBody{"rock", {0.5, 0.0, 0.0}, 0.1}};

    const auto result = manipulator_reach_status(
        ManipulatorArmId::Starboard, arm_state, ProbeWorldPose{}, bodies, "rock");
    assert(!result.has_value());
}

void test_arm_mid_deploy_returns_nullopt() {
    ManipulatorArmState arm_state;
    arm_state.is_deploying = true;
    arm_state.deployment_fraction = 0.5;
    const std::vector<StaticSphereBody> bodies{StaticSphereBody{"rock", {0.5, 0.0, 0.0}, 0.1}};

    const auto result = manipulator_reach_status(
        ManipulatorArmId::Starboard, arm_state, ProbeWorldPose{}, bodies, "rock");
    assert(!result.has_value());
}

void test_selected_target_not_registered_returns_nullopt() {
    // A stale/since-deregistered selection must fail closed the same way
    // selected_target_status() does, never fabricate stale range.
    const ManipulatorArmState arm_state = deployed_idle_arm_state();
    const std::vector<StaticSphereBody> bodies{StaticSphereBody{"other-rock", {0.5, 0.0, 0.0}, 0.1}};

    const auto result = manipulator_reach_status(
        ManipulatorArmId::Starboard, arm_state, ProbeWorldPose{}, bodies, "stale-id");
    assert(!result.has_value());
}

void test_wrist_within_envelope_reports_in_reach() {
    const ManipulatorArmState arm_state = deployed_idle_arm_state();
    const ManipulatorArmContactSamples samples = manipulator_arm_contact_samples(
        ManipulatorArmId::Starboard, arm_state.deployment_fraction, arm_state.angles);
    // Body surface sits exactly at the wrist center: surface_range_to_body
    // clamps to 0, well inside the fixed reach envelope.
    const std::vector<StaticSphereBody> bodies{StaticSphereBody{"rock", samples.wrist.center_m, 0.5}};

    const auto result = manipulator_reach_status(
        ManipulatorArmId::Starboard, arm_state, ProbeWorldPose{}, bodies, "rock");
    assert(result.has_value());
    assert(result->in_reach);
    assert(nearly_equal(result->wrist_range_to_surface_m, 0.0));
    assert(nearly_equal(result->remaining_distance_m, 0.0));
}

void test_wrist_outside_envelope_reports_remaining_distance() {
    const ManipulatorArmState arm_state = deployed_idle_arm_state();
    const ManipulatorArmContactSamples samples = manipulator_arm_contact_samples(
        ManipulatorArmId::Starboard, arm_state.deployment_fraction, arm_state.angles);
    const Vector3d far_center = contact_add(samples.wrist.center_m, Vector3d{10.0, 0.0, 0.0});
    const std::vector<StaticSphereBody> bodies{StaticSphereBody{"rock", far_center, 1.0}};

    const auto result = manipulator_reach_status(
        ManipulatorArmId::Starboard, arm_state, ProbeWorldPose{}, bodies, "rock");
    assert(result.has_value());
    assert(!result->in_reach);
    assert(nearly_equal(result->wrist_range_to_surface_m, 9.0));
    assert(nearly_equal(
        result->remaining_distance_m, 9.0 - ManipulatorReachEnvelopeMeters::kMaxWristRangeToSurfaceM));
}

void test_wrist_range_tracks_probe_world_pose() {
    // The same registered body must read as in reach at the origin and out
    // of reach once the probe (and therefore the arm) has moved away --
    // this must consult the probe's *current* pose on every call, not a
    // pose captured once.
    const ManipulatorArmState arm_state = deployed_idle_arm_state();
    const ManipulatorArmContactSamples samples = manipulator_arm_contact_samples(
        ManipulatorArmId::Starboard, arm_state.deployment_fraction, arm_state.angles);
    const std::vector<StaticSphereBody> bodies{StaticSphereBody{"rock", samples.wrist.center_m, 0.05}};

    const auto near_result = manipulator_reach_status(
        ManipulatorArmId::Starboard, arm_state, ProbeWorldPose{}, bodies, "rock");
    assert(near_result.has_value() && near_result->in_reach);

    const ProbeWorldPose moved_pose{Vector3d{50.0, 0.0, 0.0}, {}};
    const auto far_result = manipulator_reach_status(
        ManipulatorArmId::Starboard, arm_state, moved_pose, bodies, "rock");
    assert(far_result.has_value());
    assert(!far_result->in_reach);
}

void test_runtime_overload_matches_free_function() {
    DamageAwareProbeRuntime runtime = DamageAwareProbeRuntime::make_canonical_ev0001();

    ManipulatorRig rig;
    rig.begin_deploy(ManipulatorArmId::Port);
    rig.advance(ManipulatorRig::kDeployStowDurationS);
    const ManipulatorArmState& arm_state = rig.arm(ManipulatorArmId::Port);
    assert(arm_state.is_deployed);

    const ManipulatorArmContactSamples samples = manipulator_arm_contact_samples(
        ManipulatorArmId::Port, arm_state.deployment_fraction, arm_state.angles);
    const Vector3d world_wrist = contact_add(runtime.snapshot().position_m, samples.wrist.center_m);
    runtime.add_static_sphere_body({"rock", world_wrist, 0.05});

    // No selection yet: fails closed even though a fully-deployed arm and a
    // registered body sitting right at the wrist both exist.
    assert(!manipulator_reach_status(runtime, ManipulatorArmId::Port, arm_state).has_value());

    runtime.select_target("rock");
    const auto result = manipulator_reach_status(runtime, ManipulatorArmId::Port, arm_state);
    assert(result.has_value());
    assert(result->in_reach);
}

} // namespace

int main() {
    test_no_target_selected_returns_nullopt();
    test_arm_not_deployed_returns_nullopt();
    test_arm_mid_stow_returns_nullopt();
    test_arm_mid_deploy_returns_nullopt();
    test_selected_target_not_registered_returns_nullopt();
    test_wrist_within_envelope_reports_in_reach();
    test_wrist_outside_envelope_reports_remaining_distance();
    test_wrist_range_tracks_probe_world_pose();
    test_runtime_overload_matches_free_function();

    std::puts("manipulator_reach_tests: all tests passed");
    return 0;
}
