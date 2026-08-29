#pragma once

#include "everward/simulation/compound_contact.hpp"
#include "everward/simulation/manipulator.hpp"
#include "everward/simulation/types.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

namespace everward::simulation {

// Slice 6 follow-up (PHASE2_VERTICAL_SLICE_PLAN.md): EverwardProbePawn.cpp's
// arm meshes are still built with ECollisionEnabled::NoCollision, so nothing
// today stops a commanded joint pose from visually driving an arm through
// the probe's own hull. manipulator.hpp's header comment already establishes
// that Unreal only ever renders whatever ManipulatorRig produces and never
// authors motion itself, so the correct place to guarantee "collision does
// not allow impossible penetration through the probe body" is here: make a
// self-intersecting pose unreachable in the authoritative simulation, the
// same way command_joint_target_degrees already makes an out-of-range pose
// unreachable, rather than trying to react to it after Unreal has already
// rendered it.
//
// This deliberately reuses ProbeCompoundCollisionEnvelope -- the same
// five-sphere hull approximation software_policy.hpp already sweeps against
// external bodies -- as the arm's collision partner, and reuses
// rotate_local_contact_offset's roll(X)/pitch(Y)/yaw(Z) rotation convention
// (matching Unreal's FRotator composition order) for forward kinematics,
// rather than inventing a second hull shape or a second rotation convention.

// Fixed Generation-1 mechanical layout, converted from
// EverwardProbePawn.cpp's centimeter constants (UE units) to the meters
// ProbeCompoundCollisionEnvelope already uses (1 UE unit = 1 cm = 0.01 m).
// Keep this in sync with EverwardProbePawn.cpp's ConfigureArm lambda and
// UpdateManipulatorVisuals if the Unreal rig geometry changes.
struct ManipulatorArmLayoutMeters {
    static constexpr double kElbowOffsetFromShoulderM = 1.80;
    static constexpr double kWristOffsetFromElbowM = 1.50;
    static constexpr double kFoldPitchStowedDegrees = -102.0;
    static constexpr double kFoldPitchDeployedDegrees = -38.0;
    static constexpr double kRollStowedDegrees = 8.0;
    static constexpr double kRollDeployedDegrees = 24.0;
    // Coarse enclosing radius for the upper-arm/forearm cross-section
    // (EverwardProbePawn.cpp's ~18 cm square beams), matching the hull
    // envelope's own single-sphere-per-region coarseness.
    static constexpr double kArmSampleRadiusM = 0.12;

    [[nodiscard]] static Vector3d shoulder_pivot_local_m(ManipulatorArmId id) noexcept {
        const double side = id == ManipulatorArmId::Port ? -1.0 : 1.0;
        return {1.20, side * 1.05, -0.92};
    }
};

namespace manipulator_hull_contact_detail {

struct Matrix3d {
    std::array<std::array<double, 3>, 3> rows{};
};

[[nodiscard]] inline Matrix3d rotation_x_degrees(double degrees) noexcept {
    constexpr double kPi = 3.14159265358979323846;
    const double t = degrees * kPi / 180.0;
    const double c = std::cos(t);
    const double s = std::sin(t);
    return Matrix3d{{{
        {1.0, 0.0, 0.0},
        {0.0, c, -s},
        {0.0, s, c},
    }}};
}

[[nodiscard]] inline Matrix3d rotation_y_degrees(double degrees) noexcept {
    constexpr double kPi = 3.14159265358979323846;
    const double t = degrees * kPi / 180.0;
    const double c = std::cos(t);
    const double s = std::sin(t);
    return Matrix3d{{{
        {c, 0.0, s},
        {0.0, 1.0, 0.0},
        {-s, 0.0, c},
    }}};
}

[[nodiscard]] inline Matrix3d multiply(const Matrix3d& a, const Matrix3d& b) noexcept {
    Matrix3d out{};
    for (std::size_t row = 0; row < 3; ++row) {
        for (std::size_t col = 0; col < 3; ++col) {
            double sum = 0.0;
            for (std::size_t k = 0; k < 3; ++k) {
                sum += a.rows[row][k] * b.rows[k][col];
            }
            out.rows[row][col] = sum;
        }
    }
    return out;
}

[[nodiscard]] inline Vector3d apply(const Matrix3d& m, Vector3d v) noexcept {
    return {
        m.rows[0][0] * v.x + m.rows[0][1] * v.y + m.rows[0][2] * v.z,
        m.rows[1][0] * v.x + m.rows[1][1] * v.y + m.rows[1][2] * v.z,
        m.rows[2][0] * v.x + m.rows[2][1] * v.y + m.rows[2][2] * v.z,
    };
}

} // namespace manipulator_hull_contact_detail

struct ManipulatorArmSample {
    Vector3d center_m{};
    double radius_m{ManipulatorArmLayoutMeters::kArmSampleRadiusM};
};

struct ManipulatorArmContactSamples {
    ManipulatorArmSample elbow;
    ManipulatorArmSample wrist;
};

// Forward kinematics for the elbow and wrist pivot positions, in probe-local
// meters. Matches EverwardProbePawn.cpp's UpdateManipulatorVisuals exactly:
// shoulder pitch is (fold pitch derived from deployment_fraction) plus
// shoulder_degrees, shoulder roll is Side * (fold roll derived from
// deployment_fraction), and elbow/wrist each add a further pure-pitch
// rotation expressed in their parent joint's own frame.
[[nodiscard]] inline ManipulatorArmContactSamples manipulator_arm_contact_samples(
    ManipulatorArmId id,
    double deployment_fraction,
    ManipulatorArmAngles angles) noexcept {
    using manipulator_hull_contact_detail::multiply;
    using manipulator_hull_contact_detail::rotation_x_degrees;
    using manipulator_hull_contact_detail::rotation_y_degrees;
    using manipulator_hull_contact_detail::apply;

    const double side = id == ManipulatorArmId::Port ? -1.0 : 1.0;
    const double deploy = deployment_fraction < 0.0 ? 0.0 : (deployment_fraction > 1.0 ? 1.0 : deployment_fraction);

    const double fold_pitch = ManipulatorArmLayoutMeters::kFoldPitchStowedDegrees +
        deploy * (ManipulatorArmLayoutMeters::kFoldPitchDeployedDegrees -
                   ManipulatorArmLayoutMeters::kFoldPitchStowedDegrees);
    const double fold_roll = ManipulatorArmLayoutMeters::kRollStowedDegrees +
        deploy * (ManipulatorArmLayoutMeters::kRollDeployedDegrees - ManipulatorArmLayoutMeters::kRollStowedDegrees);

    const auto shoulder_rotation = multiply(
        rotation_y_degrees(fold_pitch + angles.shoulder_degrees),
        rotation_x_degrees(side * fold_roll));

    const Vector3d shoulder_pivot = ManipulatorArmLayoutMeters::shoulder_pivot_local_m(id);
    const Vector3d elbow_center = contact_add(
        shoulder_pivot,
        apply(shoulder_rotation, Vector3d{ManipulatorArmLayoutMeters::kElbowOffsetFromShoulderM, 0.0, 0.0}));

    const auto elbow_rotation = multiply(shoulder_rotation, rotation_y_degrees(angles.elbow_degrees));
    const Vector3d wrist_center = contact_add(
        elbow_center,
        apply(elbow_rotation, Vector3d{ManipulatorArmLayoutMeters::kWristOffsetFromElbowM, 0.0, 0.0}));

    return ManipulatorArmContactSamples{
        ManipulatorArmSample{elbow_center, ManipulatorArmLayoutMeters::kArmSampleRadiusM},
        ManipulatorArmSample{wrist_center, ManipulatorArmLayoutMeters::kArmSampleRadiusM},
    };
}

// True if the given arm pose would visually drive the arm through the
// probe's own hull -- i.e. any arm sample overlaps any hull envelope
// sample.
[[nodiscard]] inline bool manipulator_pose_intersects_hull(
    ManipulatorArmId id,
    double deployment_fraction,
    ManipulatorArmAngles angles,
    const ProbeCompoundCollisionEnvelope& hull) noexcept {
    const ManipulatorArmContactSamples samples = manipulator_arm_contact_samples(id, deployment_fraction, angles);
    const std::array<const ManipulatorArmSample*, 2> arm_samples{{&samples.elbow, &samples.wrist}};

    for (const ManipulatorArmSample* arm_sample : arm_samples) {
        for (const ProbeCollisionSphereSample& hull_sample : hull.samples) {
            const Vector3d delta = contact_subtract(arm_sample->center_m, hull_sample.local_center_m);
            const double combined_radius = arm_sample->radius_m + hull_sample.radius_m;
            if (contact_dot(delta, delta) < combined_radius * combined_radius) {
                return true;
            }
        }
    }
    return false;
}

// Builds the ManipulatorRig::SelfCollisionGuard that makes "collision does
// not allow impossible penetration through the probe body" (Slice 6's
// stated, previously-unimplemented requirement) hold: a candidate pose is
// only allowed once it does not intersect the canonical Prime Generation-1
// hull envelope. Composition roots (see ProbeSimulationAdapter.cpp) pass
// this straight to ManipulatorRig's constructor.
[[nodiscard]] inline ManipulatorRig::SelfCollisionGuard make_hull_self_collision_guard() {
    return [](ManipulatorArmId id, double deployment_fraction, ManipulatorArmAngles angles) {
        return !manipulator_pose_intersects_hull(id, deployment_fraction, angles, ProbeCompoundCollisionEnvelope{});
    };
}

// Slice 6 follow-up, part two (PHASE2_VERTICAL_SLICE_PLAN.md /
// PROJECT_STATUS.md "Manipulator arm/body self-collision"): the guard above
// only ever stops an arm from passing through the probe's own hull. Arm sweeping
// into a registered *external* body (an asteroid, the physical scan/contact
// target, etc.) was left as a stated but unimplemented Slice 6 requirement.
//
// This reuses the same arm samples and the same rotate_local_contact_offset
// convention software_policy.hpp's probe-vs-body contact already uses to place
// the probe's own hull samples in world space -- no second world-placement
// convention is invented. It is a static overlap test against each registered
// body's current geometry, not the swept/velocity-resolved contact response
// software_policy.hpp performs for the probe hull itself: that keeps this
// guard parallel-safe (PHASE2_VERTICAL_SLICE_PLAN.md's parallel-safe lane)
// rather than depending on the correctness of the still-Product-Reality-pending
// swept contact *response*.
struct ProbeWorldPose {
    Vector3d position_m{};
    EulerAttitudeDegrees attitude_degrees{};
};

[[nodiscard]] inline bool manipulator_pose_intersects_environment(
    ManipulatorArmId id,
    double deployment_fraction,
    ManipulatorArmAngles angles,
    ProbeWorldPose probe_pose,
    const std::vector<StaticSphereBody>& bodies) noexcept {
    const ManipulatorArmContactSamples local_samples =
        manipulator_arm_contact_samples(id, deployment_fraction, angles);
    const std::array<const ManipulatorArmSample*, 2> arm_samples{{&local_samples.elbow, &local_samples.wrist}};

    for (const ManipulatorArmSample* arm_sample : arm_samples) {
        const Vector3d world_center = contact_add(
            probe_pose.position_m,
            rotate_local_contact_offset(arm_sample->center_m, probe_pose.attitude_degrees));
        for (const StaticSphereBody& body : bodies) {
            const Vector3d delta = contact_subtract(world_center, body.center_m);
            const double combined_radius = arm_sample->radius_m + body.radius_m;
            if (contact_dot(delta, delta) < combined_radius * combined_radius) {
                return true;
            }
        }
    }
    return false;
}

// `probe_pose` and `registered_bodies` are queried fresh on every guard call
// (rather than captured once) so the guard always reasons about the probe's
// live position/attitude and the live registered-body set instead of a stale
// snapshot taken when the rig was constructed.
[[nodiscard]] inline ManipulatorRig::SelfCollisionGuard make_environment_collision_guard(
    std::function<ProbeWorldPose()> probe_pose,
    std::function<const std::vector<StaticSphereBody>&()> registered_bodies) {
    return [probe_pose = std::move(probe_pose), registered_bodies = std::move(registered_bodies)](
               ManipulatorArmId id, double deployment_fraction, ManipulatorArmAngles angles) {
        return !manipulator_pose_intersects_environment(
            id, deployment_fraction, angles, probe_pose(), registered_bodies());
    };
}

// Composition-root helper: ManipulatorRig accepts exactly one
// SelfCollisionGuard, so a rig that must reject both self-intersecting and
// environment-intersecting poses needs the two checks folded into one
// callback. Either guard may be empty (default-constructed std::function),
// matching ManipulatorRig's own "no guard installed" default behavior.
[[nodiscard]] inline ManipulatorRig::SelfCollisionGuard make_combined_collision_guard(
    ManipulatorRig::SelfCollisionGuard hull_guard,
    ManipulatorRig::SelfCollisionGuard environment_guard) {
    return [hull_guard = std::move(hull_guard), environment_guard = std::move(environment_guard)](
               ManipulatorArmId id, double deployment_fraction, ManipulatorArmAngles angles) {
        if (hull_guard && !hull_guard(id, deployment_fraction, angles)) {
            return false;
        }
        if (environment_guard && !environment_guard(id, deployment_fraction, angles)) {
            return false;
        }
        return true;
    };
}

} // namespace everward::simulation
