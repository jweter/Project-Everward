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
//  * The search grid keeps the NOMINAL step per joint, anchored at the start pose, so its size is bounded by
//    the joint ranges (never by how close the goal happens to be). The exact goal is joined to the nearest
//    grid nodes by short single-joint chains.
//  * Every move changes exactly one joint, so each segment is precisely the motion the rig performs when only
//    that joint has a new target.
//  * A segment is legal only if (a) every pose the rig actually visits while slewing it at
//    `frame_seconds` per frame is legal (the rig rejects whole frames, so these are the poses that matter), and
//    (b) poses sampled every kSampleDegrees along it are legal (so a different frame rate is still safe for
//    forbidden slices wider than that).
//  * Breadth-first search gives the fewest segments; runs of the same joint in the same direction are merged
//    into one waypoint when the merged segment is itself legal, otherwise they stay separate.

#include "everward/simulation/manipulator.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <optional>
#include <queue>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace everward::simulation {

inline constexpr double kDefaultJointPathFrameSeconds = 1.0 / 60.0;

struct JointPathOptions {
    double shoulder_step_degrees{5.0};
    double elbow_step_degrees{5.0};
    double wrist_step_degrees{15.0};
    // The frame time the follower will use; segments are validated at exactly the poses this produces.
    double frame_seconds{kDefaultJointPathFrameSeconds};
};

namespace path_planner_detail {

inline constexpr double kSampleDegrees = 0.5;
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

inline ManipulatorArmAngles with_joint(ManipulatorArmAngles a, int joint, double value) {
    (joint == 0 ? a.shoulder_degrees : (joint == 1 ? a.elbow_degrees : a.wrist_degrees)) = value;
    return a;
}

}  // namespace path_planner_detail

// Returns the waypoints (excluding `start`, ending exactly at `goal`) that move the arm from `start` to
// `goal` with every checked pose legal under `guard`; an empty vector when start == goal; std::nullopt when
// `start` or `goal` is itself illegal or no legal path exists on the grid. Throws std::invalid_argument for
// non-finite or out-of-range angles, a deployment fraction outside (0, 1], or a non-positive step/frame time.
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
    const std::array<double, 3> step{
        options.shoulder_step_degrees, options.elbow_step_degrees, options.wrist_step_degrees};
    for (double s : step) {
        if (!std::isfinite(s) || s <= 0.0) {
            throw std::invalid_argument("joint path planner: step must be positive and finite");
        }
    }
    if (!std::isfinite(options.frame_seconds) || options.frame_seconds <= 0.0) {
        throw std::invalid_argument("joint path planner: frame time must be positive and finite");
    }
    const char* names[3] = {"shoulder", "elbow", "wrist"};
    for (int j = 0; j < 3; ++j) {
        require_finite_in_range(joint_value(start, j), ranges[static_cast<std::size_t>(j)], names[j]);
        require_finite_in_range(joint_value(goal, j), ranges[static_cast<std::size_t>(j)], names[j]);
    }

    const auto legal = [&](const ManipulatorArmAngles& pose) { return !guard || guard(arm, deployment_fraction, pose); };
    if (!legal(start) || !legal(goal)) return std::nullopt;

    // Moving only `joint` from `from` to `target`: the exact poses the rig visits plus dense samples.
    const double max_frame_step = ManipulatorRig::kJointSlewDegreesPerSecond * options.frame_seconds;
    const auto segment_legal = [&](const ManipulatorArmAngles& from, int joint, double target) {
        const double origin = joint_value(from, joint);
        if (origin == target) return true;
        double value = origin;
        for (;;) {  // the rig's own slew, frame by frame
            const double delta = target - value;
            value = delta > max_frame_step ? value + max_frame_step
                                           : (delta < -max_frame_step ? value - max_frame_step : target);
            if (!legal(with_joint(from, joint, value))) return false;
            if (value == target) break;
        }
        const int samples = std::max(1, static_cast<int>(std::ceil(std::abs(target - origin) / kSampleDegrees)));
        for (int s = 1; s < samples; ++s) {
            const double t = static_cast<double>(s) / samples;
            if (!legal(with_joint(from, joint, origin + t * (target - origin)))) return false;
        }
        return true;
    };

    const std::array<double, 3> delta{
        goal.shoulder_degrees - start.shoulder_degrees,
        goal.elbow_degrees - start.elbow_degrees,
        goal.wrist_degrees - start.wrist_degrees};
    if (delta[0] == 0.0 && delta[1] == 0.0 && delta[2] == 0.0) return std::vector<ManipulatorArmAngles>{};

    const auto pose_at = [&](const Index& i) {
        return ManipulatorArmAngles{
            start.shoulder_degrees + i[0] * step[0],
            start.elbow_degrees + i[1] * step[1],
            start.wrist_degrees + i[2] * step[2]};
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
    // A node can reach the goal directly when the goal lies within one step of it on every joint.
    const auto near_goal = [&](const Index& i) {
        for (std::size_t j = 0; j < 3; ++j) {
            if (std::abs(i[j] * step[j] - delta[j]) > step[j] + 1e-9) return false;
        }
        return true;
    };
    // Join node -> exact goal with up to three single-joint segments, trying every joint order.
    const auto connect_to_goal = [&](const ManipulatorArmAngles& from) -> std::optional<std::vector<ManipulatorArmAngles>> {
        static constexpr int orders[6][3] = {{0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}};
        for (const auto& order : orders) {
            ManipulatorArmAngles at = from;
            std::vector<ManipulatorArmAngles> chain;
            bool ok = true;
            for (int joint : order) {
                const double target = joint_value(goal, joint);
                if (joint_value(at, joint) == target) continue;
                if (!segment_legal(at, joint, target)) { ok = false; break; }
                at = with_joint(at, joint, target);
                chain.push_back(at);
            }
            if (ok) return chain;
        }
        return std::nullopt;
    };

    // Unobstructed (or only mildly obstructed) moves need no search: at most one waypoint per joint.
    if (auto direct = connect_to_goal(start)) return std::move(*direct);

    std::unordered_map<Index, Index, IndexHash> came_from;
    std::queue<Index> frontier;
    const Index origin{0, 0, 0};
    came_from.emplace(origin, origin);
    frontier.push(origin);
    std::optional<Index> final_node;
    std::vector<ManipulatorArmAngles> tail;
    while (!frontier.empty()) {
        const Index current = frontier.front();
        frontier.pop();
        if (near_goal(current)) {
            if (auto chain = connect_to_goal(pose_at(current))) {
                final_node = current;
                tail = std::move(*chain);
                break;
            }
        }
        for (int joint = 0; joint < 3; ++joint) {
            for (int direction : {-1, 1}) {
                Index next = current;
                next[static_cast<std::size_t>(joint)] += direction;
                if (came_from.find(next) != came_from.end() || !in_range(next)) continue;
                if (!segment_legal(pose_at(current), joint, joint_value(pose_at(next), joint))) continue;
                came_from.emplace(next, current);
                frontier.push(next);
            }
        }
    }
    if (!final_node.has_value()) return std::nullopt;

    std::vector<Index> nodes;
    for (Index at = *final_node; at != origin; at = came_from.at(at)) nodes.push_back(at);
    std::reverse(nodes.begin(), nodes.end());

    // Merge runs of the same joint/direction into one waypoint when the merged segment is itself legal.
    std::vector<ManipulatorArmAngles> waypoints;
    ManipulatorArmAngles run_origin = start;
    std::size_t i = 0;
    Index previous = origin;
    while (i < nodes.size()) {
        int joint = 0;
        int direction = 0;
        for (std::size_t j = 0; j < 3; ++j) {
            const int d = nodes[i][j] - previous[j];
            if (d != 0) { joint = static_cast<int>(j); direction = d; }
        }
        std::size_t end = i + 1;
        while (end < nodes.size()) {
            int next_joint = -1;
            int next_direction = 0;
            for (std::size_t j = 0; j < 3; ++j) {
                const int d = nodes[end][j] - nodes[end - 1][j];
                if (d != 0) { next_joint = static_cast<int>(j); next_direction = d; }
            }
            if (next_joint != joint || next_direction != direction) break;
            ++end;
        }
        const ManipulatorArmAngles merged_end = pose_at(nodes[end - 1]);
        if (end - i > 1 && segment_legal(run_origin, joint, joint_value(merged_end, joint))) {
            waypoints.push_back(merged_end);
            run_origin = merged_end;
        } else {
            for (std::size_t k = i; k < end; ++k) {
                waypoints.push_back(pose_at(nodes[k]));
                run_origin = waypoints.back();
            }
        }
        previous = nodes[end - 1];
        i = end;
    }
    for (const ManipulatorArmAngles& pose : tail) waypoints.push_back(pose);
    if (waypoints.empty()) waypoints.push_back(goal);
    waypoints.back() = goal;  // exact, not accumulated floating-point
    return waypoints;
}

// Drives `rig` along `path` the way the live game does: command all three joints to a waypoint, advance in
// `dt_seconds` frames until every joint is within tolerance, then take the next waypoint. Returns false if a
// waypoint is not reached within `waypoint_timeout_seconds` (a stall) or an argument is invalid. Use the same
// frame time the path was planned with (JointPathOptions::frame_seconds).
[[nodiscard]] inline bool follow_joint_path(
    ManipulatorRig& rig,
    ManipulatorArmId arm,
    const std::vector<ManipulatorArmAngles>& path,
    double dt_seconds = kDefaultJointPathFrameSeconds,
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
