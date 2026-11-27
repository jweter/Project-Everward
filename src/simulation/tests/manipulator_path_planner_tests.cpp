// Everward issue #378 G01/G02: ManipulatorRig refuses any step that penetrates the hull, so a legal final pose can
// be unreachable by a direct joint command (the shoulder stalls at the edge of a forbidden band). These tests prove
// that defect with the REAL hull + environment guards, then prove the planner's waypoints get the arm to the solved
// mining pose and that the real mining path (MiningSystem::mine_once) then accepts.

#include "everward/simulation/manipulator_hull_contact.hpp"
#include "everward/simulation/manipulator_path_planner.hpp"
#include "everward/simulation/mining_staging.hpp"

#undef NDEBUG
#include <cassert>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

using namespace everward::simulation;

namespace {

constexpr ManipulatorArmId kArms[] = {ManipulatorArmId::Port, ManipulatorArmId::Starboard};

bool near(double a, double b, double eps = 1e-9) { return std::fabs(a - b) < eps; }

bool same_pose(ManipulatorArmAngles a, ManipulatorArmAngles b) {
    return near(a.shoulder_degrees, b.shoulder_degrees) && near(a.elbow_degrees, b.elbow_degrees) &&
           near(a.wrist_degrees, b.wrist_degrees);
}

// Synthetic guard: the shoulder may not sit in (12, 45) degrees unless the elbow is raised to 20 or more. From
// the zero pose a direct shoulder slew is blocked; raising the elbow first opens the way.
ManipulatorRig::SelfCollisionGuard banded_guard() {
    return [](ManipulatorArmId, double, ManipulatorArmAngles a) {
        const bool in_band = a.shoulder_degrees > 12.0 && a.shoulder_degrees < 45.0;
        return !in_band || a.elbow_degrees >= 20.0;
    };
}

ManipulatorRig deployed_rig(ManipulatorArmId arm, ManipulatorRig::SelfCollisionGuard guard) {
    ManipulatorRig rig(std::move(guard));
    rig.begin_deploy(arm);
    rig.advance(ManipulatorRig::kDeployStowDurationS);
    assert(rig.arm(arm).is_deployed);
    rig.attach_tool(arm);
    return rig;
}

void direct_slew(ManipulatorRig& rig, ManipulatorArmId arm, ManipulatorArmAngles target, int frames) {
    rig.command_joint_target_degrees(arm, ManipulatorJoint::Shoulder, target.shoulder_degrees);
    rig.command_joint_target_degrees(arm, ManipulatorJoint::Elbow, target.elbow_degrees);
    rig.command_joint_target_degrees(arm, ManipulatorJoint::Wrist, target.wrist_degrees);
    for (int i = 0; i < frames; ++i) rig.advance(1.0 / 60.0);
}

// Every pose along the straight segments between consecutive waypoints, sampled finely, must be legal.
bool path_is_legal(ManipulatorArmId arm, ManipulatorArmAngles start, const std::vector<ManipulatorArmAngles>& path,
                   const ManipulatorRig::SelfCollisionGuard& guard) {
    ManipulatorArmAngles from = start;
    for (const ManipulatorArmAngles& to : path) {
        for (int s = 0; s <= 200; ++s) {
            const double t = s / 200.0;
            const ManipulatorArmAngles p{from.shoulder_degrees + t * (to.shoulder_degrees - from.shoulder_degrees),
                                         from.elbow_degrees + t * (to.elbow_degrees - from.elbow_degrees),
                                         from.wrist_degrees + t * (to.wrist_degrees - from.wrist_degrees)};
            if (!guard(arm, 1.0, p)) return false;
        }
        from = to;
    }
    return true;
}

void test_start_equal_to_goal_needs_no_waypoints() {
    const auto path = plan_collision_free_joint_path(ManipulatorArmId::Port, 1.0, {10, 20, 30}, {10, 20, 30}, banded_guard());
    assert(path.has_value() && path->empty());
}

void test_an_unguarded_path_is_direct_and_ends_exactly_at_the_goal() {
    const ManipulatorArmAngles goal{50.0, 5.0, -30.0};
    const auto path = plan_collision_free_joint_path(ManipulatorArmId::Port, 1.0, {0, 0, 0}, goal, {});
    assert(path.has_value() && !path->empty() && path->size() <= 3);   // one merged segment per joint at most
    assert(same_pose(path->back(), goal));
}

void test_a_blocked_direct_slew_is_solved_by_a_detour() {
    const auto guard = banded_guard();
    const ManipulatorArmAngles goal{50.0, 25.0, 0.0};   // legal on its own (elbow >= 20)
    assert(guard(ManipulatorArmId::Port, 1.0, goal));

    // The defect: commanded directly, the shoulder stalls at the band's edge and never arrives.
    ManipulatorRig stalled = deployed_rig(ManipulatorArmId::Port, guard);
    direct_slew(stalled, ManipulatorArmId::Port, goal, 3000);
    assert(stalled.arm(ManipulatorArmId::Port).angles.shoulder_degrees <= 12.0 + 1e-9);

    const auto path = plan_collision_free_joint_path(ManipulatorArmId::Port, 1.0, {0, 0, 0}, goal, guard);
    assert(path.has_value());
    assert(path->size() >= 2);                         // a real detour, not one straight command
    assert(same_pose(path->back(), goal));
    assert(path_is_legal(ManipulatorArmId::Port, {0, 0, 0}, *path, guard));

    ManipulatorRig rig = deployed_rig(ManipulatorArmId::Port, guard);
    assert(follow_joint_path(rig, ManipulatorArmId::Port, *path));
    assert(same_pose(rig.arm(ManipulatorArmId::Port).angles, goal));
}

void test_an_illegal_start_or_goal_or_a_sealed_wall_has_no_path() {
    const auto guard = banded_guard();
    // Goal inside the band with a low elbow: illegal.
    assert(!plan_collision_free_joint_path(ManipulatorArmId::Port, 1.0, {0, 0, 0}, {30, 0, 0}, guard).has_value());
    // Start illegal.
    assert(!plan_collision_free_joint_path(ManipulatorArmId::Port, 1.0, {30, 0, 0}, {0, 0, 0}, guard).has_value());
    // A wall no pose can cross: shoulder beyond 20 is never legal, goal at shoulder 60 is legal but sealed off.
    const ManipulatorRig::SelfCollisionGuard wall = [](ManipulatorArmId, double, ManipulatorArmAngles a) {
        return a.shoulder_degrees < 20.0 || a.shoulder_degrees > 55.0;
    };
    assert(!plan_collision_free_joint_path(ManipulatorArmId::Port, 1.0, {0, 0, 0}, {60, 0, 0}, wall).has_value());
}

void test_invalid_arguments_throw() {
    const auto guard = banded_guard();
    auto throws = [](auto&& call) {
        try {
            (void)call();
        } catch (const std::invalid_argument&) {
            return true;
        }
        return false;
    };
    const auto plan = [&](double fraction, ManipulatorArmAngles s, ManipulatorArmAngles g, JointPathOptions o = {}) {
        return plan_collision_free_joint_path(ManipulatorArmId::Port, fraction, s, g, guard, o);
    };
    assert(throws([&] { return plan(0.0, {0, 0, 0}, {1, 0, 0}); }));
    assert(throws([&] { return plan(1.5, {0, 0, 0}, {1, 0, 0}); }));
    assert(throws([&] { return plan(std::nan(""), {0, 0, 0}, {1, 0, 0}); }));
    assert(throws([&] { return plan(1.0, {std::nan(""), 0, 0}, {1, 0, 0}); }));
    assert(throws([&] { return plan(1.0, {0, 0, 0}, {1000.0, 0, 0}); }));
    assert(throws([&] { return plan(1.0, {0, 0, 0}, {1, 0, 0}, JointPathOptions{0.0, 5.0, 15.0}); }));
    assert(throws([&] { return plan(1.0, {0, 0, 0}, {1, 0, 0}, JointPathOptions{5.0, -1.0, 15.0}); }));
}

void test_a_goal_that_barely_differs_does_not_blow_up_the_search() {
    // Codex review: a tiny non-zero delta used to become the grid spacing for the whole joint range
    // (millions of guard calls). Spacing stays nominal, so the cost is bounded by the joint ranges.
    long calls = 0;
    const auto inner = banded_guard();
    const ManipulatorRig::SelfCollisionGuard counted = [&calls, inner](ManipulatorArmId a, double f, ManipulatorArmAngles p) {
        ++calls;
        return inner(a, f, p);
    };
    const ManipulatorArmAngles goal{50.0, 25.00001, 0.0};
    const auto path = plan_collision_free_joint_path(ManipulatorArmId::Port, 1.0, {0, 0, 0}, goal, counted);
    assert(path.has_value());
    assert(same_pose(path->back(), goal));
    assert(calls < 400000);
    ManipulatorRig rig = deployed_rig(ManipulatorArmId::Port, banded_guard());
    assert(follow_joint_path(rig, ManipulatorArmId::Port, *path));

    // The Codex scenario itself: goal elbow differs from the start by 1e-5 and the detour needs elbow >= 20.
    long calls2 = 0;
    const ManipulatorRig::SelfCollisionGuard counted2 = [&calls2, inner](ManipulatorArmId a, double f, ManipulatorArmAngles p) {
        ++calls2;
        return inner(a, f, p);
    };
    const auto hard = plan_collision_free_joint_path(ManipulatorArmId::Port, 1.0, {0, 0, 0}, {50.0, 0.00001, 0.0}, counted2);
    assert(!hard.has_value() || same_pose(hard->back(), {50.0, 0.00001, 0.0}));
    assert(calls2 < 400000);
}

void test_a_forbidden_sliver_between_samples_is_still_avoided() {
    // Codex review: shoulder angles in (0.3, 0.9) are illegal unless the elbow is raised. The follower's real
    // 0.75-degree frames land inside it, so a plain shoulder move must be rejected and a detour planned.
    const ManipulatorRig::SelfCollisionGuard sliver = [](ManipulatorArmId, double, ManipulatorArmAngles a) {
        return !(a.shoulder_degrees > 0.3 && a.shoulder_degrees < 0.9) || a.elbow_degrees >= 20.0;
    };
    const ManipulatorArmAngles goal{5.0, 0.0, 0.0};
    ManipulatorRig stalled = deployed_rig(ManipulatorArmId::Port, sliver);
    direct_slew(stalled, ManipulatorArmId::Port, goal, 600);
    assert(stalled.arm(ManipulatorArmId::Port).angles.shoulder_degrees < 1.0);   // the direct command stalls

    const auto path = plan_collision_free_joint_path(ManipulatorArmId::Port, 1.0, {0, 0, 0}, goal, sliver);
    assert(path.has_value());
    assert(path->size() >= 2);                                                   // not the one-segment shortcut
    ManipulatorRig rig = deployed_rig(ManipulatorArmId::Port, sliver);
    assert(follow_joint_path(rig, ManipulatorArmId::Port, *path));
    assert(same_pose(rig.arm(ManipulatorArmId::Port).angles, goal));
}

void test_the_frame_time_is_validated_and_used() {
    const auto guard = banded_guard();
    bool threw = false;
    try {
        JointPathOptions bad;
        bad.frame_seconds = 0.0;
        (void)plan_collision_free_joint_path(ManipulatorArmId::Port, 1.0, {0, 0, 0}, {1, 0, 0}, guard, bad);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    assert(threw);
    // A path planned for a slower frame rate is also followed successfully at that frame rate.
    JointPathOptions slow;
    slow.frame_seconds = 1.0 / 30.0;
    const ManipulatorArmAngles goal{50.0, 25.0, 0.0};
    const auto path = plan_collision_free_joint_path(ManipulatorArmId::Port, 1.0, {0, 0, 0}, goal, guard, slow);
    assert(path.has_value());
    ManipulatorRig rig = deployed_rig(ManipulatorArmId::Port, guard);
    assert(follow_joint_path(rig, ManipulatorArmId::Port, *path, 1.0 / 30.0));
    assert(same_pose(rig.arm(ManipulatorArmId::Port).angles, goal));
}

void test_follow_reports_a_stall_instead_of_hanging() {
    ManipulatorRig rig = deployed_rig(ManipulatorArmId::Port, banded_guard());
    // A direct (illegal-for-the-rig) waypoint into the band stalls; the follower must give up, not loop forever.
    assert(!follow_joint_path(rig, ManipulatorArmId::Port, {{50.0, 0.0, 0.0}}, 1.0 / 60.0, 5.0));
    assert(!follow_joint_path(rig, ManipulatorArmId::Port, {{1.0, 0.0, 0.0}}, 0.0));
}

// The real scenario: canonical hull + a registered deposit, the staging solver's pose, the REAL combined guard.
void test_the_real_guards_block_the_direct_command_and_the_planned_path_mines() {
    StaticSphereBody body;
    body.body_id = "bootstrap-rock";
    body.center_m = {50.0, 0.0, 0.0};
    body.radius_m = 2.0;
    const std::vector<StaticSphereBody> bodies{body};

    for (ManipulatorArmId arm : kArms) {
        const EulerAttitudeDegrees attitude{};
        const auto solution =
            mining_staging_solution(arm, body, attitude, {0.0, 0.0, 0.0}, ProbeCompoundCollisionEnvelope{}, bodies);
        assert(solution.has_value());
        const ProbeWorldPose staged{solution->probe_position_m, attitude};
        const auto guard = make_combined_collision_guard(
            make_hull_self_collision_guard(),
            make_environment_collision_guard([staged]() { return staged; },
                                             [&bodies]() -> const std::vector<StaticSphereBody>& { return bodies; }));
        assert(guard(arm, 1.0, solution->angles));   // the solved pose itself is legal

        // Defect: the straight command stalls well short of the solved shoulder angle.
        ManipulatorRig direct = deployed_rig(arm, guard);
        direct_slew(direct, arm, solution->angles, 3000);
        assert(direct.arm(arm).angles.shoulder_degrees < solution->angles.shoulder_degrees - 5.0);

        // Fix: the planned waypoints arrive exactly, and the real mining path then accepts.
        const auto path = plan_collision_free_joint_path(arm, 1.0, {0, 0, 0}, solution->angles, guard);
        assert(path.has_value() && !path->empty());
        assert(same_pose(path->back(), solution->angles));
        ManipulatorRig rig = deployed_rig(arm, guard);
        assert(follow_joint_path(rig, arm, *path));
        assert(same_pose(rig.arm(arm).angles, solution->angles));

        MiningSystem mining;
        ResourceDeposit deposit;
        deposit.body = body;
        deposit.material_id = "iron_regolith";
        deposit.display_name = "Iron-bearing regolith";
        deposit.remaining_kg = 20.0;
        deposit.extraction_kg_per_cycle = 5.0;
        mining.add_deposit(deposit);
        mining.mark_surveyed(body.body_id);
        const MiningAttemptResult result = mining.mine_once(body.body_id, rig, staged, 0.0, 500.0);
        assert(result.accepted);
        assert(result.extracted_kg > 0.0 && result.remaining_deposit_kg < 20.0);
    }
}

}  // namespace

int main() {
    test_start_equal_to_goal_needs_no_waypoints();
    test_an_unguarded_path_is_direct_and_ends_exactly_at_the_goal();
    test_a_blocked_direct_slew_is_solved_by_a_detour();
    test_an_illegal_start_or_goal_or_a_sealed_wall_has_no_path();
    test_invalid_arguments_throw();
    test_a_goal_that_barely_differs_does_not_blow_up_the_search();
    test_a_forbidden_sliver_between_samples_is_still_avoided();
    test_the_frame_time_is_validated_and_used();
    test_follow_reports_a_stall_instead_of_hanging();
    test_the_real_guards_block_the_direct_command_and_the_planned_path_mines();
    std::cout << "manipulator_path_planner_tests passed\n";
    return 0;
}
