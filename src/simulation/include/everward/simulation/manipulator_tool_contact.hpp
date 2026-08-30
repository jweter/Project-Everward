#pragma once

#include "everward/simulation/manipulator_hull_contact.hpp"

namespace everward::simulation {

// Generation-1 tool geometry mirrored from EverwardProbePawn.cpp:
// ToolHead center is 0.38 m forward of the wrist pivot. With the attached-tool
// visual scale of 0.82 on Unreal's 1 m basic cylinder, the forward half-length
// is 0.41 m. The physical mining contact point is therefore 0.79 m beyond the
// wrist along the fully composed wrist orientation.
struct ManipulatorToolLayoutMeters {
    static constexpr double kTipOffsetFromWristM = 0.79;
    static constexpr double kTipContactRadiusM = 0.17;
};

[[nodiscard]] inline ManipulatorArmSample manipulator_tool_tip_contact_sample(
    ManipulatorArmId id,
    double deployment_fraction,
    ManipulatorArmAngles angles) noexcept {
    using manipulator_hull_contact_detail::apply;
    using manipulator_hull_contact_detail::multiply;
    using manipulator_hull_contact_detail::rotation_x_degrees;
    using manipulator_hull_contact_detail::rotation_y_degrees;

    const double side = id == ManipulatorArmId::Port ? -1.0 : 1.0;
    const double deploy = deployment_fraction < 0.0
        ? 0.0
        : (deployment_fraction > 1.0 ? 1.0 : deployment_fraction);

    const double fold_pitch = ManipulatorArmLayoutMeters::kFoldPitchStowedDegrees +
        deploy * (ManipulatorArmLayoutMeters::kFoldPitchDeployedDegrees -
                   ManipulatorArmLayoutMeters::kFoldPitchStowedDegrees);
    const double fold_roll = ManipulatorArmLayoutMeters::kRollStowedDegrees +
        deploy * (ManipulatorArmLayoutMeters::kRollDeployedDegrees -
                   ManipulatorArmLayoutMeters::kRollStowedDegrees);

    const auto shoulder_rotation = multiply(
        rotation_y_degrees(fold_pitch + angles.shoulder_degrees),
        rotation_x_degrees(side * fold_roll));
    const auto elbow_rotation = multiply(
        shoulder_rotation,
        rotation_y_degrees(angles.elbow_degrees));
    const auto wrist_rotation = multiply(
        elbow_rotation,
        rotation_y_degrees(angles.wrist_degrees));

    const Vector3d shoulder = ManipulatorArmLayoutMeters::shoulder_pivot_local_m(id);
    const Vector3d elbow = contact_add(
        shoulder,
        apply(shoulder_rotation, Vector3d{ManipulatorArmLayoutMeters::kElbowOffsetFromShoulderM, 0.0, 0.0}));
    const Vector3d wrist = contact_add(
        elbow,
        apply(elbow_rotation, Vector3d{ManipulatorArmLayoutMeters::kWristOffsetFromElbowM, 0.0, 0.0}));
    const Vector3d tip = contact_add(
        wrist,
        apply(wrist_rotation, Vector3d{ManipulatorToolLayoutMeters::kTipOffsetFromWristM, 0.0, 0.0}));

    return ManipulatorArmSample{tip, ManipulatorToolLayoutMeters::kTipContactRadiusM};
}

} // namespace everward::simulation
