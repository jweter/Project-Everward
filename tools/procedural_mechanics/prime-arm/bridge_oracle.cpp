// Copyright Project Everward contributors.
// Standalone, deterministic cross-language oracle. Never writes gameplay state.
// Build from repository root:
// c++ -std=c++20 -Isrc/simulation/include tools/procedural_mechanics/prime-arm/bridge_oracle.cpp -o /tmp/everward_arm_oracle
// /tmp/everward_arm_oracle contract | poses
#include "everward/simulation/manipulator_tool_contact.hpp"
#include <array>
#include <iomanip>
#include <iostream>
#include <string_view>

using namespace everward::simulation;

namespace {
void vec(Vector3d p) {
    std::cout << '[' << p.x << ',' << p.y << ',' << p.z << ']';
}

void pair(double first, double second) {
    std::cout << '[' << first << ',' << second << ']';
}

void contract() {
    const auto port = ManipulatorArmLayoutMeters::shoulder_pivot_local_m(ManipulatorArmId::Port);
    const auto starboard = ManipulatorArmLayoutMeters::shoulder_pivot_local_m(ManipulatorArmId::Starboard);
    const auto shoulder = ManipulatorRig::shoulder_range();
    const auto elbow = ManipulatorRig::elbow_range();
    const auto wrist = ManipulatorRig::wrist_range();
    std::cout << '{';
    std::cout << "\"shoulder_m\":{\"port\":"; vec(port);
    std::cout << ",\"starboard\":"; vec(starboard);
    std::cout << "},\"link_m\":{";
    std::cout << "\"upper\":" << ManipulatorArmLayoutMeters::kElbowOffsetFromShoulderM;
    std::cout << ",\"forearm\":" << ManipulatorArmLayoutMeters::kWristOffsetFromElbowM;
    std::cout << ",\"tool_tip_from_wrist\":" << ManipulatorToolLayoutMeters::kTipOffsetFromWristM;
    std::cout << "},\"sample_radius_m\":{";
    std::cout << "\"arm\":" << ManipulatorArmLayoutMeters::kArmSampleRadiusM;
    std::cout << ",\"tool_tip\":" << ManipulatorToolLayoutMeters::kTipContactRadiusM;
    std::cout << "},\"fold_pitch_degrees\":{";
    std::cout << "\"stowed\":" << ManipulatorArmLayoutMeters::kFoldPitchStowedDegrees;
    std::cout << ",\"deployed\":" << ManipulatorArmLayoutMeters::kFoldPitchDeployedDegrees;
    std::cout << "},\"fold_roll_degrees\":{";
    std::cout << "\"stowed\":" << ManipulatorArmLayoutMeters::kRollStowedDegrees;
    std::cout << ",\"deployed\":" << ManipulatorArmLayoutMeters::kRollDeployedDegrees;
    std::cout << "},\"joint_limits_degrees\":{";
    std::cout << "\"shoulder\":"; pair(shoulder.min_degrees, shoulder.max_degrees);
    std::cout << ",\"elbow\":"; pair(elbow.min_degrees, elbow.max_degrees);
    std::cout << ",\"wrist\":"; pair(wrist.min_degrees, wrist.max_degrees);
    std::cout << "}}\n";
}

void cases() {
    constexpr std::array<double, 4> fractions{0.0, 0.35, 0.7, 1.0};
    constexpr std::array<ManipulatorArmAngles, 5> angle_sets{{
        {0.0, 0.0, 0.0},
        {-45.0, 80.0, -20.0},
        {20.0, 30.0, 90.0},
        {60.0, 0.0, -90.0},
        {-20.0, 140.0, 160.0},
    }};
    std::cout << "{\"cases\":[";
    bool first = true;
    for (const auto arm : {ManipulatorArmId::Port, ManipulatorArmId::Starboard}) {
        for (const double fraction : fractions) {
            for (const auto angles : angle_sets) {
                if (!first) std::cout << ',';
                first = false;
                const auto samples = manipulator_arm_contact_samples(arm, fraction, angles);
                const auto tip = manipulator_tool_tip_contact_sample(arm, fraction, angles);
                std::cout << "{\"arm\":\""
                    << (arm == ManipulatorArmId::Port ? "port" : "starboard")
                    << "\",\"deployment_fraction\":" << fraction
                    << ",\"angles_degrees\":{\"shoulder\":" << angles.shoulder_degrees
                    << ",\"elbow\":" << angles.elbow_degrees
                    << ",\"wrist\":" << angles.wrist_degrees
                    << "},\"elbow_m\":";
                vec(samples.elbow.center_m);
                std::cout << ",\"wrist_m\":"; vec(samples.wrist.center_m);
                std::cout << ",\"tool_tip_m\":"; vec(tip.center_m);
                std::cout << ",\"contact_radius_m\":" << tip.radius_m << '}';
            }
        }
    }
    std::cout << "]}\n";
}
}

int main(int argc, char** argv) {
    std::cout << std::fixed << std::setprecision(12);
    if (argc != 2) {
        std::cerr << "usage: bridge_oracle contract|poses\n";
        return 2;
    }
    const std::string_view mode{argv[1]};
    if (mode == "contract") contract();
    else if (mode == "poses") cases();
    else {
        std::cerr << "unknown oracle mode\n";
        return 2;
    }
    return 0;
}
