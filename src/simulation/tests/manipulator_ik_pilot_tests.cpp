#include "everward/simulation/manipulator_ik_pilot.hpp"

#undef NDEBUG
#include <cassert>
#include <cmath>
#include <limits>

using namespace everward::simulation;

namespace {
[[nodiscard]] bool close(double a, double b, double eps = 1e-12) {
    return std::abs(a-b) <= eps;
}
[[nodiscard]] bool same(ManipulatorArmAngles a, ManipulatorArmAngles b) {
    return close(a.shoulder_degrees,b.shoulder_degrees) &&
           close(a.elbow_degrees,b.elbow_degrees) &&
           close(a.wrist_degrees,b.wrist_degrees);
}

void test_same_pose_and_tool_tip_truth() {
    for (auto arm : {ManipulatorArmId::Port, ManipulatorArmId::Starboard}) {
        ManipulatorArmAngles start{};
        const auto tip = manipulator_tool_tip_contact_sample(arm, 1.0, start).center_m;
        const auto solution = solve_manipulator_ik_pilot(arm, tip, start);
        assert(solution.converged());
        assert(same(start, solution.suggested_angles));
        assert(close(solution.best_error_m, 0.0));
        assert(solution.iterations == 0);
    }
}

void test_reaching_with_authoritative_fk_and_mirrored_arms() {
    for (auto arm : {ManipulatorArmId::Port, ManipulatorArmId::Starboard}) {
        const ManipulatorArmAngles start{0.0, 0.0, 0.0};
        const ManipulatorArmAngles known_target_pose{15.0, 25.0, -10.0};
        const auto target = manipulator_tool_tip_contact_sample(arm, 1.0, known_target_pose).center_m;
        const auto a = solve_manipulator_ik_pilot(arm, target, start);
        const auto b = solve_manipulator_ik_pilot(arm, target, start);
        assert(a.converged());
        assert(a.best_error_m <= 0.025);
        assert(a.best_error_m < a.initial_error_m);
        assert(same(a.suggested_angles, b.suggested_angles));
        assert(a.iterations == b.iterations);
        assert(a.suggested_angles.shoulder_degrees >= ManipulatorRig::shoulder_range().min_degrees);
        assert(a.suggested_angles.shoulder_degrees <= ManipulatorRig::shoulder_range().max_degrees);
        assert(a.suggested_angles.elbow_degrees >= ManipulatorRig::elbow_range().min_degrees);
        assert(a.suggested_angles.elbow_degrees <= ManipulatorRig::elbow_range().max_degrees);
        assert(a.suggested_angles.wrist_degrees >= ManipulatorRig::wrist_range().min_degrees);
        assert(a.suggested_angles.wrist_degrees <= ManipulatorRig::wrist_range().max_degrees);
        const auto actual = manipulator_tool_tip_contact_sample(arm, 1.0, a.suggested_angles).center_m;
        const double error = std::sqrt((actual.x-target.x)*(actual.x-target.x) +
                    (actual.y-target.y)*(actual.y-target.y) +
                    (actual.z-target.z)*(actual.z-target.z));
        assert(close(error, a.best_error_m, 1e-9));
    }
}

void test_unreachable_and_bad_inputs_fail_closed() {
    const ManipulatorArmAngles start{};
    const auto unreachable = solve_manipulator_ik_pilot(ManipulatorArmId::Port, {999.0, 999.0, 999.0}, start);
    assert(unreachable.status == ManipulatorIKPilotStatus::Unreachable);
    assert(!unreachable.converged());
    assert(same(unreachable.suggested_angles, start));
    const auto invalid = solve_manipulator_ik_pilot(ManipulatorArmId::Port,
         {std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0}, start);
    assert(invalid.status == ManipulatorIKPilotStatus::InvalidInput);
    assert(same(invalid.suggested_angles, start));
    const ManipulatorArmAngles out_of_range{999.0, 0.0, 0.0};
    const auto bad_seed = solve_manipulator_ik_pilot(ManipulatorArmId::Port, {0.0,0.0,0.0}, out_of_range);
    assert(bad_seed.status == ManipulatorIKPilotStatus::InvalidInput);
}

void test_collision_guard_never_approves_forbidden_pose() {
    const ManipulatorArmAngles start{};
    const ManipulatorArmAngles other{20.0, 30.0, -20.0};
    const auto target = manipulator_tool_tip_contact_sample(ManipulatorArmId::Port, 1.0, other).center_m;
    // The guard accepts only the unchanged starting pose, so no candidate
    // may advance toward target. This is a strict fail-closed regression.
    const ManipulatorRig::SelfCollisionGuard guard = [](
        ManipulatorArmId, double deploy, ManipulatorArmAngles pose) {
            return deploy == 1.0 && same(pose, {});
        };
    const auto answer = solve_manipulator_ik_pilot(ManipulatorArmId::Port, target, start, guard);
    assert(answer.status == ManipulatorIKPilotStatus::BlockedOrUnsolved);
    assert(same(answer.suggested_angles, start));
}

void test_solver_does_not_modify_live_rig_or_rate_limits() {
    ManipulatorRig rig;
    rig.begin_deploy(ManipulatorArmId::Port);
    rig.advance(ManipulatorRig::kDeployStowDurationS);
    const auto before = rig.arm(ManipulatorArmId::Port).angles;
    const auto original_command = rig.arm(ManipulatorArmId::Port).commanded_angles;
    const ManipulatorArmAngles target_pose{15.0, 25.0, -10.0};
    const auto tip = manipulator_tool_tip_contact_sample(ManipulatorArmId::Port, 1.0, target_pose).center_m;
    (void) solve_manipulator_ik_pilot(ManipulatorArmId::Port, tip, before);
    assert(same(before, rig.arm(ManipulatorArmId::Port).angles));
    assert(same(original_command, rig.arm(ManipulatorArmId::Port).commanded_angles));
    assert(ManipulatorRig::kJointSlewDegreesPerSecond == 45.0);
}
}
int main() {
    test_same_pose_and_tool_tip_truth();
    test_reaching_with_authoritative_fk_and_mirrored_arms();
    test_unreachable_and_bad_inputs_fail_closed();
    test_collision_guard_never_approves_forbidden_pose();
    test_solver_does_not_modify_live_rig_or_rate_limits();
}
