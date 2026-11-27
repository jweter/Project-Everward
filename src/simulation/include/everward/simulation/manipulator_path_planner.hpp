#pragma once

// Everward issue #378 G01/G02: collision-free joint-space path planning for the manipulator rig.
//
// ManipulatorRig slews every joint straight toward its commanded target and refuses a whole step if the
// resulting pose penetrates the hull or an environment body (manipulator.hpp, advance_arm). A pose that is
// legal on its own can therefore be unreachable by a direct command: from the stowed pose the shoulder
// stalls at the edge of a forbidden band and never gets past it, even though a detour (move the elbow or
// wrist first, then the shoulder) exists. mining_staging_solution() returns the legal final pose; this
// header supplies the missing piece, the waypoints that get the arm there under the same guard.
//
// The planner is pure: it only asks the guard "is this pose legal?" and never mutates a rig.
//  * The search grid is anchored so BOTH the start and the goal pose are nodes (each joint's step is the
//    nominal step stretched so the start-to-goal distance is a whole number of steps).
//  * Moves change exactly one joint, so each segment is precisely the motion the rig performs when only
//    that joint has a new target. Legality is sampled at most every kSampleDegrees along a segment.
//  * Breadth-first search gives the fewest segments; consecutive moves of the same joint in the same
//    direction are merged into one waypoint.

#include "everward/simulation/manipulator.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <optional>
#include <queue>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace everward::simulation {

struct JointPathOptions {
    double shoulder_step_degrees{5.0};
    double elbow_step_degrees{5.0};
    double wrist_step_degrees{15.0};
};

namespace path_planner_detail {

inline constexpr double kSampleDegrees = 1.0;
inline constexpr double kReachedToleranceDegrees = 1e-6;

using Index = std::array<int, 3>;

struct IndexHash {
    std::size_t operator()(const Index& i) const noexcept {
        return (static_cast<std::size_t>(i[0] + 4096) * 8191u + static_cast<std::size_t>(i[1] + 4096)) * 8191u +
               static_cast<std::size_t>(i[2] + 4096);
    }
};

inline void require_finite_in_range(double value, ManipulatorJointRangeDegrees range, const char* what) {
    if (!std::isfinite(value) || value < range.min_degrees || value > range.max_degrees) {
        throw std::invalid_argument(std::string("joint path planner: ") + what + " out of range");
    }
}

inline double joint_value(const ManipulatorArmAngles& a, int joint) {
    return joint == 0 ? a.shoulder_degrees : (joint == 1 ? a.elbow_degrees : a.wrist_degrees);
}

}  // namespace path_planner_detail

// Returns the waypoints (excluding `start`, ending exactly at `goal`) that move the arm from `start` to
// `goal` with every sampled pose legal under `guard`; an empty vector when start == goal; std::nullopt when
// `start` or `goal` is itself illegal or no legal path exists on the grid. Throws std::invalid_argument for
// non-finite or out-of-range angles, a deployment fraction outside (0, 1], or a non-positive step.
[[nodiscard]] inline std::optional<std::vector<ManipulatorArmAngles>> plan_collision_free_joint_path(
    ManipulatorArmId arm,
    double deployment_fraction,
    ManipulatorArmAngles start,
    ManipulatorArmAngles goal,
    const ManipulatorRig::SelfCollisionGuard& guard,
    JointPathOptions options = {}) {
    using namespace path_planner_detail;
    if (!std::isfinite(deployment_fraction) || deployment_fraction <= 0.0 || deployment_fraction > 1.0) {
        throw std::invalid_argument("joint path planner: deployment fraction must be in (0, 1]");
    }
    const std::array<ManipulatorJointRangeDegrees, 3> ranges{
        ManipulatorRig::shoulder_range(), ManipulatorRig::elbow_range(), ManipulatorRig::wrist_range()};
    const std::array<double, 3> nominal{
        options.shoulder_step_degrees, options.elbow_step_degrees, options.wrist_step_degrees};
    for (double step : nominal) {
        if (!std::isfinite(step) || step <= 0.0) {
            throw std::invalid_argument("joint path planner: step must be positive and finite");
        }
    }
    const char* names[3] = {"shoulder", "elbow", "wrist"};
    for (int j = 0; j < 3; ++j) {
        require_finite_in_range(joint_value(start, j), ranges[static_cast<std::size_t>(j)], names[j]);
        require_finite_in_range(joint_value(goal, j), ranges[static_cast<std::size_t>(j)], names[j]);
    }

    const auto legal = [&](const ManipulatorArmAngles& pose) { return !guard || guard(arm, deployment_fraction, pose); };
    if (!legal(start) || !legal(goal)) return std::nullopt;

    // Per-joint step: nominal, stretched so start->goal is a whole number of steps (goal is then a node).
    std::array<double, 3> step{};
    Index goal_index{};
    for (int j = 0; j < 3; ++j) {
        const double distance = joint_value(goal, j) - joint_value(start, j);
        const double count = std::round(std::abs(distance) / nominal[static_cast<std::size_t>(j)]);
        if (count < 1.0) {
            step[static_cast<std::size_t>(j)] = std::abs(distance) > 0.0 ? std::abs(distance) : nominal[static_cast<std::size_t>(j)];
            goal_index[static_cast<std::size_t>(j)] = std::abs(distance) > 0.0 ? (distance > 0 ? 1 : -1) : 0;
        } else {
            step[static_cast<std::size_t>(j)] = std::abs(distance) / count;
            goal_index[static_cast<std::size_t>(j)] = static_cast<int>(distance > 0 ? count : -count);
        }
    }
    if (goal_index == Index{0, 0, 0}) return std::vector<ManipulatorArmAngles>{};

    const auto pose_at = [&](const Index& i) {
        return ManipulatorArmAngles{
            joint_value(start, 0) + i[0] * step[0],
            joint_value(start, 1) + i[1] * step[1],
            joint_value(start, 2) + i[2] * step[2]};
    };
    const auto in_range = [&](const Index& i) {
        const ManipulatorArmAngles p = pose_at(i);
        for (int j = 0; j < 3; ++j) {
            const double v = joint_value(p, j);
            const auto& r = ranges[static_cast<std::size_t>(j)];
            if (v < r.min_degrees - 1e-9 || v > r.max_degrees + 1e-9) return false;
        }
        return true;
    };
    // A segment between adjacent nodes: both ends and every sample at most kSampleDegrees apart are legal.
    const auto edge_legal = [&](const Index& from, int joint, int direction) {
        Index to = from;
        to[static_cast<std::size_t>(joint)] += direction;
        if (!in_range(to) || !legal(pose_at(to))) return false;
        const ManipulatorArmAngles a = pose_at(from);
        const ManipulatorArmAngles b = pose_at(to);
        const int samples = std::max(1, static_cast<int>(std::ceil(step[static_cast<std::size_t>(joint)] / kSampleDegrees)));
        for (int s = 1; s < samples; ++s) {
            const double t = static_cast<double>(s) / samples;
            ManipulatorArmAngles m = a;
            double* field = joint == 0 ? &m.shoulder_degrees : (joint == 1 ? &m.elbow_degrees : &m.wrist_degrees);
            *field = joint_value(a, joint) + t * (joint_value(b, joint) - joint_value(a, joint));
            if (!legal(m)) return false;
        }
        return true;
    };

    std::unordered_map<Index, Index, IndexHash> came_from;
    std::queue<Index> frontier;
    const Index origin{0, 0, 0};
    came_from.emplace(origin, origin);
    frontier.push(origin);
    while (!frontier.empty() && came_from.find(goal_index) == came_from.end()) {
        const Index current = frontier.front();
        frontier.pop();
        for (int joint = 0; joint < 3; ++joint) {
            for (int direction : {-1, 1}) {
                Index next = current;
                next[static_cast<std::size_t>(joint)] += direction;
                if (came_from.find(next) != came_from.end()) continue;
                if (!edge_legal(current, joint, direction)) continue;
                came_from.emplace(next, current);
                frontier.push(next);
            }
        }
    }
    if (came_from.find(goal_index) == came_from.end()) return std::nullopt;

    std::vector<Index> nodes;
    for (Index at = goal_index; at != origin; at = came_from.at(at)) nodes.push_back(at);
    std::reverse(nodes.begin(), nodes.end());

    // Merge consecutive moves of the same joint in the same direction into one waypoint.
    std::vector<ManipulatorArmAngles> waypoints;
    Index previous = origin;
    int run_joint = -1;
    int run_direction = 0;
    for (const Index& node : nodes) {
        int joint = 0;
        int direction = 0;
        for (int j = 0; j < 3; ++j) {
            const int d = node[static_cast<std::size_t>(j)] - previous[static_cast<std::size_t>(j)];
            if (d != 0) { joint = j; direction = d; }
        }
        if (joint == run_joint && direction == run_direction && !waypoints.empty()) {
            waypoints.back() = pose_at(node);
        } else {
            waypoints.push_back(pose_at(node));
        }
        run_joint = joint;
        run_direction = direction;
        previous = node;
    }
    waypoints.back() = goal;  // exact, not accumulated floating-point
    return waypoints;
}

// Drives `rig` along `path` the way the live game does: command all three joints to a waypoint, advance in
// `dt_seconds` frames until every joint is within tolerance, then take the next waypoint. Returns false if a
// waypoint is not reached within `waypoint_timeout_seconds` (a stall) or an argument is invalid.
[[nodiscard]] inline bool follow_joint_path(
    ManipulatorRig& rig,
    ManipulatorArmId arm,
    const std::vector<ManipulatorArmAngles>& path,
    double dt_seconds = 1.0 / 60.0,
    double waypoint_timeout_seconds = 30.0) {
    using namespace path_planner_detail;
    if (!std::isfinite(dt_seconds) || dt_seconds <= 0.0 || !std::isfinite(waypoint_timeout_seconds) ||
        waypoint_timeout_seconds <= 0.0) {
        return false;
    }
    for (const ManipulatorArmAngles& waypoint : path) {
        rig.command_joint_target_degrees(arm, ManipulatorJoint::Shoulder, waypoint.shoulder_degrees);
        rig.command_joint_target_degrees(arm, ManipulatorJoint::Elbow, waypoint.elbow_degrees);
        rig.command_joint_target_degrees(arm, ManipulatorJoint::Wrist, waypoint.wrist_degrees);
        double elapsed = 0.0;
        for (;;) {
            const ManipulatorArmAngles now = rig.arm(arm).angles;
            if (std::abs(now.shoulder_degrees - waypoint.shoulder_degrees) <= kReachedToleranceDegrees &&
                std::abs(now.elbow_degrees - waypoint.elbow_degrees) <= kReachedToleranceDegrees &&
                std::abs(now.wrist_degrees - waypoint.wrist_degrees) <= kReachedToleranceDegrees) {
                break;
            }
            if (elapsed >= waypoint_timeout_seconds) return false;
            rig.advance(dt_seconds);
            elapsed += dt_seconds;
        }
    }
    return true;
}

}  // namespace everward::simulation
