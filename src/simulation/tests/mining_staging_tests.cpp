// Everward issue #372 P0-1 (Aura run 20261108-084846): the P auto-approach staged the probe where no arm input could
// reach the deposit. These tests prove that root cause from the arm's own kinematics, then prove the replacement
// staging solver yields a pose from which the REAL mining path (MiningSystem::mine_once, the same oracle G uses)
// extracts material, for both arms at several attitudes, without the compound hull touching any body.

#include "everward/simulation/mining_staging.hpp"

#undef NDEBUG
#include <cassert>
#include <cmath>
#include <string>
#include <vector>

using namespace everward::simulation;

namespace {

StaticSphereBody deposit_body(Vector3d centre = {50.0, 0.0, 0.0}, double radius = 2.0) {
    StaticSphereBody body;
    body.body_id = "bootstrap-deposit";
    body.center_m = centre;
    body.radius_m = radius;
    return body;
}

// Best tool-tip surface gap any joint pose in the arm's full range can produce from a fixed probe pose (no hull filter:
// a pose that would hit the hull is generous to the old staging, so this is a lower bound on what a player could do).
double best_reachable_gap(ManipulatorArmId arm, const StaticSphereBody& body, ProbeWorldPose pose) {
    double best = 1.0e30;
    for (double s = -90.0; s <= 90.0; s += 5.0) {
        for (double e = 0.0; e <= 150.0; e += 5.0) {
            for (double w = -180.0; w <= 180.0; w += 15.0) {
                const ManipulatorArmSample tip = manipulator_tool_tip_contact_sample(arm, 1.0, {s, e, w});
                const Vector3d world = contact_add(pose.position_m, rotate_local_contact_offset(tip.center_m, pose.attitude_degrees));
                const Vector3d d = contact_subtract(world, body.center_m);
                best = std::min(best, std::sqrt(contact_dot(d, d)) - (body.radius_m + tip.radius_m));
            }
        }
    }
    return best;
}

void test_the_old_side_staging_point_was_out_of_reach_for_every_joint_pose() {
    const StaticSphereBody body = deposit_body();
    for (ManipulatorArmId arm : {ManipulatorArmId::Port, ManipulatorArmId::Starboard}) {
        for (double side : {1.0, -1.0}) {   // both side conventions the old P code could produce
            const ProbeWorldPose old_staging{{50.0, side * (2.0 + 6.5), 0.0}, {}};
            const double gap = best_reachable_gap(arm, body, old_staging);
            assert(gap > MiningSystem::kGeneration1ToolWorkingReachM);   // no input could have mined (Aura saw ~6 m)
        }
    }
}

ManipulatorRig rig_aligned_to(ManipulatorArmId arm, ManipulatorArmAngles angles) {
    ManipulatorRig rig;
    rig.begin_deploy(arm);
    rig.advance(ManipulatorRig::kDeployStowDurationS);
    assert(rig.arm(arm).is_deployed);
    rig.attach_tool(arm);
    rig.command_joint_target_degrees(arm, ManipulatorJoint::Shoulder, angles.shoulder_degrees);
    rig.command_joint_target_degrees(arm, ManipulatorJoint::Elbow, angles.elbow_degrees);
    rig.command_joint_target_degrees(arm, ManipulatorJoint::Wrist, angles.wrist_degrees);
    for (int i = 0; i < 120; ++i) rig.advance(0.5);   // the real, rate-limited joint slew
    const ManipulatorArmAngles reached = rig.arm(arm).angles;
    assert(std::fabs(reached.shoulder_degrees - angles.shoulder_degrees) < 1e-6);
    assert(std::fabs(reached.elbow_degrees - angles.elbow_degrees) < 1e-6);
    assert(std::fabs(reached.wrist_degrees - angles.wrist_degrees) < 1e-6);
    return rig;
}

bool hull_overlaps(Vector3d position, EulerAttitudeDegrees attitude, const StaticSphereBody& body) {
    return sweep_compound_probe_against_body(position, position, attitude, ProbeCompoundCollisionEnvelope{}, body).hit;
}

void test_the_solved_staging_pose_mines_through_the_real_mining_path() {
    const std::vector<EulerAttitudeDegrees> attitudes{{0.0, 0.0, 0.0}, {90.0, 0.0, 0.0}, {0.0, 15.0, 0.0}, {0.0, 0.0, 30.0},
                                                      {-35.0, 10.0, -20.0}};
    for (ManipulatorArmId arm : {ManipulatorArmId::Port, ManipulatorArmId::Starboard}) {
        for (const EulerAttitudeDegrees& attitude : attitudes) {
            const StaticSphereBody body = deposit_body();
            const std::vector<StaticSphereBody> bodies{body, deposit_body({-80.0, 40.0, 0.0}, 3.0)};
            const auto solution = mining_staging_solution(arm, body, attitude, {0.0, 0.0, 0.0}, ProbeCompoundCollisionEnvelope{}, bodies);
            assert(solution.has_value());
            assert(solution->tool_surface_gap_m > 0.25 && solution->tool_surface_gap_m < 0.35);
            for (const StaticSphereBody& other : bodies) assert(!hull_overlaps(solution->probe_position_m, attitude, other));

            ManipulatorRig rig = rig_aligned_to(arm, solution->angles);
            MiningSystem mining;
            ResourceDeposit deposit;
            deposit.body = body;
            deposit.material_id = "iron_regolith";
            deposit.display_name = "Iron-bearing regolith";
            deposit.remaining_kg = 20.0;
            deposit.extraction_kg_per_cycle = 5.0;
            mining.add_deposit(deposit);
            mining.mark_surveyed(body.body_id);

            const MiningAttemptResult result =
                mining.mine_once(body.body_id, rig, ProbeWorldPose{solution->probe_position_m, attitude}, 0.0, 500.0);
            assert(result.accepted);                                         // the same oracle G uses
            assert(result.extracted_kg > 0.0 && result.material_id == "iron_regolith");
            assert(result.remaining_deposit_kg < 20.0);
            assert(result.tool_surface_gap_m <= MiningSystem::kGeneration1ToolWorkingReachM);
        }
    }
}

void test_a_body_in_the_way_is_never_ignored() {
    const StaticSphereBody body = deposit_body();
    const auto first = mining_staging_solution(ManipulatorArmId::Port, body, {}, {0.0, 0.0, 0.0}, ProbeCompoundCollisionEnvelope{}, {body});
    assert(first.has_value());
    StaticSphereBody blocker;
    blocker.body_id = "blocker";
    blocker.center_m = first->probe_position_m;   // a rock exactly where the probe would sit
    blocker.radius_m = 1.0;
    const auto second = mining_staging_solution(
        ManipulatorArmId::Port, body, {}, {0.0, 0.0, 0.0}, ProbeCompoundCollisionEnvelope{}, {body, blocker});
    if (second.has_value()) {
        assert(!hull_overlaps(second->probe_position_m, {}, blocker));    // a different, clear pose
        assert(!hull_overlaps(second->probe_position_m, {}, body));
    }
    StaticSphereBody invalid = body;
    invalid.radius_m = 0.0;
    assert(!mining_staging_solution(ManipulatorArmId::Port, invalid, {}, {}, ProbeCompoundCollisionEnvelope{}, {invalid}).has_value());
}

} // namespace

int main() {
    test_the_old_side_staging_point_was_out_of_reach_for_every_joint_pose();
    test_the_solved_staging_pose_mines_through_the_real_mining_path();
    test_a_body_in_the_way_is_never_ignored();
    return 0;
}
