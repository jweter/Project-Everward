#pragma once
// Engine-independent core of the Godot binding (Godot path G1). It owns the real Everward simulation
// objects and exposes the few operations the binding spike needs. The GDExtension wrapper and the plain
// C++ reference program both use this file, so a parity check compares Godot's results with native ones.

#include "everward/simulation/mining.hpp"
#include "everward/simulation/save_data.hpp"

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>

namespace everward::binding {

using namespace everward::simulation;

struct MineResult {
    bool accepted{false};
    double extracted_kg{0.0};
    double remaining_kg{0.0};
    std::string material_id;
    std::string reason;
};

class SimCore {
public:
    SimCore() { reset(); }

    void reset() {
        runtime_ = DamageAwareProbeRuntime::make_canonical_ev0001();
        rig_ = ManipulatorRig{};
        mining_ = MiningSystem{};
        deposit_remaining_kg_ = 0.0;
        extracted_kg_ = 0.0;
        has_deposit_ = false;
    }

    void advance_ticks(std::int64_t wall_ticks) { runtime_.advance_wall_ticks(wall_ticks); }
    [[nodiscard]] std::int64_t tick() const { return runtime_.tick(); }
    void set_velocity(double x, double y, double z) { runtime_.set_velocity_mps({x, y, z}); }
    [[nodiscard]] Vector3d position() const { return runtime_.snapshot().position_m; }

    // Deploy the port arm, attach its tool and place a deposit just beyond the tool tip (the same setup
    // the native save/deposit regression test uses).
    void setup_bootstrap_deposit(double remaining_kg) {
        rig_ = ManipulatorRig{};
        rig_.begin_deploy(ManipulatorArmId::Port);
        rig_.advance(ManipulatorRig::kDeployStowDurationS);
        rig_.attach_tool(ManipulatorArmId::Port);
        const ManipulatorArmState& arm = rig_.arm(ManipulatorArmId::Port);
        const ManipulatorArmSample tip =
            manipulator_tool_tip_contact_sample(ManipulatorArmId::Port, arm.deployment_fraction, arm.angles);
        ResourceDeposit deposit;
        deposit.body.body_id = kTarget;
        deposit.body.radius_m = 0.50;
        deposit.body.center_m = {tip.center_m.x + deposit.body.radius_m + tip.radius_m + 0.20, tip.center_m.y,
                                 tip.center_m.z};
        deposit.material_id = "iron_regolith";
        deposit.display_name = "Iron-bearing regolith";
        deposit.remaining_kg = remaining_kg;
        deposit.extraction_kg_per_cycle = 5.0;
        mining_ = MiningSystem{};
        mining_.add_deposit(deposit);
        mining_.mark_surveyed(kTarget);
        deposit_remaining_kg_ = remaining_kg;
        has_deposit_ = true;
    }

    MineResult mine_once() {
        MineResult out;
        if (!has_deposit_) {
            out.reason = "no deposit";
            return out;
        }
        const MiningAttemptResult r = mining_.mine_once(kTarget, rig_, ProbeWorldPose{},
                                                        runtime_.snapshot().storage_used_kg,
                                                        runtime_.snapshot().storage_capacity_kg);
        out.accepted = r.accepted;
        if (r.accepted) {
            runtime_.add_stored_material_kg(r.extracted_kg, r.material_id);
            extracted_kg_ += r.extracted_kg;
            deposit_remaining_kg_ = r.remaining_deposit_kg;
            out.extracted_kg = r.extracted_kg;
            out.remaining_kg = r.remaining_deposit_kg;
            out.material_id = r.material_id;
        }
        return out;
    }

    [[nodiscard]] double inventory_kg(const std::string& material) const {
        const auto& inv = runtime_.snapshot().material_inventory_kg;
        const auto it = inv.find(material);
        return it == inv.end() ? 0.0 : it->second;
    }
    [[nodiscard]] double extracted_kg() const { return extracted_kg_; }
    [[nodiscard]] double deposit_remaining_kg() const { return deposit_remaining_kg_; }

    [[nodiscard]] std::string save_json() const {
        SaveGameV1 save;
        save.simulation_tick = runtime_.tick();
        ProbeSaveData data = capture_probe_save_data(runtime_, rig_);
        data.bootstrap_resource_surveyed = has_deposit_;
        if (has_deposit_) {
            data.bootstrap_deposit_remaining_kg = deposit_remaining_kg_;
            data.bootstrap_extracted_kg = extracted_kg_;
        }
        save.probes.push_back(data);
        return serialize_save_game(save);
    }

    // Restores the runtime and the deposit counters, then rebuilds the deposit at the saved remaining mass.
    // Returns false (and changes nothing) if the save cannot be parsed.
    bool load_json(const std::string& json) {
        SaveGameV1 loaded;
        try {
            loaded = deserialize_save_game(json);
        } catch (const std::exception&) {
            return false;
        }
        if (loaded.probes.empty()) return false;
        const ProbeSaveData& data = loaded.probes.at(0);
        runtime_ = DamageAwareProbeRuntime(restore_probe_runtime(data, loaded.simulation_tick));
        const double remaining = data.bootstrap_deposit_remaining_kg.value_or(deposit_remaining_kg_);
        extracted_kg_ = data.bootstrap_extracted_kg.value_or(extracted_kg_);
        if (data.bootstrap_resource_surveyed) {
            const double keep_extracted = extracted_kg_;
            setup_bootstrap_deposit(remaining);
            extracted_kg_ = keep_extracted;
        }
        return true;
    }

private:
    static constexpr const char* kTarget = "bootstrap-rock";
    DamageAwareProbeRuntime runtime_ = DamageAwareProbeRuntime::make_canonical_ev0001();
    ManipulatorRig rig_;
    MiningSystem mining_;
    double deposit_remaining_kg_{0.0};
    double extracted_kg_{0.0};
    bool has_deposit_{false};
};

}  // namespace everward::binding
