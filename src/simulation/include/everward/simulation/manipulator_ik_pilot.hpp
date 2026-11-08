#pragma once

// Experimental, opt-in ONLY: proposed arm poses are never applied to ManipulatorRig.
// No Unreal / Unity / OpenIK dependency. The authoritative tool-tip FK, joint
// limits and caller-provided collision guard remain the source of truth.
#include "everward/simulation/manipulator_tool_contact.hpp"

#include <algorithm>
#include <cmath>

namespace everward::simulation {

enum class ManipulatorIKPilotStatus {
    Converged,
    Unreachable,
    BlockedOrUnsolved,
    InvalidInput
};

struct ManipulatorIKPilotResult {
    ManipulatorIKPilotStatus status{ManipulatorIKPilotStatus::InvalidInput};
    ManipulatorArmAngles suggested_angles{};
    double initial_error_m{};
    double best_error_m{};
    int iterations{};
    [[nodiscard]] bool converged() const noexcept {
        return status == ManipulatorIKPilotStatus::Converged;
    }
};

namespace manipulator_ik_pilot_detail {

[[nodiscard]] inline Vector3d subtract(Vector3d a, Vector3d b) noexcept {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}
[[nodiscard]] inline double dot(Vector3d a, Vector3d b) noexcept {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
[[nodiscard]] inline Vector3d cross(Vector3d a, Vector3d b) noexcept {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x};
}
[[nodiscard]] inline Vector3d project(Vector3d v, Vector3d axis) noexcept {
    const double d = dot(v, axis);
    return {v.x - axis.x*d, v.y - axis.y*d, v.z - axis.z*d};
}
[[nodiscard]] inline double norm(Vector3d a) noexcept {
    return std::sqrt(dot(a, a));
}
[[nodiscard]] inline bool finite(Vector3d v) noexcept {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}
[[nodiscard]] inline bool valid(ManipulatorArmAngles a) noexcept {
    return std::isfinite(a.shoulder_degrees) && std::isfinite(a.elbow_degrees) &&
           std::isfinite(a.wrist_degrees) &&
           a.shoulder_degrees >= ManipulatorRig::shoulder_range().min_degrees &&
           a.shoulder_degrees <= ManipulatorRig::shoulder_range().max_degrees &&
           a.elbow_degrees >= ManipulatorRig::elbow_range().min_degrees &&
           a.elbow_degrees <= ManipulatorRig::elbow_range().max_degrees &&
           a.wrist_degrees >= ManipulatorRig::wrist_range().min_degrees &&
           a.wrist_degrees <= ManipulatorRig::wrist_range().max_degrees;
}
[[nodiscard]] inline double error(ManipulatorArmId arm, ManipulatorArmAngles a, Vector3d target) noexcept {
    return norm(subtract(manipulator_tool_tip_contact_sample(arm, 1.0, a).center_m, target));
}
[[nodiscard]] inline Vector3d axis_for_joint(ManipulatorArmId arm, ManipulatorArmAngles a, int joint) noexcept {
    using namespace manipulator_hull_contact_detail;
    const double side = arm == ManipulatorArmId::Port ? -1.0 : 1.0;
    // Shoulder rotates around probe-local Y; elbow and wrist around the
    // Y axis inherited from the upstream folding shoulder assembly.
    if (joint == 0) return {0.0, 1.0, 0.0};
    const auto shoulder = multiply(
        rotation_y_degrees(ManipulatorArmLayoutMeters::kFoldPitchDeployedDegrees + a.shoulder_degrees),
        rotation_x_degrees(side * ManipulatorArmLayoutMeters::kRollDeployedDegrees));
    if (joint == 1) return apply(shoulder, {0.0, 1.0, 0.0});
    const auto elbow = multiply(shoulder, rotation_y_degrees(a.elbow_degrees));
    return apply(elbow, {0.0, 1.0, 0.0});
}
[[nodiscard]] inline Vector3d pivot_for_joint(ManipulatorArmId arm, ManipulatorArmAngles a, int joint) noexcept {
    if (joint == 0) return ManipulatorArmLayoutMeters::shoulder_pivot_local_m(arm);
    const auto joints = manipulator_arm_contact_samples(arm, 1.0, a);
    return joint == 1 ? joints.elbow.center_m : joints.wrist.center_m;
}
[[nodiscard]] inline double angle(ManipulatorArmAngles a, int joint) noexcept {
    return joint == 0 ? a.shoulder_degrees : joint == 1 ? a.elbow_degrees : a.wrist_degrees;
}
inline void set_angle(ManipulatorArmAngles& a, int joint, double value) noexcept {
    if (joint == 0) a.shoulder_degrees = value;
    else if (joint == 1) a.elbow_degrees = value;
    else a.wrist_degrees = value;
}
[[nodiscard]] inline ManipulatorJointRangeDegrees range(int joint) noexcept {
    return joint == 0 ? ManipulatorRig::shoulder_range()
         : joint == 1 ? ManipulatorRig::elbow_range()
                      : ManipulatorRig::wrist_range();
}
}  // namespace manipulator_ik_pilot_detail

// Joint-by-joint constrained CCD position solving in probe-local metres.
// A proposed pose is returned ONLY on convergence. Otherwise the input pose
// is returned unchanged; caller must never treat an unsolved pose as a command.
// The optional guard should be the same authoritative combined hull/environment
// guard as the live ManipulatorRig uses. This checks *candidate poses*, not
// swept motion; integrating this into live controls requires a separate gate.
[[nodiscard]] inline ManipulatorIKPilotResult solve_manipulator_ik_pilot(
    ManipulatorArmId arm,
    Vector3d target_local_m,
    ManipulatorArmAngles starting_pose,
    ManipulatorRig::SelfCollisionGuard collision_guard = {}) {
    using namespace manipulator_ik_pilot_detail;
    ManipulatorIKPilotResult result{};
    result.suggested_angles = starting_pose;
    if (!finite(target_local_m) || !valid(starting_pose)) {
        return result;
    }
    result.initial_error_m = error(arm, starting_pose, target_local_m);
    result.best_error_m = result.initial_error_m;
    if (collision_guard && !collision_guard(arm, 1.0, starting_pose)) {
        result.status = ManipulatorIKPilotStatus::BlockedOrUnsolved;
        return result;
    }

    constexpr double tolerance_m = 0.025;
    constexpr double pi = 3.14159265358979323846;
    constexpr int max_iterations = 64;
    const auto shoulder = ManipulatorArmLayoutMeters::shoulder_pivot_local_m(arm);
    constexpr double max_reach_m = ManipulatorArmLayoutMeters::kElbowOffsetFromShoulderM +
        ManipulatorArmLayoutMeters::kWristOffsetFromElbowM +
        ManipulatorToolLayoutMeters::kTipOffsetFromWristM;
    if (norm(subtract(target_local_m, shoulder)) > max_reach_m + tolerance_m) {
        result.status = ManipulatorIKPilotStatus::Unreachable;
        return result;
    }
    if (result.initial_error_m <= tolerance_m) {
        result.status = ManipulatorIKPilotStatus::Converged;
        return result;
    }

    auto current = starting_pose;
    double best = result.initial_error_m;
    for (int pass = 0; pass < max_iterations; ++pass) {
        for (int joint = 2; joint >= 0; --joint) {
            const auto pivot = pivot_for_joint(arm, current, joint);
            const auto axis = axis_for_joint(arm, current, joint);
            const auto tip = manipulator_tool_tip_contact_sample(arm, 1.0, current).center_m;
            const auto toward_tip = project(subtract(tip, pivot), axis);
            const auto toward_target = project(subtract(target_local_m, pivot), axis);
            if (norm(toward_tip) < 1e-10 || norm(toward_target) < 1e-10) continue;
            const double delta_degrees = (180.0 / pi) * std::atan2(
                dot(axis, cross(toward_tip, toward_target)),
                dot(toward_tip, toward_target));
            const auto limits = range(joint);
            // Bounded correction and monotonic improvement avoid jumps at a
            // singularity. No random restarts or frame-time-dependent loops.
            for (double fraction : {0.8, 0.4, 0.2}) {
                auto trial = current;
                set_angle(trial, joint, std::clamp(
                    angle(current, joint) + delta_degrees*fraction,
                    limits.min_degrees, limits.max_degrees));
                if (collision_guard && !collision_guard(arm, 1.0, trial)) continue;
                const double trial_error = error(arm, trial, target_local_m);
                if (trial_error + 1e-12 < best) {
                    current = trial;
                    best = trial_error;
                    break;
                }
            }
        }
        result.iterations = pass + 1;
        result.best_error_m = best;
        if (best <= tolerance_m) {
            result.status = ManipulatorIKPilotStatus::Converged;
            result.suggested_angles = current;
            return result;
        }
    }
    result.status = ManipulatorIKPilotStatus::BlockedOrUnsolved;
    return result;
}

} // namespace everward::simulation
