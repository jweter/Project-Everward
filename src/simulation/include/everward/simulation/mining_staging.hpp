#pragma once

#include "everward/simulation/compound_contact.hpp"
#include "everward/simulation/manipulator.hpp"
#include "everward/simulation/manipulator_hull_contact.hpp"
#include "everward/simulation/manipulator_tool_contact.hpp"
#include "everward/simulation/mining.hpp"
#include "everward/simulation/types.hpp"

#include <cmath>
#include <optional>
#include <vector>

namespace everward::simulation {

// Mining staging from the arm's real kinematics (Aura run 20261108-084846, Everward issue #372 P0-1).
//
// The old auto-approach (P) staged the probe at target centre + (0, +/-(radius + 6.5 m), 0) on the WORLD Y axis. Every
// arm joint pitches about the probe's local Y axis with only a fixed 24 degree outward roll, so a side-mounted arm
// reaches only about 2 m sideways while that point puts the deposit surface about 6.5 m to the side: no joint input
// can bring the tool into mining reach from there (proved by mining_staging_tests.cpp). This solver searches the arm's
// actual joint ranges instead and places the probe so the tool tip ends a small gap off the deposit surface, at the
// probe's current attitude, rejecting any pose whose arm crosses the probe's own hull, whose arm samples pass through
// the target, or whose compound hull envelope touches any registered body. It authors no mining credit: MiningSystem
// remains the only judge of reach (tool-tip gap <= kGeneration1ToolWorkingReachM).

struct MiningStagingSolution {
    Vector3d probe_position_m{};
    ManipulatorArmAngles angles{};
    // The tool-tip surface gap this pose produces, computed exactly as MiningSystem::mine_once does.
    double tool_surface_gap_m{0.0};
};

struct MiningStagingSearch {
    double target_gap_m{0.30};          // well inside the 0.55 m working reach, outside the -0.02 m penetration limit
    double hull_clearance_m{0.50};      // extra margin between every compound hull sample and every body
    double shoulder_step_degrees{5.0};
    double elbow_step_degrees{5.0};
    double wrist_step_degrees{15.0};
};

namespace mining_staging_detail {

[[nodiscard]] inline double length(Vector3d v) noexcept { return std::sqrt(contact_dot(v, v)); }

[[nodiscard]] inline bool hull_touches_any_body(
    Vector3d probe_position_m,
    EulerAttitudeDegrees attitude,
    const ProbeCompoundCollisionEnvelope& hull,
    const std::vector<StaticSphereBody>& bodies,
    double clearance_m) noexcept {
    for (const StaticSphereBody& body : bodies) {
        for (const ProbeCollisionSphereSample& sample : hull.samples) {
            const Vector3d centre = contact_add(probe_position_m, rotate_local_contact_offset(sample.local_center_m, attitude));
            if (length(contact_subtract(centre, body.center_m)) < body.radius_m + sample.radius_m + clearance_m) {
                return true;
            }
        }
    }
    return false;
}

} // namespace mining_staging_detail

// Returns the reachable staging pose closest to ``from_position_m`` (deterministic: ties keep the first pose in grid
// order), or nullopt when the arm cannot reach the target from any collision-free position at this attitude.
[[nodiscard]] inline std::optional<MiningStagingSolution> mining_staging_solution(
    ManipulatorArmId arm,
    const StaticSphereBody& target,
    EulerAttitudeDegrees attitude,
    Vector3d from_position_m,
    const ProbeCompoundCollisionEnvelope& hull,
    const std::vector<StaticSphereBody>& bodies,
    MiningStagingSearch search = {}) noexcept {
    using mining_staging_detail::length;
    if (!(target.radius_m > 0.0) || !std::isfinite(target.radius_m)) return std::nullopt;

    std::optional<MiningStagingSolution> best;
    double best_travel = 0.0;
    const auto shoulder = ManipulatorRig::shoulder_range();
    const auto elbow = ManipulatorRig::elbow_range();
    const auto wrist = ManipulatorRig::wrist_range();

    for (double s = shoulder.min_degrees; s <= shoulder.max_degrees + 1e-9; s += search.shoulder_step_degrees) {
        for (double e = elbow.min_degrees; e <= elbow.max_degrees + 1e-9; e += search.elbow_step_degrees) {
            for (double w = wrist.min_degrees; w <= wrist.max_degrees + 1e-9; w += search.wrist_step_degrees) {
                const ManipulatorArmAngles angles{s, e, w};
                if (manipulator_pose_intersects_hull(arm, 1.0, angles, hull)) continue;

                const ManipulatorArmSample tip = manipulator_tool_tip_contact_sample(arm, 1.0, angles);
                const ManipulatorArmContactSamples links = manipulator_arm_contact_samples(arm, 1.0, angles);
                const Vector3d tool_axis_local = contact_subtract(tip.center_m, links.wrist.center_m);
                const double axis_length = length(tool_axis_local);
                if (!(axis_length > 1e-9)) continue;

                // The tool points into the deposit: the tip sits on the line from the deposit centre against the tool
                // axis, target_gap_m off the surface (gap measured exactly as MiningSystem does).
                const Vector3d axis_world = rotate_local_contact_offset(
                    contact_scale(tool_axis_local, 1.0 / axis_length), attitude);
                const double centre_to_tip = target.radius_m + tip.radius_m + search.target_gap_m;
                const Vector3d tip_world = contact_subtract(target.center_m, contact_scale(axis_world, centre_to_tip));
                const Vector3d probe_position = contact_subtract(tip_world, rotate_local_contact_offset(tip.center_m, attitude));

                // The arm links must stay outside the deposit (only the tool tip approaches it).
                bool link_inside = false;
                for (const ManipulatorArmSample* link : {&links.elbow, &links.wrist}) {
                    const Vector3d world = contact_add(probe_position, rotate_local_contact_offset(link->center_m, attitude));
                    if (length(contact_subtract(world, target.center_m)) < target.radius_m + link->radius_m) link_inside = true;
                }
                if (link_inside) continue;
                if (mining_staging_detail::hull_touches_any_body(probe_position, attitude, hull, bodies, search.hull_clearance_m)) {
                    continue;
                }

                const double travel = length(contact_subtract(probe_position, from_position_m));
                if (!best.has_value() || travel < best_travel) {
                    const Vector3d check = contact_add(probe_position, rotate_local_contact_offset(tip.center_m, attitude));
                    const double gap = length(contact_subtract(check, target.center_m)) - (target.radius_m + tip.radius_m);
                    best = MiningStagingSolution{probe_position, angles, gap};
                    best_travel = travel;
                }
            }
        }
    }
    return best;
}

} // namespace everward::simulation
