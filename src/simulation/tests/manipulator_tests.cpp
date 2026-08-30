#include "everward/simulation/manipulator.hpp"

#undef NDEBUG
#include <cassert>
#include <cmath>
#include <cstdio>
#include <stdexcept>

namespace {

using everward::simulation::ManipulatorArmId;
using everward::simulation::ManipulatorEventType;
using everward::simulation::ManipulatorJoint;
using everward::simulation::ManipulatorRig;

bool nearly_equal(double a, double b, double epsilon = 1e-9) {
    return std::fabs(a - b) <= epsilon;
}

// Named for the common case (most guard checks in this module throw
// std::runtime_error), but catches std::exception broadly so it also covers
// the std::invalid_argument checks (e.g. non-finite/negative arguments).
template <typename Fn>
bool throws_runtime_error(Fn&& fn) {
    try {
        fn();
    } catch (const std::exception&) {
        return true;
    }
    return false;
}

void test_default_state_is_stowed_and_idle() {
    ManipulatorRig rig;
    for (auto id : {ManipulatorArmId::Port, ManipulatorArmId::Starboard}) {
        const auto& state = rig.arm(id);
        assert(!state.is_deployed);
        assert(!state.is_deploying);
        assert(!state.is_stowing);
        assert(nearly_equal(state.deployment_fraction, 0.0));
        assert(!state.tool_attached);
        assert(nearly_equal(state.angles.shoulder_degrees, 0.0));
        assert(nearly_equal(state.angles.elbow_degrees, 0.0));
        assert(nearly_equal(state.angles.wrist_degrees, 0.0));
    }
}

void test_deploy_progresses_gradually_then_completes_exactly_once() {
    ManipulatorRig rig;
    rig.begin_deploy(ManipulatorArmId::Port);
    {
        const auto events = rig.drain_events();
        assert(events.size() == 1);
        assert(events[0].arm == ManipulatorArmId::Port);
        assert(events[0].type == ManipulatorEventType::ArmDeployStarted);
    }

    rig.advance(ManipulatorRig::kDeployStowDurationS * 0.5);
    assert(!rig.is_deployed(ManipulatorArmId::Port));
    assert(nearly_equal(rig.arm(ManipulatorArmId::Port).deployment_fraction, 0.5));
    assert(rig.drain_events().empty());

    // Crossing the threshold fires ArmDeployCompleted exactly once, not on
    // every subsequent step once already deployed.
    rig.advance(ManipulatorRig::kDeployStowDurationS * 0.5);
    assert(rig.is_deployed(ManipulatorArmId::Port));
    assert(nearly_equal(rig.arm(ManipulatorArmId::Port).deployment_fraction, 1.0));
    {
        const auto events = rig.drain_events();
        assert(events.size() == 1);
        assert(events[0].type == ManipulatorEventType::ArmDeployCompleted);
    }

    rig.advance(1.0);
    assert(rig.drain_events().empty());
    assert(nearly_equal(rig.arm(ManipulatorArmId::Port).deployment_fraction, 1.0));
}

void test_begin_deploy_already_deployed_throws() {
    ManipulatorRig rig;
    rig.begin_deploy(ManipulatorArmId::Port);
    rig.advance(ManipulatorRig::kDeployStowDurationS);
    assert(rig.is_deployed(ManipulatorArmId::Port));
    assert(throws_runtime_error([&] { rig.begin_deploy(ManipulatorArmId::Port); }));
}

void test_joint_command_rejected_until_fully_deployed() {
    ManipulatorRig rig;
    assert(throws_runtime_error([&] {
        rig.command_joint_target_degrees(ManipulatorArmId::Port, ManipulatorJoint::Shoulder, 30.0);
    }));

    rig.begin_deploy(ManipulatorArmId::Port);
    rig.advance(ManipulatorRig::kDeployStowDurationS * 0.5);
    assert(throws_runtime_error([&] {
        rig.command_joint_target_degrees(ManipulatorArmId::Port, ManipulatorJoint::Shoulder, 30.0);
    }));

    rig.advance(ManipulatorRig::kDeployStowDurationS * 0.5);
    assert(rig.is_deployed(ManipulatorArmId::Port));
    rig.command_joint_target_degrees(ManipulatorArmId::Port, ManipulatorJoint::Shoulder, 30.0);
    assert(nearly_equal(rig.arm(ManipulatorArmId::Port).commanded_angles.shoulder_degrees, 30.0));
}

void test_joint_motion_is_rate_limited_toward_commanded_target() {
    ManipulatorRig rig;
    rig.begin_deploy(ManipulatorArmId::Port);
    rig.advance(ManipulatorRig::kDeployStowDurationS);
    (void)rig.drain_events();

    rig.command_joint_target_degrees(ManipulatorArmId::Port, ManipulatorJoint::Elbow, 90.0);
    assert(nearly_equal(rig.arm(ManipulatorArmId::Port).angles.elbow_degrees, 0.0));

    rig.advance(1.0);
    const double expected_after_one_second = ManipulatorRig::kJointSlewDegreesPerSecond * 1.0;
    assert(nearly_equal(rig.arm(ManipulatorArmId::Port).angles.elbow_degrees, expected_after_one_second));
    assert(expected_after_one_second < 90.0);

    // Enough additional time reaches, but does not overshoot, the target.
    rig.advance(10.0);
    assert(nearly_equal(rig.arm(ManipulatorArmId::Port).angles.elbow_degrees, 90.0));
}

void test_joint_target_out_of_range_is_clamped_not_rejected() {
    ManipulatorRig rig;
    rig.begin_deploy(ManipulatorArmId::Port);
    rig.advance(ManipulatorRig::kDeployStowDurationS);

    rig.command_joint_target_degrees(ManipulatorArmId::Port, ManipulatorJoint::Shoulder, 500.0);
    assert(nearly_equal(
        rig.arm(ManipulatorArmId::Port).commanded_angles.shoulder_degrees,
        ManipulatorRig::shoulder_range().max_degrees));

    rig.command_joint_target_degrees(ManipulatorArmId::Port, ManipulatorJoint::Elbow, -45.0);
    assert(nearly_equal(
        rig.arm(ManipulatorArmId::Port).commanded_angles.elbow_degrees,
        ManipulatorRig::elbow_range().min_degrees));
}

void test_tool_attach_requires_deployed_and_is_idempotent_guarded() {
    ManipulatorRig rig;
    assert(throws_runtime_error([&] { rig.attach_tool(ManipulatorArmId::Port); }));

    rig.begin_deploy(ManipulatorArmId::Port);
    rig.advance(ManipulatorRig::kDeployStowDurationS);
    (void)rig.drain_events();

    rig.attach_tool(ManipulatorArmId::Port);
    assert(rig.arm(ManipulatorArmId::Port).tool_attached);
    {
        const auto events = rig.drain_events();
        assert(events.size() == 1);
        assert(events[0].type == ManipulatorEventType::ToolAttached);
    }

    assert(throws_runtime_error([&] { rig.attach_tool(ManipulatorArmId::Port); }));

    rig.detach_tool(ManipulatorArmId::Port);
    assert(!rig.arm(ManipulatorArmId::Port).tool_attached);
    assert(throws_runtime_error([&] { rig.detach_tool(ManipulatorArmId::Port); }));
}

void test_stowing_with_tool_attached_is_rejected() {
    ManipulatorRig rig;
    rig.begin_deploy(ManipulatorArmId::Port);
    rig.advance(ManipulatorRig::kDeployStowDurationS);
    rig.attach_tool(ManipulatorArmId::Port);

    assert(throws_runtime_error([&] { rig.begin_stow(ManipulatorArmId::Port); }));

    rig.detach_tool(ManipulatorArmId::Port);
    rig.begin_stow(ManipulatorArmId::Port);
    assert(rig.arm(ManipulatorArmId::Port).is_stowing);
}

void test_stow_retracts_joints_and_completes_exactly_once() {
    ManipulatorRig rig;
    rig.begin_deploy(ManipulatorArmId::Port);
    rig.advance(ManipulatorRig::kDeployStowDurationS);
    rig.command_joint_target_degrees(ManipulatorArmId::Port, ManipulatorJoint::Wrist, 90.0);
    rig.advance(10.0);
    assert(nearly_equal(rig.arm(ManipulatorArmId::Port).angles.wrist_degrees, 90.0));
    (void)rig.drain_events();

    rig.begin_stow(ManipulatorArmId::Port);
    {
        const auto events = rig.drain_events();
        assert(events.size() == 1);
        assert(events[0].type == ManipulatorEventType::ArmStowStarted);
    }
    // Stow commands the joint back toward zero immediately, then rate-limited
    // motion and deployment-fraction retraction both progress under advance().
    assert(nearly_equal(rig.arm(ManipulatorArmId::Port).commanded_angles.wrist_degrees, 0.0));

    rig.advance(ManipulatorRig::kDeployStowDurationS + 10.0);
    assert(!rig.is_deployed(ManipulatorArmId::Port));
    assert(nearly_equal(rig.arm(ManipulatorArmId::Port).deployment_fraction, 0.0));
    assert(nearly_equal(rig.arm(ManipulatorArmId::Port).angles.wrist_degrees, 0.0));
    {
        const auto events = rig.drain_events();
        assert(events.size() == 1);
        assert(events[0].type == ManipulatorEventType::ArmStowCompleted);
    }
}

void test_begin_stow_already_stowed_throws() {
    ManipulatorRig rig;
    assert(throws_runtime_error([&] { rig.begin_stow(ManipulatorArmId::Port); }));
}

void test_reversing_mid_transition_converges_to_new_direction() {
    ManipulatorRig rig;
    rig.begin_deploy(ManipulatorArmId::Port);
    rig.advance(ManipulatorRig::kDeployStowDurationS * 0.25);
    assert(rig.arm(ManipulatorArmId::Port).is_deploying);

    // Reverse before completion: no tool is attached, so this is legal.
    rig.begin_stow(ManipulatorArmId::Port);
    assert(rig.arm(ManipulatorArmId::Port).is_stowing);
    assert(!rig.arm(ManipulatorArmId::Port).is_deploying);

    rig.advance(ManipulatorRig::kDeployStowDurationS);
    assert(!rig.is_deployed(ManipulatorArmId::Port));
    assert(nearly_equal(rig.arm(ManipulatorArmId::Port).deployment_fraction, 0.0));
}

void test_arms_are_independent() {
    ManipulatorRig rig;
    rig.begin_deploy(ManipulatorArmId::Port);
    rig.advance(ManipulatorRig::kDeployStowDurationS);
    (void)rig.drain_events();

    assert(rig.is_deployed(ManipulatorArmId::Port));
    assert(!rig.is_deployed(ManipulatorArmId::Starboard));
    assert(throws_runtime_error([&] {
        rig.command_joint_target_degrees(ManipulatorArmId::Starboard, ManipulatorJoint::Shoulder, 10.0);
    }));

    rig.command_joint_target_degrees(ManipulatorArmId::Port, ManipulatorJoint::Shoulder, 45.0);
    rig.advance(10.0);
    assert(nearly_equal(rig.arm(ManipulatorArmId::Port).angles.shoulder_degrees, 45.0));
    assert(nearly_equal(rig.arm(ManipulatorArmId::Starboard).angles.shoulder_degrees, 0.0));
}

void test_advance_rejects_negative_seconds() {
    ManipulatorRig rig;
    assert(throws_runtime_error([&] { rig.advance(-1.0); }));
}

} // namespace

int main() {
    test_default_state_is_stowed_and_idle();
    test_deploy_progresses_gradually_then_completes_exactly_once();
    test_begin_deploy_already_deployed_throws();
    test_joint_command_rejected_until_fully_deployed();
    test_joint_motion_is_rate_limited_toward_commanded_target();
    test_joint_target_out_of_range_is_clamped_not_rejected();
    test_tool_attach_requires_deployed_and_is_idempotent_guarded();
    test_stowing_with_tool_attached_is_rejected();
    test_stow_retracts_joints_and_completes_exactly_once();
    test_begin_stow_already_stowed_throws();
    test_reversing_mid_transition_converges_to_new_direction();
    test_arms_are_independent();
    test_advance_rejects_negative_seconds();

    std::puts("manipulator_tests: all tests passed");
    return 0;
}
