#pragma once

#include "everward/simulation/manipulator.hpp"
#include "everward/simulation/manipulator_hull_contact.hpp"
#include "everward/simulation/types.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace everward::simulation {

// First playable mining foundation. Scanning is not flavor text: a resource
// body must be surveyed before its deposit can be extracted. Mining then
// requires a deployed manipulator with a tool physically close enough to the
// surveyed body's surface. The system is engine-independent; Unreal only
// presents commands and telemetry.
struct ResourceDeposit {
    StaticSphereBody body{};
    std::string material_id{"raw_regolith"};
    std::string display_name{"Raw regolith"};
    double remaining_kg{0.0};
    double extraction_kg_per_cycle{5.0};
};

struct MiningAttemptResult {
    bool accepted{false};
    ManipulatorArmId arm{ManipulatorArmId::Port};
    double extracted_kg{0.0};
    double stored_material_kg{0.0};
    double remaining_deposit_kg{0.0};
    double tool_surface_gap_m{0.0};
    std::string detail;
};

class MiningSystem {
public:
    static constexpr double kGeneration1ToolWorkingReachM = 0.55;

    void add_deposit(ResourceDeposit deposit) {
        if (deposit.body.body_id.empty()) {
            throw std::invalid_argument("resource deposit body id must not be empty");
        }
        if (deposit.material_id.empty() || deposit.display_name.empty()) {
            throw std::invalid_argument("resource deposit material identity must not be empty");
        }
        if (!(deposit.body.radius_m > 0.0) || !std::isfinite(deposit.body.radius_m)) {
            throw std::invalid_argument("resource deposit radius must be finite and positive");
        }
        if (!std::isfinite(deposit.remaining_kg) || deposit.remaining_kg < 0.0) {
            throw std::invalid_argument("resource deposit remaining mass must be finite and non-negative");
        }
        if (!(deposit.extraction_kg_per_cycle > 0.0) || !std::isfinite(deposit.extraction_kg_per_cycle)) {
            throw std::invalid_argument("resource extraction rate must be finite and positive");
        }
        const std::string id = deposit.body.body_id;
        if (deposits_.contains(id)) {
            throw std::invalid_argument("resource deposit already registered: " + id);
        }
        deposits_.emplace(id, DepositState{std::move(deposit), false});
    }

    [[nodiscard]] bool has_deposit(const std::string& body_id) const noexcept {
        return deposits_.contains(body_id);
    }

    void mark_surveyed(const std::string& body_id) {
        auto it = deposits_.find(body_id);
        if (it == deposits_.end()) {
            throw std::runtime_error("scan target has no registered resource deposit: " + body_id);
        }
        it->second.surveyed = true;
    }

    [[nodiscard]] bool is_surveyed(const std::string& body_id) const noexcept {
        const auto it = deposits_.find(body_id);
        return it != deposits_.end() && it->second.surveyed;
    }

    [[nodiscard]] double stored_material_kg() const noexcept {
        return stored_material_kg_;
    }

    [[nodiscard]] std::string survey_summary(const std::string& body_id) const {
        const auto it = deposits_.find(body_id);
        if (it == deposits_.end()) {
            return "scan complete: no extractable deposit registered";
        }
        const auto& deposit = it->second.deposit;
        return deposit.display_name + " // " +
            whole_kilograms(deposit.remaining_kg) + " kg estimated extractable // MINING UNLOCKED";
    }

    [[nodiscard]] MiningAttemptResult mine_once(
        const std::string& body_id,
        const ManipulatorRig& rig,
        ProbeWorldPose probe_pose,
        double existing_storage_kg,
        double storage_capacity_kg) {
        MiningAttemptResult result;

        auto it = deposits_.find(body_id);
        if (it == deposits_.end()) {
            result.detail = "target has no registered resource deposit";
            return result;
        }
        DepositState& state = it->second;
        result.remaining_deposit_kg = state.deposit.remaining_kg;

        if (!state.surveyed) {
            result.detail = "scan target first; mining requires a completed resource survey";
            return result;
        }
        if (state.deposit.remaining_kg <= 0.0) {
            result.detail = "surveyed deposit is exhausted";
            return result;
        }
        if (!std::isfinite(existing_storage_kg) || !std::isfinite(storage_capacity_kg) ||
            existing_storage_kg < 0.0 || storage_capacity_kg < 0.0) {
            result.detail = "invalid storage telemetry";
            return result;
        }

        const double total_stored = existing_storage_kg + stored_material_kg_;
        const double free_storage = std::max(0.0, storage_capacity_kg - total_stored);
        if (free_storage <= 0.0) {
            result.detail = "storage is full";
            return result;
        }

        bool found_ready_arm = false;
        double best_gap = 1.0e30;
        ManipulatorArmId best_arm = ManipulatorArmId::Port;

        for (ManipulatorArmId arm_id : {ManipulatorArmId::Port, ManipulatorArmId::Starboard}) {
            const ManipulatorArmState& arm = rig.arm(arm_id);
            if (!arm.is_deployed || arm.is_deploying || arm.is_stowing || !arm.tool_attached) {
                continue;
            }
            found_ready_arm = true;

            const ManipulatorArmContactSamples local_samples =
                manipulator_arm_contact_samples(arm_id, arm.deployment_fraction, arm.angles);
            const Vector3d wrist_world = contact_add(
                probe_pose.position_m,
                rotate_local_contact_offset(local_samples.wrist.center_m, probe_pose.attitude_degrees));
            const Vector3d delta = contact_subtract(wrist_world, state.deposit.body.center_m);
            const double center_distance = std::sqrt(contact_dot(delta, delta));
            const double surface_gap = center_distance -
                (state.deposit.body.radius_m + local_samples.wrist.radius_m);

            if (surface_gap < best_gap) {
                best_gap = surface_gap;
                best_arm = arm_id;
            }
        }

        if (!found_ready_arm) {
            result.detail = "deploy a manipulator arm and attach its tool before mining";
            return result;
        }

        result.arm = best_arm;
        result.tool_surface_gap_m = best_gap;
        if (best_gap > kGeneration1ToolWorkingReachM) {
            result.detail = "mining tool is not in reach; surface gap " +
                one_decimal(best_gap) + " m (need <= " + one_decimal(kGeneration1ToolWorkingReachM) + " m)";
            return result;
        }
        if (best_gap < -0.02) {
            result.detail = "mining tool geometry is penetrating the target; back out before extraction";
            return result;
        }

        const double extracted = std::min({
            state.deposit.extraction_kg_per_cycle,
            state.deposit.remaining_kg,
            free_storage,
        });
        if (extracted <= 0.0) {
            result.detail = "no material could be extracted";
            return result;
        }

        state.deposit.remaining_kg -= extracted;
        stored_material_kg_ += extracted;

        result.accepted = true;
        result.extracted_kg = extracted;
        result.stored_material_kg = stored_material_kg_;
        result.remaining_deposit_kg = state.deposit.remaining_kg;
        result.detail = "MINED " + one_decimal(extracted) + " kg " + state.deposit.display_name +
            " // cargo " + one_decimal(total_stored + extracted) + "/" +
            one_decimal(storage_capacity_kg) + " kg // deposit " +
            one_decimal(state.deposit.remaining_kg) + " kg remaining";
        return result;
    }

private:
    struct DepositState {
        ResourceDeposit deposit{};
        bool surveyed{false};
    };

    [[nodiscard]] static std::string whole_kilograms(double value) {
        return std::to_string(static_cast<long long>(std::llround(value)));
    }

    [[nodiscard]] static std::string one_decimal(double value) {
        const long long scaled = static_cast<long long>(std::llround(value * 10.0));
        const long long whole = scaled / 10;
        const long long fraction = std::llabs(scaled % 10);
        return std::to_string(whole) + "." + std::to_string(fraction);
    }

    std::unordered_map<std::string, DepositState> deposits_{};
    double stored_material_kg_{0.0};
};

} // namespace everward::simulation
