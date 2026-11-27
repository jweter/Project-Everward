#pragma once
// Engine-independent core of the Godot binding (Godot path G1). It owns the real Everward simulation
// objects and exposes the few operations the binding spike needs. The GDExtension wrapper and the plain
// C++ reference program both use this file, so a parity check compares Godot's results with native ones.
//
// Mining follows the Unreal adapter's bridge (ProbeMiningBridge.cpp): a fresh MiningSystem is built for each
// attempt from the deposit's current remaining mass, and the runtime's storage_used_kg is the single storage
// account. (MiningSystem also counts what it extracted; keeping one alive across attempts would count cargo
// twice.) Every operation that can see bad input returns a failure instead of throwing into the engine.

#include "everward/simulation/first_loop_controls.hpp"
#include "everward/simulation/fix_it.hpp"
#include "everward/simulation/manipulator_hull_contact.hpp"
#include "everward/simulation/manipulator_path_planner.hpp"
#include "everward/simulation/mining.hpp"
#include "everward/simulation/mining_staging.hpp"
#include "everward/simulation/save_data.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <exception>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace everward::binding {

using namespace everward::simulation;

struct MineResult {
    bool accepted{false};
    double extracted_kg{0.0};
    double remaining_kg{0.0};
    double tool_gap_m{0.0};
    int arm{0};
    std::string material_id;
    std::string reason;
};

// G3: a mineable deposit placed in the world (a registered static body plus its resource record).
struct WorldDeposit {
    ResourceDeposit deposit;
    double extracted_kg{0.0};
    bool surveyed{false};
};

struct StagingPlan {
    bool has_solution{false};
    Vector3d probe_position{};
    ManipulatorArmAngles angles{};
    double tool_gap_m{0.0};
};

// Fix_It as the Unreal FixItRuntimeActor runs it: replan when idle, start a Repair decision only when stored
// material and energy cover it, advance the executor, wait (never run) when short.
struct FixItInfo {
    bool enabled{false};
    std::string state{"off"};     // off, monitor, waiting, repairing
    std::string stage;
    std::string subsystem;
    std::string reason;
    double progress{0.0};
    double material_required_kg{0.0};
    double material_consumed_kg{0.0};
    double last_completed_material_consumed_kg{0.0};
    // Every kilogram Fix_It has taken from storage since it was enabled (completed and active repairs), measured as
    // the storage debit of each executor step.
    double total_material_consumed_kg{0.0};
    double energy_required_j{0.0};
    double energy_consumed_j{0.0};
    double target_integrity{0.0};
    std::string last_event;
};

struct ArmInfo {
    bool deployed{false};
    bool deploying{false};
    bool stowing{false};
    bool tool_attached{false};
    bool accepts_joint_commands{false};
    double deployment_fraction{0.0};
    ManipulatorArmAngles angles{};
    ManipulatorArmAngles commanded{};
};

class SimCore {
public:
    // The Unreal adapter's fixed step (ProbeSimulationAdapter.h FixedStepTicks): wall time is accumulated and the
    // runtime and the manipulator rig advance together in whole steps of this size, so the arm path planner's
    // frame time matches the follower's exactly.
    static constexpr std::int64_t kFixedStepTicks = 16667;

    SimCore() { reset(); }
    // The rig's collision guard reads this object's runtime, so a SimCore is never copied or moved.
    SimCore(const SimCore&) = delete;
    SimCore& operator=(const SimCore&) = delete;

    void reset() {
        runtime_ = DamageAwareProbeRuntime::make_canonical_ev0001();
        rig_ = ManipulatorRig(collision_guard());
        deposit_remaining_kg_ = 0.0;
        extracted_kg_ = 0.0;
        has_deposit_ = false;
        deposits_.clear();
        pending_contacts_ = 0;
        accumulator_ticks_ = 0;
        fix_it_enabled_ = false;
        fix_it_running_ = false;
        fix_it_waiting_ = false;
        fix_it_repair_ = FixItRepairExecutor{};
        fix_it_last_completed_material_consumed_kg_ = 0.0;
        fix_it_total_material_consumed_kg_ = 0.0;
        fix_it_last_decision_.reset();
        fix_it_last_event_.clear();
        fix_it_events_.clear();
    }

    // Commands return false for input the simulation rejects; no exception ever leaves this class.
    bool advance_ticks(std::int64_t wall_ticks) {
        if (wall_ticks < 0) return false;
        try {
            accumulator_ticks_ += wall_ticks;
            while (accumulator_ticks_ >= kFixedStepTicks) {
                runtime_.advance_wall_ticks(kFixedStepTicks);
                rig_.advance(static_cast<double>(kFixedStepTicks) / SimulationClock::TicksPerSecond);
                if (fix_it_enabled_) advance_fix_it(static_cast<double>(kFixedStepTicks) / SimulationClock::TicksPerSecond);
                accumulator_ticks_ -= kFixedStepTicks;
                consume_events();
            }
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
    // Moves a registered body (used for terrain proxy spheres that follow the ground under the probe).
    bool move_sphere_body(const std::string& id, double x, double y, double z) {
        if (id.empty() || !std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) return false;
        bool known = false;  // the simulation ignores unknown ids silently; report them instead
        for (const StaticSphereBody& body : runtime_.static_bodies()) known = known || body.body_id == id;
        if (!known) return false;
        try {
            runtime_.update_static_sphere_body_position(id, {x, y, z});
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }

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
    // Returns how many contact events the simulation raised since the last call.
    int drain_contact_events() {
        consume_events();
        const int contacts = pending_contacts_;
        pending_contacts_ = 0;
        return contacts;
    }

    // ------------------------------------------------------------------ G3 first loop
    // A deposit in the world: a static body the probe collides with, and the resource it holds.
    bool add_deposit(const std::string& id, double x, double y, double z, double radius, const std::string& material_id,
                     const std::string& display_name, double remaining_kg, double kg_per_cycle) {
        if (id.empty() || material_id.empty() || deposits_.count(id) != 0 || !std::isfinite(remaining_kg) ||
            remaining_kg < 0.0 || !std::isfinite(kg_per_cycle) || kg_per_cycle <= 0.0) {
            return false;
        }
        if (!add_sphere_body(id, x, y, z, radius)) return false;
        WorldDeposit d;
        d.deposit.body = find_body(id).value();
        d.deposit.body.material_id = material_id;
        d.deposit.material_id = material_id;
        d.deposit.display_name = display_name.empty() ? material_id : display_name;
        d.deposit.remaining_kg = remaining_kg;
        d.deposit.extraction_kg_per_cycle = kg_per_cycle;
        deposits_.emplace(id, d);
        return true;
    }
    [[nodiscard]] bool is_surveyed(const std::string& id) const {
        const auto it = deposits_.find(id);
        return it != deposits_.end() && it->second.surveyed;
    }
    [[nodiscard]] double world_deposit_remaining_kg(const std::string& id) const {
        const auto it = deposits_.find(id);
        return it == deposits_.end() ? -1.0 : it->second.deposit.remaining_kg;
    }

    bool start_scan(const std::string& target_id, double duration_s) {
        last_scan_refusal_.clear();
        if (target_id.empty() || !std::isfinite(duration_s) || duration_s <= 0.0) {
            last_scan_refusal_ = "invalid scan request";
            return false;
        }
        try {
            runtime_.start_scan(target_id, duration_s);
            return true;
        } catch (const std::exception& e) {
            last_scan_refusal_ = e.what();  // e.g. the sensor-power guidance (G09)
            return false;
        }
    }
    [[nodiscard]] const std::string& last_scan_refusal() const { return last_scan_refusal_; }

    // ------------------------------------------------------------------ power and software policy (#378 G09)
    // Subsystems: 0 sensors, 1 propulsion, 2 computation, 3 thermal. Lowering sensors below the scan floor during a
    // scan cancels it (the simulation's rule).
    bool allocate_power(int subsystem, double watts) {
        const auto s = power_subsystem(subsystem);
        if (!s || !std::isfinite(watts) || watts < 0.0) return false;
        try {
            runtime_.allocate_power(*s, watts);
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }
    [[nodiscard]] std::vector<double> power_allocations() const {
        const auto& s = runtime_.snapshot();
        return {s.power_allocated_sensors_w, s.power_allocated_propulsion_w, s.power_allocated_computation_w,
                s.power_allocated_thermal_w};
    }
    // The Unreal adapter's GEN1 BASIC SURVIVAL policy (CommandInstallBasicSurvivalPolicy), rule for rule: shed sensor
    // power below 60% stored energy, shed propulsion above 350 K.
    bool install_basic_survival_policy() {
        try {
            SoftwarePolicy policy;
            policy.id = "gen1_basic_survival";
            policy.rules = {
                {"shed_sensors_below_60_percent_energy", PolicyConditionKind::EnergyFractionBelow, 0.60,
                 PolicyActionKind::SetPowerAllocation, PowerSubsystem::Sensors, 0.0},
                {"shed_propulsion_above_350_kelvin", PolicyConditionKind::TemperatureAboveKelvin, 350.0,
                 PolicyActionKind::SetPowerAllocation, PowerSubsystem::Propulsion, 0.0},
            };
            runtime_.install_policy(std::move(policy));
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }
    bool clear_policy() {
        if (!runtime_.policy_status().installed) return false;
        runtime_.clear_policy();
        return true;
    }
    [[nodiscard]] SoftwarePolicyStatus policy_status() const { return runtime_.policy_status(); }
    [[nodiscard]] SensorPowerGuidance scan_power_guidance() const { return runtime_.scan_power_guidance(); }
    bool cancel_scan() {
        try {
            runtime_.cancel_scan();
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }
    [[nodiscard]] bool is_scanning() const { return runtime_.snapshot().is_scanning; }
    [[nodiscard]] double scan_remaining_s() const { return runtime_.snapshot().scan_remaining_s; }
    [[nodiscard]] std::string scan_target() const { return runtime_.snapshot().active_scan_target_id; }

    // Arms: 0 = port, 1 = starboard. One key toggles an arm; first_loop_controls decides the direction (G06).
    bool toggle_arm(int arm) {
        const auto id = arm_id(arm);
        if (!id) return false;
        try {
            if (arm_toggle_command(rig_.arm(*id)) == ArmToggleCommand::Deploy) {
                rig_.begin_deploy(*id);
            } else {
                rig_.begin_stow(*id);
            }
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }
    bool attach_tool(int arm) {
        const auto id = arm_id(arm);
        if (!id) return false;
        try {
            rig_.attach_tool(*id);
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }
    bool detach_tool(int arm) {
        const auto id = arm_id(arm);
        if (!id) return false;
        try {
            rig_.detach_tool(*id);
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }
    bool set_joint_targets(int arm, double shoulder, double elbow, double wrist) {
        const auto id = arm_id(arm);
        if (!id) return false;
        try {
            rig_.command_joint_target_degrees(*id, ManipulatorJoint::Shoulder, shoulder);
            rig_.command_joint_target_degrees(*id, ManipulatorJoint::Elbow, elbow);
            rig_.command_joint_target_degrees(*id, ManipulatorJoint::Wrist, wrist);
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }
    [[nodiscard]] ArmInfo arm_info(int arm) const {
        ArmInfo out;
        const auto id = arm_id(arm);
        if (!id) return out;
        const ManipulatorArmState& s = rig_.arm(*id);
        out.deployed = s.is_deployed;
        out.deploying = s.is_deploying;
        out.stowing = s.is_stowing;
        out.tool_attached = s.tool_attached;
        out.accepts_joint_commands = arm_accepts_joint_commands(s.is_deployed, s.is_deploying, s.is_stowing);
        out.deployment_fraction = s.deployment_fraction;
        out.angles = s.angles;
        out.commanded = s.commanded_angles;
        return out;
    }

    // ------------------------------------------------------------------ Fix_It (#378 G02)
    // Turns Fix_It on. With seed_damaged_awakening, an undamaged probe first wakes at the Unreal actor's Gen-1
    // startup integrity (computation 18%, thermal 22%, sensors 35%, propulsion 40%).
    void enable_fix_it(bool seed_damaged_awakening) {
        fix_it_enabled_ = true;
        const auto& i = runtime_.component_integrity();
        if (seed_damaged_awakening && i.sensors >= 0.999 && i.propulsion >= 0.999 && i.computation >= 0.999 &&
            i.thermal >= 0.999) {
            runtime_.set_subsystem_integrity(PowerSubsystem::Computation, 0.18);
            runtime_.set_subsystem_integrity(PowerSubsystem::Thermal, 0.22);
            runtime_.set_subsystem_integrity(PowerSubsystem::Sensors, 0.35);
            runtime_.set_subsystem_integrity(PowerSubsystem::Propulsion, 0.40);
            fix_it_event("Fix_It active: Gen-1 startup integrity COMPUTATION 18%, THERMAL 22%, SENSORS 35%, PROPULSION 40%.");
        }
    }
    [[nodiscard]] ComponentIntegritySnapshot integrity() const { return runtime_.component_integrity(); }
    [[nodiscard]] double stored_energy_j() const { return runtime_.snapshot().stored_energy_j; }
    [[nodiscard]] FixItInfo fix_it_info() const {
        FixItInfo out;
        out.enabled = fix_it_enabled_;
        out.last_event = fix_it_last_event_;
        out.last_completed_material_consumed_kg = fix_it_last_completed_material_consumed_kg_;
        out.total_material_consumed_kg = fix_it_total_material_consumed_kg_;
        if (!fix_it_enabled_) return out;
        out.state = fix_it_running_ ? "repairing" : (fix_it_waiting_ ? "waiting" : "monitor");
        const auto& status = fix_it_repair_.status();
        const std::optional<FixItDecision>& d = fix_it_running_ ? status.decision : fix_it_last_decision_;
        if (d) {
            out.stage = stage_name(d->stage);
            out.subsystem = subsystem_name(d->subsystem);
            out.reason = d->reason;
            out.material_required_kg = d->material_required_kg;
            out.energy_required_j = d->energy_required_j;
            out.target_integrity = d->target_integrity;
        }
        if (fix_it_running_ && status.decision) {
            out.progress = std::clamp(status.elapsed_s / status.decision->time_required_s, 0.0, 1.0);
            out.material_consumed_kg = status.material_consumed_kg;
            out.energy_consumed_j = status.energy_consumed_j;
        }
        return out;
    }
    // Fix_It's event lines since the last call (started, completed with the materials consumed, interrupted).
    std::vector<std::string> drain_fix_it_events() {
        std::vector<std::string> out;
        out.swap(fix_it_events_);
        return out;
    }

    // World positions (simulation frame) of the arm's shoulder, elbow, wrist and tool tip, from the same forward
    // kinematics MiningSystem and the hull checks use, for drawing the arm. Empty when the arm id is invalid.
    [[nodiscard]] std::vector<Vector3d> arm_points(int arm) const {
        std::vector<Vector3d> out;
        const auto id = arm_id(arm);
        if (!id) return out;
        const ManipulatorArmState& s = rig_.arm(*id);
        const auto links = manipulator_arm_contact_samples(*id, s.deployment_fraction, s.angles);
        const auto tip = manipulator_tool_tip_contact_sample(*id, s.deployment_fraction, s.angles);
        const auto& snap = runtime_.snapshot();
        for (const Vector3d& local : {ManipulatorArmLayoutMeters::shoulder_pivot_local_m(*id), links.elbow.center_m,
                                      links.wrist.center_m, tip.center_m}) {
            out.push_back(contact_add(snap.position_m, rotate_local_contact_offset(local, snap.attitude_degrees)));
        }
        return out;
    }

    // The #375 staging solver: where the probe must stand, at its current attitude, for this arm's tool to sit just
    // off the deposit surface without the hull or arm touching anything. Same call as the Unreal adapter's.
    [[nodiscard]] StagingPlan plan_staging(int arm, const std::string& target_id) const {
        StagingPlan out;
        const auto id = arm_id(arm);
        const auto target = find_body(target_id);
        if (!id || !target) return out;
        const auto& s = runtime_.snapshot();
        const auto solution = mining_staging_solution(*id, *target, s.attitude_degrees, s.position_m,
                                                      ProbeCompoundCollisionEnvelope{}, runtime_.static_bodies());
        if (!solution) return out;
        out.has_solution = true;
        out.probe_position = solution->probe_position_m;
        out.angles = solution->angles;
        out.tool_gap_m = solution->tool_surface_gap_m;
        return out;
    }

    // Collision-free joint waypoints from the arm's current pose to the goal (empty optional: no path).
    [[nodiscard]] std::optional<std::vector<ManipulatorArmAngles>> plan_arm_path(int arm, double shoulder, double elbow,
                                                                                double wrist) const {
        const auto id = arm_id(arm);
        if (!id) return std::nullopt;
        try {
            const ManipulatorArmState& s = rig_.arm(*id);
            JointPathOptions options;
            options.frame_seconds = static_cast<double>(kFixedStepTicks) / SimulationClock::TicksPerSecond;
            return plan_collision_free_joint_path(*id, s.deployment_fraction, s.angles,
                                                  ManipulatorArmAngles{shoulder, elbow, wrist}, collision_guard(),
                                                  options);
        } catch (const std::exception&) {
            return std::nullopt;
        }
    }

    // Mines a world deposit from the probe's real pose with whichever ready arm is closest. MiningSystem judges
    // survey, reach, storage and the deposit; a fresh one is built per attempt as in ProbeMiningBridge.
    MineResult mine_deposit(const std::string& target_id) {
        MineResult out;
        const auto it = deposits_.find(target_id);
        if (it == deposits_.end()) {
            out.reason = "target has no registered resource deposit";
            return out;
        }
        WorldDeposit& d = it->second;
        MiningAttemptResult r;
        try {
            MiningSystem mining;
            ResourceDeposit current = d.deposit;
            if (const auto body = find_body(target_id)) current.body.center_m = body->center_m;
            mining.add_deposit(current);
            if (d.surveyed) mining.mark_surveyed(target_id);
            const auto& snap = runtime_.snapshot();
            r = mining.mine_once(target_id, rig_, ProbeWorldPose{snap.position_m, snap.attitude_degrees},
                                 snap.storage_used_kg, snap.storage_capacity_kg);
        } catch (const std::exception& e) {
            out.reason = std::string("rejected: ") + e.what();
            return out;
        }
        out.accepted = r.accepted;
        out.remaining_kg = r.remaining_deposit_kg;
        out.tool_gap_m = r.tool_surface_gap_m;
        out.arm = r.arm == ManipulatorArmId::Starboard ? 1 : 0;
        out.reason = r.detail;
        if (r.accepted) {
            runtime_.add_stored_material_kg(r.extracted_kg, r.material_id);
            d.deposit.remaining_kg = r.remaining_deposit_kg;
            d.extracted_kg += r.extracted_kg;
            out.extracted_kg = r.extracted_kg;
            out.material_id = r.material_id;
        }
        return out;
    }

    // Deploy the port arm, attach its tool and register a surveyed deposit just beyond the tool tip (the same
    // setup the native save/deposit regression test uses).
    bool setup_bootstrap_deposit(double remaining_kg) {
        if (!std::isfinite(remaining_kg) || remaining_kg < 0.0) return false;
        rig_ = ManipulatorRig(collision_guard());
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
        for (const auto& [id, d] : deposits_) {  // #378 G08: world deposits and their surveys
            data.world_deposits.push_back({id, d.deposit.material_id, d.deposit.display_name, d.deposit.remaining_kg,
                                           d.extracted_kg, d.deposit.extraction_kg_per_cycle, d.surveyed});
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
            ManipulatorRig new_rig = restore_manipulator_rig(data, collision_guard());
            const bool new_has_deposit =
                data.bootstrap_resource_surveyed && data.bootstrap_deposit_remaining_kg.has_value();
            const double new_remaining = new_has_deposit ? *data.bootstrap_deposit_remaining_kg : 0.0;
            const double new_extracted = data.bootstrap_extracted_kg.value_or(0.0);
            // World deposits come back from the save (the schema guarantees each names a saved body); a save
            // without them restores none, never keeping the current session's deposits or surveys.
            std::map<std::string, WorldDeposit> new_deposits;
            for (const WorldDepositSaveData& w : data.world_deposits) {
                WorldDeposit d;
                for (const StaticSphereBody& body : data.static_bodies) {
                    if (body.body_id == w.body_id) d.deposit.body = body;
                }
                d.deposit.material_id = w.material_id;
                d.deposit.display_name = w.display_name;
                d.deposit.remaining_kg = w.remaining_kg;
                d.deposit.extraction_kg_per_cycle = w.extraction_kg_per_cycle;
                d.extracted_kg = w.extracted_kg;
                d.surveyed = w.surveyed;
                new_deposits.emplace(w.body_id, d);
            }
            runtime_ = std::move(new_runtime);
            rig_ = std::move(new_rig);
            has_deposit_ = new_has_deposit;
            deposit_remaining_kg_ = new_remaining;
            extracted_kg_ = new_extracted;
            deposits_ = std::move(new_deposits);
            pending_contacts_ = 0;
            // In-progress Fix_It operations are intentionally not persisted yet. A load replaces
            // the authoritative world state, so no executor decision/progress from the old world
            // may run against the restored probe on the next fixed step.
            fix_it_running_ = false;
            fix_it_waiting_ = false;
            fix_it_repair_ = FixItRepairExecutor{};
            fix_it_last_completed_material_consumed_kg_ = 0.0;
            fix_it_total_material_consumed_kg_ = 0.0;
            fix_it_last_decision_.reset();
            fix_it_last_event_.clear();
            fix_it_events_.clear();
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }

private:
    // Hull self-collision plus the registered bodies around the probe, as the Unreal adapter builds it.
    [[nodiscard]] ManipulatorRig::SelfCollisionGuard collision_guard() const {
        return make_combined_collision_guard(
            make_hull_self_collision_guard(),
            make_environment_collision_guard(
                [this]() {
                    const auto& s = runtime_.snapshot();
                    return ProbeWorldPose{s.position_m, s.attitude_degrees};
                },
                [this]() -> const std::vector<StaticSphereBody>& { return runtime_.static_bodies(); }));
    }

    [[nodiscard]] static std::optional<PowerSubsystem> power_subsystem(int s) {
        switch (s) {
            case 0: return PowerSubsystem::Sensors;
            case 1: return PowerSubsystem::Propulsion;
            case 2: return PowerSubsystem::Computation;
            case 3: return PowerSubsystem::Thermal;
            default: return std::nullopt;
        }
    }

    [[nodiscard]] static std::optional<ManipulatorArmId> arm_id(int arm) {
        if (arm == 0) return ManipulatorArmId::Port;
        if (arm == 1) return ManipulatorArmId::Starboard;
        return std::nullopt;
    }

    [[nodiscard]] std::optional<StaticSphereBody> find_body(const std::string& id) const {
        for (const StaticSphereBody& body : runtime_.static_bodies()) {
            if (body.body_id == id) return body;
        }
        return std::nullopt;
    }

    static const char* subsystem_name(PowerSubsystem s) {
        switch (s) {
            case PowerSubsystem::Sensors: return "SENSORS";
            case PowerSubsystem::Propulsion: return "PROPULSION";
            case PowerSubsystem::Computation: return "COMPUTATION";
            case PowerSubsystem::Thermal: return "THERMAL";
        }
        return "UNKNOWN";
    }
    static const char* stage_name(FixItStage s) {
        switch (s) {
            case FixItStage::Repair: return "REPAIR";
            case FixItStage::Replacement: return "REPLACEMENT";
            case FixItStage::Upgrade: return "UPGRADE";
            case FixItStage::Redesign: return "REDESIGN";
            case FixItStage::Evolution: return "EVOLUTION";
        }
        return "UNKNOWN";
    }
    void fix_it_event(std::string line) {
        fix_it_last_event_ = line;
        fix_it_events_.push_back(std::move(line));
        if (fix_it_events_.size() > 64) fix_it_events_.erase(fix_it_events_.begin());
    }
    static std::string kg_breakdown(const std::map<std::string, double>& by_material) {
        std::string out;
        char buf[160];
        for (const auto& [id, kg] : by_material) {
            std::snprintf(buf, sizeof(buf), "%s%s %.2f kg", out.empty() ? "" : ", ", id.c_str(), kg);
            out += buf;
        }
        return out.empty() ? std::string("nothing") : out;
    }

    // FixItRuntimeActor::AdvanceFixIt / ReplanOrStart, on the fixed step.
    void advance_fix_it(double step_s) {
        char buf[320];
        if (fix_it_running_) {
            const double stored_before = runtime_.snapshot().storage_used_kg;
            fix_it_repair_.advance(runtime_, step_s);
            fix_it_total_material_consumed_kg_ += stored_before - runtime_.snapshot().storage_used_kg;
            const auto& status = fix_it_repair_.status();
            if (status.state == FixItExecutionState::Completed) {
                fix_it_last_completed_material_consumed_kg_ = status.material_consumed_kg;
                fix_it_running_ = false;
                fix_it_waiting_ = false;
                std::snprintf(buf, sizeof(buf), "%s restored to %.0f%%; consumed %.2f kg (%s) and %.0f J.",
                              subsystem_name(status.decision->subsystem), status.decision->target_integrity * 100.0,
                              status.material_consumed_kg, kg_breakdown(status.material_consumed_breakdown_kg).c_str(),
                              status.energy_consumed_j);
                fix_it_event(buf);
            } else if (status.state == FixItExecutionState::Interrupted) {
                fix_it_running_ = false;
                fix_it_waiting_ = true;
                fix_it_event(status.detail);
            }
            return;
        }
        const auto& snap = runtime_.snapshot();
        FixItResources resources;
        resources.material_kg = snap.storage_used_kg;
        resources.energy_j = snap.stored_energy_j;
        resources.available_time_s = std::numeric_limits<double>::max();
        fix_it_last_decision_ = FixItPlanner::plan_next(runtime_.component_integrity(), resources);
        if (!fix_it_last_decision_) {
            fix_it_waiting_ = false;
            return;
        }
        const FixItDecision& d = *fix_it_last_decision_;
        if (d.stage != FixItStage::Repair) {
            fix_it_waiting_ = d.stage == FixItStage::Replacement;  // fabrication is not online yet
            return;
        }
        if (snap.storage_used_kg + 1e-9 < d.material_required_kg || snap.stored_energy_j + 1e-6 < d.energy_required_j) {
            fix_it_waiting_ = true;
            return;
        }
        fix_it_repair_.start(d);
        fix_it_running_ = true;
        fix_it_waiting_ = false;
        std::snprintf(buf, sizeof(buf), "%s %.0f%% -> %.0f%% // need %.2f kg, %.0f J, %.1f s // %s",
                      subsystem_name(d.subsystem), d.integrity_before * 100.0, d.target_integrity * 100.0,
                      d.material_required_kg, d.energy_required_j, d.time_required_s, d.reason.c_str());
        fix_it_event(buf);
    }

    // Counts contacts for drain_contact_events and latches surveys from genuine ScanCompleted events (G05: a later
    // cancelled scan or a scan of another target never clears one). Manipulator events are not needed yet.
    void consume_events() {
        for (const DomainEvent& e : runtime_.drain_events()) {
            if (e.type == DomainEventType::Contact) ++pending_contacts_;
            for (auto& [id, d] : deposits_) {
                if (scan_event_completes_survey(e, id)) d.surveyed = true;
            }
        }
        (void)rig_.drain_events();
    }

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
    std::map<std::string, WorldDeposit> deposits_;
    int pending_contacts_{0};
    std::int64_t accumulator_ticks_{0};
    std::string last_scan_refusal_;
    bool fix_it_enabled_{false};
    bool fix_it_running_{false};
    bool fix_it_waiting_{false};
    FixItRepairExecutor fix_it_repair_{};
    double fix_it_last_completed_material_consumed_kg_{0.0};
    double fix_it_total_material_consumed_kg_{0.0};
    std::optional<FixItDecision> fix_it_last_decision_{};
    std::string fix_it_last_event_;
    std::vector<std::string> fix_it_events_;
};

}  // namespace everward::binding
