#pragma once
// Engine-independent core of the Godot binding (Godot path G1). It owns the real Everward simulation
// objects and exposes the few operations the binding spike needs. The GDExtension wrapper and the plain
// C++ reference program both use this file, so a parity check compares Godot's results with native ones.
//
// Mining follows the Unreal adapter's bridge (ProbeMiningBridge.cpp): a fresh MiningSystem is built for each
// attempt from the deposit's current remaining mass, and the runtime's storage_used_kg is the single storage
// account. (MiningSystem also counts what it extracted; keeping one alive across attempts would count cargo
// twice.) Every operation that can see bad input returns a failure instead of throwing into the engine.

#include "everward/simulation/mining.hpp"
#include "everward/simulation/save_data.hpp"

#include <cmath>
#include <cstdint>
#include <exception>
#include <optional>
#include <string>
#include <utility>

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
        deposit_remaining_kg_ = 0.0;
        extracted_kg_ = 0.0;
        has_deposit_ = false;
    }

    // Commands return false for input the simulation rejects; no exception ever leaves this class.
    bool advance_ticks(std::int64_t wall_ticks) {
        if (wall_ticks < 0) return false;
        try {
            runtime_.advance_wall_ticks(wall_ticks);
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }
    [[nodiscard]] std::int64_t tick() const { return runtime_.tick(); }
    bool set_velocity(double x, double y, double z) {
        if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) return false;
        try {
            runtime_.set_velocity_mps({x, y, z});
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }
    [[nodiscard]] Vector3d position() const { return runtime_.snapshot().position_m; }
    [[nodiscard]] Vector3d velocity() const { return runtime_.snapshot().velocity_mps; }
    [[nodiscard]] EulerAttitudeDegrees attitude() const { return runtime_.snapshot().attitude_degrees; }

    // G2 flight: turn the probe, and change velocity in the probe's own frame (x forward, y right, z up).
    bool adjust_attitude(double yaw, double pitch, double roll) {
        if (!std::isfinite(yaw) || !std::isfinite(pitch) || !std::isfinite(roll)) return false;
        try {
            runtime_.adjust_attitude_degrees({yaw, pitch, roll});
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }
    bool adjust_local_velocity(double x, double y, double z) {
        if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) return false;
        try {
            runtime_.adjust_local_velocity_mps({x, y, z});
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }

    // Static sphere bodies the probe can collide with (rocks, boulders). The simulation owns contact.
    bool add_sphere_body(const std::string& id, double x, double y, double z, double radius) {
        if (id.empty() || !std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z) || !std::isfinite(radius) ||
            radius <= 0.0) {
            return false;
        }
        try {
            StaticSphereBody body;
            body.body_id = id;
            body.center_m = {x, y, z};
            body.radius_m = radius;
            runtime_.add_static_sphere_body(body);
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }
    void clear_bodies() noexcept { runtime_.clear_static_bodies(); }

    struct ContactInfo {
        bool has_contact{false};
        std::string body_id;
        Vector3d point{};
        Vector3d normal{};
        double normal_speed{0.0};
        std::int64_t tick{0};
    };
    [[nodiscard]] ContactInfo last_contact() const {
        const auto& s = runtime_.snapshot();
        return ContactInfo{s.has_contact_history, s.last_contact_body_id, s.last_contact_point_m,
                           s.last_contact_surface_normal, s.last_contact_normal_speed_mps, s.last_contact_tick};
    }
    // Drains the simulation's event queue and returns how many contact events it held.
    int drain_contact_events() {
        int contacts = 0;
        for (const DomainEvent& e : runtime_.drain_events()) {
            if (e.type == DomainEventType::Contact) ++contacts;
        }
        return contacts;
    }

    // Deploy the port arm, attach its tool and register a surveyed deposit just beyond the tool tip (the same
    // setup the native save/deposit regression test uses).
    bool setup_bootstrap_deposit(double remaining_kg) {
        if (!std::isfinite(remaining_kg) || remaining_kg < 0.0) return false;
        rig_ = ManipulatorRig{};
        rig_.begin_deploy(ManipulatorArmId::Port);
        rig_.advance(ManipulatorRig::kDeployStowDurationS);
        rig_.attach_tool(ManipulatorArmId::Port);
        deposit_remaining_kg_ = remaining_kg;
        has_deposit_ = true;
        return true;
    }

    MineResult mine_once() {
        MineResult out;
        if (!has_deposit_) {
            out.reason = "no deposit";
            return out;
        }
        MiningAttemptResult r;
        try {
            MiningSystem mining;  // fresh per attempt, like the game's mining bridge
            mining.add_deposit(deposit_at_tool_tip(deposit_remaining_kg_));
            mining.mark_surveyed(kTarget);
            const auto& snap = runtime_.snapshot();
            r = mining.mine_once(kTarget, rig_, ProbeWorldPose{}, snap.storage_used_kg, snap.storage_capacity_kg);
        } catch (const std::exception& e) {
            out.reason = std::string("rejected: ") + e.what();
            return out;
        }
        out.accepted = r.accepted;
        out.remaining_kg = r.remaining_deposit_kg;
        out.reason = r.detail;
        if (r.accepted) {
            runtime_.add_stored_material_kg(r.extracted_kg, r.material_id);
            extracted_kg_ += r.extracted_kg;
            deposit_remaining_kg_ = r.remaining_deposit_kg;
            out.extracted_kg = r.extracted_kg;
            out.material_id = r.material_id;
        }
        return out;
    }

    [[nodiscard]] double inventory_kg(const std::string& material) const {
        const auto& inv = runtime_.snapshot().material_inventory_kg;
        const auto it = inv.find(material);
        return it == inv.end() ? 0.0 : it->second;
    }
    [[nodiscard]] double storage_used_kg() const { return runtime_.snapshot().storage_used_kg; }
    [[nodiscard]] double storage_capacity_kg() const { return runtime_.snapshot().storage_capacity_kg; }
    [[nodiscard]] double extracted_kg() const { return extracted_kg_; }
    [[nodiscard]] double deposit_remaining_kg() const { return deposit_remaining_kg_; }
    [[nodiscard]] bool has_deposit() const { return has_deposit_; }

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

    // Replaces the whole simulation state with the save's. Everything is parsed, validated and built into
    // temporaries first; if anything fails, nothing changes and false is returned (never an exception).
    // A save without a recorded deposit restores the no-deposit state rather than keeping the current one.
    bool load_json(const std::string& json) {
        try {
            const SaveGameV1 loaded = deserialize_save_game(json);
            if (loaded.probes.empty()) return false;
            const ProbeSaveData& data = loaded.probes.at(0);
            DamageAwareProbeRuntime new_runtime(restore_probe_runtime(data, loaded.simulation_tick));
            ManipulatorRig new_rig = restore_manipulator_rig(data);
            const bool new_has_deposit =
                data.bootstrap_resource_surveyed && data.bootstrap_deposit_remaining_kg.has_value();
            const double new_remaining = new_has_deposit ? *data.bootstrap_deposit_remaining_kg : 0.0;
            const double new_extracted = data.bootstrap_extracted_kg.value_or(0.0);
            runtime_ = std::move(new_runtime);
            rig_ = std::move(new_rig);
            has_deposit_ = new_has_deposit;
            deposit_remaining_kg_ = new_remaining;
            extracted_kg_ = new_extracted;
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }

private:
    [[nodiscard]] ResourceDeposit deposit_at_tool_tip(double remaining_kg) const {
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
        return deposit;
    }

    static constexpr const char* kTarget = "bootstrap-rock";
    DamageAwareProbeRuntime runtime_ = DamageAwareProbeRuntime::make_canonical_ev0001();
    ManipulatorRig rig_;
    double deposit_remaining_kg_{0.0};
    double extracted_kg_{0.0};
    bool has_deposit_{false};
};

}  // namespace everward::binding
