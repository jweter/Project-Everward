#include "everward/simulation/manipulator_hull_contact.hpp"

#include <cassert>
#include <cstdio>

namespace {

using everward::simulation::ManipulatorArmAngles;
using everward::simulation::ManipulatorArmId;
using everward::simulation::ManipulatorJoint;
using everward::simulation::ManipulatorRig;
using everward::simulation::ProbeCompoundCollisionEnvelope;
using everward::simulation::make_hull_self_collision_guard;
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

    std::puts("manipulator_hull_contact_tests: all tests passed");
    return 0;
}
