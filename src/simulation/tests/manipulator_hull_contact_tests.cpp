#include "everward/simulation/manipulator_hull_contact.hpp"

#include <cassert>
#include <cstdio>
#include <vector>

namespace {

using everward::simulation::EulerAttitudeDegrees;
using everward::simulation::ManipulatorArmAngles;
using everward::simulation::ManipulatorArmContactSamples;
using everward::simulation::ManipulatorArmId;
using everward::simulation::ManipulatorJoint;
using everward::simulation::ManipulatorRig;
using everward::simulation::ProbeCompoundCollisionEnvelope;
using everward::simulation::ProbeWorldPose;
using everward::simulation::StaticSphereBody;
using everward::simulation::Vector3d;
using everward::simulation::make_combined_collision_guard;
using everward::simulation::make_environment_collision_guard;
using everward::simulation::contact_add;
using everward::simulation::make_hull_self_collision_guard;
using everward::simulation::manipulator_arm_contact_samples;
using everward::simulation::manipulator_pose_intersects_environment;
using everward::simulation::manipulator_pose_intersects_hull;

void test_idle_deployed_rest_pose_does_not_intersect_hull() {
    // Fully deployed, all joints neutral: this is the pose the arm sits in
    // immediately after deploy completes and whenever it is not being
    // actively commanded. It must never be flagged as self-colliding, or
    // the guard would deadlock deploy itself.
    for (auto id : {ManipulatorArmId::Port, ManipulatorArmId::Starboard}) {
        const bool intersects =
            manipulator_pose_intersects_hull(id, 1.0, ManipulatorArmAngles{}, ProbeCompoundCollisionEnvelope{});
        assert(!intersects);
    }
}

void test_extreme_folded_shoulder_intersects_hull() {
    // Folding the shoulder all the way to its -90 degree limit swings the
    // forearm back in toward the central hull instead of away from it. This
    // is the concrete case PROJECT_STATUS.md's Slice 6 note describes: with
    // no collision at all, nothing stops this pose from being reached.
    for (auto id : {ManipulatorArmId::Port, ManipulatorArmId::Starboard}) {
        ManipulatorArmAngles folded_in{};
        folded_in.shoulder_degrees = ManipulatorRig::shoulder_range().min_degrees;
        const bool intersects =
            manipulator_pose_intersects_hull(id, 1.0, folded_in, ProbeCompoundCollisionEnvelope{});
        assert(intersects);
    }
}

void test_default_rig_has_no_guard_and_reaches_commanded_extreme() {
    // A default-constructed rig (as every existing manipulator_tests.cpp
    // test uses) must behave exactly as it did before this guard existed.
    ManipulatorRig rig;
    rig.begin_deploy(ManipulatorArmId::Port);
    rig.advance(ManipulatorRig::kDeployStowDurationS);
    rig.command_joint_target_degrees(
        ManipulatorArmId::Port, ManipulatorJoint::Shoulder, ManipulatorRig::shoulder_range().min_degrees);
    rig.advance(10.0);
    assert(rig.arm(ManipulatorArmId::Port).angles.shoulder_degrees == ManipulatorRig::shoulder_range().min_degrees);
}

void test_guard_that_always_rejects_freezes_joint_motion() {
    ManipulatorRig rig([](ManipulatorArmId, double, ManipulatorArmAngles) { return false; });
    rig.begin_deploy(ManipulatorArmId::Port);
    rig.advance(ManipulatorRig::kDeployStowDurationS);
    assert(rig.is_deployed(ManipulatorArmId::Port));

    rig.command_joint_target_degrees(ManipulatorArmId::Port, ManipulatorJoint::Elbow, 90.0);
    rig.advance(10.0);
    assert(rig.arm(ManipulatorArmId::Port).angles.elbow_degrees == 0.0);
}

void test_guard_that_always_accepts_matches_unguarded_behavior() {
    ManipulatorRig rig([](ManipulatorArmId, double, ManipulatorArmAngles) { return true; });
    rig.begin_deploy(ManipulatorArmId::Port);
    rig.advance(ManipulatorRig::kDeployStowDurationS);
    rig.command_joint_target_degrees(ManipulatorArmId::Port, ManipulatorJoint::Elbow, 90.0);
    rig.advance(10.0);
    assert(rig.arm(ManipulatorArmId::Port).angles.elbow_degrees == 90.0);
}

void test_hull_guard_deploy_and_stow_still_complete() {
    // The guard only applies to steady-state joint commands (see
    // manipulator.hpp's advance_arm), so installing the real hull guard
    // must not affect deploy/stow at all.
    ManipulatorRig rig(make_hull_self_collision_guard());
    rig.begin_deploy(ManipulatorArmId::Port);
    rig.advance(ManipulatorRig::kDeployStowDurationS);
    assert(rig.is_deployed(ManipulatorArmId::Port));

    rig.begin_stow(ManipulatorArmId::Port);
    rig.advance(ManipulatorRig::kDeployStowDurationS);
    assert(!rig.is_deployed(ManipulatorArmId::Port));
    assert(rig.arm(ManipulatorArmId::Port).deployment_fraction == 0.0);
}

void test_hull_guard_permits_ordinary_commanded_motion() {
    // A moderate shoulder swing plus a full elbow bend stays well clear of
    // the hull (see the hand-verified rest/extension cases this guard's
    // geometry was checked against) and must reach its commanded target
    // exactly, or the guard would be an unusable false-positive trap.
    ManipulatorRig rig(make_hull_self_collision_guard());
    rig.begin_deploy(ManipulatorArmId::Starboard);
    rig.advance(ManipulatorRig::kDeployStowDurationS);

    rig.command_joint_target_degrees(ManipulatorArmId::Starboard, ManipulatorJoint::Shoulder, 45.0);
    rig.command_joint_target_degrees(ManipulatorArmId::Starboard, ManipulatorJoint::Elbow, 150.0);
    rig.advance(10.0);

    const auto& state = rig.arm(ManipulatorArmId::Starboard);
    assert(state.angles.shoulder_degrees == 45.0);
    assert(state.angles.elbow_degrees == 150.0);
}

void test_hull_guard_prevents_reaching_extreme_folded_shoulder() {
    // The wiring proof: commanding the same -90 degree fold that
    // test_extreme_folded_shoulder_intersects_hull shows overlaps the hull
    // must not actually be reached once the real hull guard is installed --
    // this is what makes "collision does not allow impossible penetration
    // through the probe body" true end to end, not just present as inert
    // geometry math alongside the rig.
    ManipulatorRig rig(make_hull_self_collision_guard());
    rig.begin_deploy(ManipulatorArmId::Port);
    rig.advance(ManipulatorRig::kDeployStowDurationS);

    rig.command_joint_target_degrees(
        ManipulatorArmId::Port, ManipulatorJoint::Shoulder, ManipulatorRig::shoulder_range().min_degrees);
    rig.advance(10.0);

    const auto& state = rig.arm(ManipulatorArmId::Port);
    assert(state.angles.shoulder_degrees > ManipulatorRig::shoulder_range().min_degrees);
    assert(!manipulator_pose_intersects_hull(
        ManipulatorArmId::Port, state.deployment_fraction, state.angles, ProbeCompoundCollisionEnvelope{}));
}

void test_environment_intersection_is_false_with_no_registered_bodies() {
    const std::vector<StaticSphereBody> no_bodies;
    const bool intersects = manipulator_pose_intersects_environment(
        ManipulatorArmId::Starboard, 1.0, ManipulatorArmAngles{}, ProbeWorldPose{}, no_bodies);
    assert(!intersects);
}

void test_environment_intersection_detects_body_colocated_with_wrist() {
    // Probe sits at the world origin with identity attitude, so an arm
    // sample's world position equals its probe-local position exactly.
    const ManipulatorArmContactSamples samples =
        manipulator_arm_contact_samples(ManipulatorArmId::Starboard, 1.0, ManipulatorArmAngles{});
    const std::vector<StaticSphereBody> colocated_body{
        StaticSphereBody{"test-rock", samples.wrist.center_m, 0.05}};

    assert(manipulator_pose_intersects_environment(
        ManipulatorArmId::Starboard, 1.0, ManipulatorArmAngles{}, ProbeWorldPose{}, colocated_body));
}

void test_environment_intersection_ignores_distant_body() {
    const std::vector<StaticSphereBody> distant_body{
        StaticSphereBody{"test-rock", Vector3d{500.0, 500.0, 500.0}, 1.0}};

    assert(!manipulator_pose_intersects_environment(
        ManipulatorArmId::Starboard, 1.0, ManipulatorArmAngles{}, ProbeWorldPose{}, distant_body));
}

void test_environment_intersection_moves_with_probe_world_pose() {
    // The same registered body that misses the arm at the origin must be
    // detected once the probe (and therefore the arm) has moved to sit on
    // top of it -- the guard must consult the probe's *current* pose, not a
    // pose fixed at construction time.
    const ManipulatorArmContactSamples samples =
        manipulator_arm_contact_samples(ManipulatorArmId::Starboard, 1.0, ManipulatorArmAngles{});
    const Vector3d body_center = contact_add(samples.wrist.center_m, Vector3d{50.0, 0.0, 0.0});
    const std::vector<StaticSphereBody> body{StaticSphereBody{"test-rock", body_center, 0.05}};

    assert(!manipulator_pose_intersects_environment(
        ManipulatorArmId::Starboard, 1.0, ManipulatorArmAngles{}, ProbeWorldPose{}, body));

    const ProbeWorldPose moved_pose{Vector3d{50.0, 0.0, 0.0}, EulerAttitudeDegrees{}};
    assert(manipulator_pose_intersects_environment(
        ManipulatorArmId::Starboard, 1.0, ManipulatorArmAngles{}, moved_pose, body));
}

void test_environment_guard_prevents_reaching_pose_that_intersects_registered_body() {
    // Mirrors test_hull_guard_prevents_reaching_extreme_folded_shoulder, but
    // for a registered external body instead of the probe's own hull: place
    // a body exactly where the extreme-folded pose's elbow would land, and
    // confirm a rig guarded only by the environment check stops short of
    // that commanded target the same way the hull guard does.
    const ManipulatorArmAngles folded_target{ManipulatorRig::shoulder_range().min_degrees, 0.0, 0.0};
    const ManipulatorArmContactSamples folded_samples =
        manipulator_arm_contact_samples(ManipulatorArmId::Port, 1.0, folded_target);
    const std::vector<StaticSphereBody> blocking_body{
        StaticSphereBody{"test-rock", folded_samples.elbow.center_m, 0.05}};

    ManipulatorRig rig(make_environment_collision_guard(
        []() { return ProbeWorldPose{}; },
        [&blocking_body]() -> const std::vector<StaticSphereBody>& { return blocking_body; }));
    rig.begin_deploy(ManipulatorArmId::Port);
    rig.advance(ManipulatorRig::kDeployStowDurationS);

    rig.command_joint_target_degrees(
        ManipulatorArmId::Port, ManipulatorJoint::Shoulder, ManipulatorRig::shoulder_range().min_degrees);
    rig.advance(10.0);

    const auto& state = rig.arm(ManipulatorArmId::Port);
    assert(state.angles.shoulder_degrees > ManipulatorRig::shoulder_range().min_degrees);
    assert(!manipulator_pose_intersects_environment(
        ManipulatorArmId::Port, state.deployment_fraction, state.angles, ProbeWorldPose{}, blocking_body));
}

void test_environment_guard_deploy_and_stow_still_complete() {
    const std::vector<StaticSphereBody> no_bodies;
    ManipulatorRig rig(make_environment_collision_guard(
        []() { return ProbeWorldPose{}; },
        [&no_bodies]() -> const std::vector<StaticSphereBody>& { return no_bodies; }));
    rig.begin_deploy(ManipulatorArmId::Starboard);
    rig.advance(ManipulatorRig::kDeployStowDurationS);
    assert(rig.is_deployed(ManipulatorArmId::Starboard));

    rig.begin_stow(ManipulatorArmId::Starboard);
    rig.advance(ManipulatorRig::kDeployStowDurationS);
    assert(!rig.is_deployed(ManipulatorArmId::Starboard));
}

void test_combined_guard_rejects_if_either_check_rejects() {
    auto always_true = [](ManipulatorArmId, double, ManipulatorArmAngles) { return true; };
    auto always_false = [](ManipulatorArmId, double, ManipulatorArmAngles) { return false; };

    ManipulatorRig rejecting_rig(make_combined_collision_guard(always_true, always_false));
    rejecting_rig.begin_deploy(ManipulatorArmId::Port);
    rejecting_rig.advance(ManipulatorRig::kDeployStowDurationS);
    rejecting_rig.command_joint_target_degrees(ManipulatorArmId::Port, ManipulatorJoint::Elbow, 90.0);
    rejecting_rig.advance(10.0);
    assert(rejecting_rig.arm(ManipulatorArmId::Port).angles.elbow_degrees == 0.0);

    ManipulatorRig permitting_rig(make_combined_collision_guard(always_true, always_true));
    permitting_rig.begin_deploy(ManipulatorArmId::Port);
    permitting_rig.advance(ManipulatorRig::kDeployStowDurationS);
    permitting_rig.command_joint_target_degrees(ManipulatorArmId::Port, ManipulatorJoint::Elbow, 90.0);
    permitting_rig.advance(10.0);
    assert(permitting_rig.arm(ManipulatorArmId::Port).angles.elbow_degrees == 90.0);
}

void test_combined_guard_with_both_empty_matches_unguarded_behavior() {
    ManipulatorRig rig(make_combined_collision_guard(ManipulatorRig::SelfCollisionGuard{}, ManipulatorRig::SelfCollisionGuard{}));
    rig.begin_deploy(ManipulatorArmId::Port);
    rig.advance(ManipulatorRig::kDeployStowDurationS);
    rig.command_joint_target_degrees(ManipulatorArmId::Port, ManipulatorJoint::Elbow, 90.0);
    rig.advance(10.0);
    assert(rig.arm(ManipulatorArmId::Port).angles.elbow_degrees == 90.0);
}

} // namespace

int main() {
    test_idle_deployed_rest_pose_does_not_intersect_hull();
    test_extreme_folded_shoulder_intersects_hull();
    test_default_rig_has_no_guard_and_reaches_commanded_extreme();
    test_guard_that_always_rejects_freezes_joint_motion();
    test_guard_that_always_accepts_matches_unguarded_behavior();
    test_hull_guard_deploy_and_stow_still_complete();
    test_hull_guard_permits_ordinary_commanded_motion();
    test_hull_guard_prevents_reaching_extreme_folded_shoulder();
    test_environment_intersection_is_false_with_no_registered_bodies();
    test_environment_intersection_detects_body_colocated_with_wrist();
    test_environment_intersection_ignores_distant_body();
    test_environment_intersection_moves_with_probe_world_pose();
    test_environment_guard_prevents_reaching_pose_that_intersects_registered_body();
    test_environment_guard_deploy_and_stow_still_complete();
    test_combined_guard_rejects_if_either_check_rejects();
    test_combined_guard_with_both_empty_matches_unguarded_behavior();

    std::puts("manipulator_hull_contact_tests: all tests passed");
    return 0;
}
