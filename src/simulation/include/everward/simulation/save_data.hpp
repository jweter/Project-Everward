#pragma once

// Deterministic, versioned save/load for the engine-independent simulation
// state, per docs/SAVE_FORMAT.md. This is a first-prototype slice: it
// covers exactly the state that exists today (the single canonical EV-0001
// probe's physical/energy/thermal/storage/scan state, component integrity,
// registered physical targets, an installed software policy, target
// selection, and both manipulator arms' deploy/joint/tool/grasp state). The
// many other top-level categories docs/SAVE_FORMAT.md anticipates
// (lineages, infrastructure, civilizations, ...) do not exist in the
// simulation yet and are therefore not represented here; adding them is a
// later, explicit schema version rather than something this slice should
// invent ahead of the mechanics that would give them meaning.
//
// Save data is untrusted input once it leaves this process (a player can
// hand-edit the human-inspectable JSON), so every read path below fails
// with a clear std::runtime_error on a missing/malformed/out-of-range
// field rather than silently substituting a default -- matching
// docs/SAVE_FORMAT.md's "unknown or unsupported ... fail clearly" rule and
// this codebase's existing fail-closed conventions.

#include "everward/simulation/impact_damage.hpp"
#include "everward/simulation/json_value.hpp"
#include "everward/simulation/manipulator.hpp"
#include "everward/simulation/software_policy.hpp"

#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace everward::simulation {

inline constexpr int kSaveFormatVersion = 1;

// A single probe's persisted state. Grouped separately from the top-level
// SaveGameV1 envelope so it can be captured from / restored onto a
// DamageAwareProbeRuntime without going through JSON when that is useful
// (e.g. a future in-memory "load checkpoint" path).
struct ProbeSaveData {
    ProbeStateSnapshot probe{};
    ComponentIntegritySnapshot integrity{};
    std::vector<StaticSphereBody> static_bodies{};
    std::optional<SoftwarePolicy> policy{};
    std::string selected_target_id{};
    // ManipulatorRig is not composed inside DamageAwareProbeRuntime (see
    // manipulator.hpp's header comment), so it is captured/restored here as
    // plain arm state rather than as a whole ManipulatorRig -- the rig's
    // SelfCollisionGuard is presentation-adjacent wiring the caller supplies
    // when reconstructing the rig, not persisted state.
    ManipulatorArmState port_manipulator_arm{};
    ManipulatorArmState starboard_manipulator_arm{};
};

struct SaveGameV1 {
    int save_version{kSaveFormatVersion};
    std::int64_t simulation_tick{0};
    std::vector<ProbeSaveData> probes{};
};

namespace detail {

[[nodiscard]] inline JsonValue vector3d_to_json(const Vector3d& value) {
    JsonValue object = JsonValue::make_object();
    object.set("x", JsonValue(value.x));
    object.set("y", JsonValue(value.y));
    object.set("z", JsonValue(value.z));
    return object;
}

[[nodiscard]] inline Vector3d vector3d_from_json(const JsonValue& value) {
    Vector3d out;
    out.x = value.require("x").as_double();
    out.y = value.require("y").as_double();
    out.z = value.require("z").as_double();
    return out;
}

// JsonValue numbers are doubles, which cannot represent every std::int64_t
// exactly above 2^53 (ticks are a microsecond-resolution counter, so a
// long-running campaign is not a purely theoretical way to reach that
// range). Tick fields are therefore encoded as decimal strings instead of
// JSON numbers so serialize -> parse round-trips them exactly regardless of
// magnitude.
[[nodiscard]] inline JsonValue int64_to_json(std::int64_t value) {
    return JsonValue(std::to_string(value));
}

[[nodiscard]] inline std::int64_t int64_from_json(const JsonValue& value) {
    const std::string& text = value.as_string();
    std::size_t consumed = 0;
    long long parsed = 0;
    try {
        parsed = std::stoll(text, &consumed);
    } catch (const std::exception&) {
        throw std::runtime_error("save data integer field is not a valid integer: " + text);
    }
    if (consumed != text.size()) {
        throw std::runtime_error("save data integer field is not a valid integer: " + text);
    }
    return static_cast<std::int64_t>(parsed);
}

[[nodiscard]] inline JsonValue attitude_to_json(const EulerAttitudeDegrees& value) {
    JsonValue object = JsonValue::make_object();
    object.set("yaw", JsonValue(value.yaw));
    object.set("pitch", JsonValue(value.pitch));
    object.set("roll", JsonValue(value.roll));
    return object;
}

[[nodiscard]] inline EulerAttitudeDegrees attitude_from_json(const JsonValue& value) {
    EulerAttitudeDegrees out;
    out.yaw = value.require("yaw").as_double();
    out.pitch = value.require("pitch").as_double();
    out.roll = value.require("roll").as_double();
    return out;
}

[[nodiscard]] inline std::string power_subsystem_to_string(PowerSubsystem subsystem) {
    switch (subsystem) {
        case PowerSubsystem::Sensors: return "sensors";
        case PowerSubsystem::Propulsion: return "propulsion";
        case PowerSubsystem::Computation: return "computation";
        case PowerSubsystem::Thermal: return "thermal";
    }
    throw std::invalid_argument("unknown PowerSubsystem value");
}

[[nodiscard]] inline PowerSubsystem power_subsystem_from_string(const std::string& value) {
    if (value == "sensors") return PowerSubsystem::Sensors;
    if (value == "propulsion") return PowerSubsystem::Propulsion;
    if (value == "computation") return PowerSubsystem::Computation;
    if (value == "thermal") return PowerSubsystem::Thermal;
    throw std::runtime_error("unknown power subsystem in save data: " + value);
}

[[nodiscard]] inline std::string policy_condition_kind_to_string(PolicyConditionKind kind) {
    switch (kind) {
        case PolicyConditionKind::EnergyFractionBelow: return "energy_fraction_below";
        case PolicyConditionKind::EnergyFractionAbove: return "energy_fraction_above";
        case PolicyConditionKind::TemperatureAboveKelvin: return "temperature_above_kelvin";
        case PolicyConditionKind::TemperatureBelowKelvin: return "temperature_below_kelvin";
    }
    throw std::invalid_argument("unknown PolicyConditionKind value");
}

[[nodiscard]] inline PolicyConditionKind policy_condition_kind_from_string(const std::string& value) {
    if (value == "energy_fraction_below") return PolicyConditionKind::EnergyFractionBelow;
    if (value == "energy_fraction_above") return PolicyConditionKind::EnergyFractionAbove;
    if (value == "temperature_above_kelvin") return PolicyConditionKind::TemperatureAboveKelvin;
    if (value == "temperature_below_kelvin") return PolicyConditionKind::TemperatureBelowKelvin;
    throw std::runtime_error("unknown policy condition kind in save data: " + value);
}

[[nodiscard]] inline std::string policy_action_kind_to_string(PolicyActionKind kind) {
    switch (kind) {
        case PolicyActionKind::SetPowerAllocation: return "set_power_allocation";
    }
    throw std::invalid_argument("unknown PolicyActionKind value");
}

[[nodiscard]] inline PolicyActionKind policy_action_kind_from_string(const std::string& value) {
    if (value == "set_power_allocation") return PolicyActionKind::SetPowerAllocation;
    throw std::runtime_error("unknown policy action kind in save data: " + value);
}

[[nodiscard]] inline JsonValue static_body_to_json(const StaticSphereBody& body) {
    JsonValue object = JsonValue::make_object();
    object.set("body_id", JsonValue(body.body_id));
    object.set("center_m", vector3d_to_json(body.center_m));
    object.set("radius_m", JsonValue(body.radius_m));
    return object;
}

[[nodiscard]] inline StaticSphereBody static_body_from_json(const JsonValue& value) {
    StaticSphereBody body;
    body.body_id = value.require("body_id").as_string();
    body.center_m = vector3d_from_json(value.require("center_m"));
    body.radius_m = value.require("radius_m").as_double();
    return body;
}

[[nodiscard]] inline JsonValue manipulator_arm_angles_to_json(const ManipulatorArmAngles& angles) {
    JsonValue object = JsonValue::make_object();
    object.set("shoulder_degrees", JsonValue(angles.shoulder_degrees));
    object.set("elbow_degrees", JsonValue(angles.elbow_degrees));
    object.set("wrist_degrees", JsonValue(angles.wrist_degrees));
    return object;
}

[[nodiscard]] inline ManipulatorArmAngles manipulator_arm_angles_from_json(const JsonValue& value) {
    ManipulatorArmAngles angles;
    angles.shoulder_degrees = value.require("shoulder_degrees").as_double();
    angles.elbow_degrees = value.require("elbow_degrees").as_double();
    angles.wrist_degrees = value.require("wrist_degrees").as_double();
    return angles;
}

[[nodiscard]] inline JsonValue manipulator_arm_state_to_json(const ManipulatorArmState& state) {
    JsonValue object = JsonValue::make_object();
    object.set("is_deployed", JsonValue(state.is_deployed));
    object.set("is_deploying", JsonValue(state.is_deploying));
    object.set("is_stowing", JsonValue(state.is_stowing));
    object.set("deployment_fraction", JsonValue(state.deployment_fraction));
    object.set("angles", manipulator_arm_angles_to_json(state.angles));
    object.set("commanded_angles", manipulator_arm_angles_to_json(state.commanded_angles));
    object.set("tool_attached", JsonValue(state.tool_attached));
    object.set("grasped_target_body_id", JsonValue(state.grasped_target_body_id));
    return object;
}

[[nodiscard]] inline ManipulatorArmState manipulator_arm_state_from_json(const JsonValue& value) {
    ManipulatorArmState state;
    state.is_deployed = value.require("is_deployed").as_bool();
    state.is_deploying = value.require("is_deploying").as_bool();
    state.is_stowing = value.require("is_stowing").as_bool();
    state.deployment_fraction = value.require("deployment_fraction").as_double();
    state.angles = manipulator_arm_angles_from_json(value.require("angles"));
    state.commanded_angles = manipulator_arm_angles_from_json(value.require("commanded_angles"));
    state.tool_attached = value.require("tool_attached").as_bool();
    state.grasped_target_body_id = value.require("grasped_target_body_id").as_string();
    return state;
}

[[nodiscard]] inline JsonValue policy_rule_to_json(const SoftwarePolicyRule& rule) {
    JsonValue object = JsonValue::make_object();
    object.set("id", JsonValue(rule.id));
    object.set("condition", JsonValue(policy_condition_kind_to_string(rule.condition)));
    object.set("threshold", JsonValue(rule.threshold));
    object.set("action", JsonValue(policy_action_kind_to_string(rule.action)));
    object.set("subsystem", JsonValue(power_subsystem_to_string(rule.subsystem)));
    object.set("action_watts", JsonValue(rule.action_watts));
    return object;
}

[[nodiscard]] inline SoftwarePolicyRule policy_rule_from_json(const JsonValue& value) {
    SoftwarePolicyRule rule;
    rule.id = value.require("id").as_string();
    rule.condition = policy_condition_kind_from_string(value.require("condition").as_string());
    rule.threshold = value.require("threshold").as_double();
    rule.action = policy_action_kind_from_string(value.require("action").as_string());
    rule.subsystem = power_subsystem_from_string(value.require("subsystem").as_string());
    rule.action_watts = value.require("action_watts").as_double();
    return rule;
}

[[nodiscard]] inline JsonValue policy_to_json(const SoftwarePolicy& policy) {
    JsonValue object = JsonValue::make_object();
    object.set("id", JsonValue(policy.id));
    object.set("enabled", JsonValue(policy.enabled));
    JsonValue rules = JsonValue::make_array();
    for (const auto& rule : policy.rules) {
        rules.push_back(policy_rule_to_json(rule));
    }
    object.set("rules", std::move(rules));
    return object;
}

[[nodiscard]] inline SoftwarePolicy policy_from_json(const JsonValue& value) {
    SoftwarePolicy policy;
    policy.id = value.require("id").as_string();
    policy.enabled = value.require("enabled").as_bool();
    for (const auto& rule_value : value.require("rules").as_array()) {
        policy.rules.push_back(policy_rule_from_json(rule_value));
    }
    return policy;
}

[[nodiscard]] inline JsonValue probe_state_to_json(const ProbeStateSnapshot& state) {
    JsonValue object = JsonValue::make_object();
    object.set("probe_id", JsonValue(state.probe_id));
    object.set("generation", JsonValue(static_cast<std::int64_t>(state.generation)));
    object.set("position_m", vector3d_to_json(state.position_m));
    object.set("velocity_mps", vector3d_to_json(state.velocity_mps));
    object.set("attitude_degrees", attitude_to_json(state.attitude_degrees));
    object.set("mass_kg", JsonValue(state.mass_kg));
    object.set("collision_envelope_radius_m", JsonValue(state.collision_envelope_radius_m));
    object.set("has_contact_history", JsonValue(state.has_contact_history));
    object.set("last_contact_body_id", JsonValue(state.last_contact_body_id));
    object.set("last_contact_point_m", vector3d_to_json(state.last_contact_point_m));
    object.set("last_contact_surface_normal", vector3d_to_json(state.last_contact_surface_normal));
    object.set(
        "last_contact_relative_velocity_mps",
        vector3d_to_json(state.last_contact_relative_velocity_mps));
    object.set("last_contact_normal_speed_mps", JsonValue(state.last_contact_normal_speed_mps));
    object.set("last_contact_tick", int64_to_json(state.last_contact_tick));
    object.set("stored_energy_j", JsonValue(state.stored_energy_j));
    object.set("energy_capacity_j", JsonValue(state.energy_capacity_j));
    object.set("energy_generation_w", JsonValue(state.energy_generation_w));
    object.set("is_energy_depleted", JsonValue(state.is_energy_depleted));
    object.set("temperature_k", JsonValue(state.temperature_k));
    object.set("thermal_capacity_j_per_k", JsonValue(state.thermal_capacity_j_per_k));
    object.set("ambient_temperature_k", JsonValue(state.ambient_temperature_k));
    object.set("passive_cooling_w_per_k", JsonValue(state.passive_cooling_w_per_k));
    object.set("max_operating_temperature_k", JsonValue(state.max_operating_temperature_k));
    object.set("is_overheated", JsonValue(state.is_overheated));
    object.set("storage_used_kg", JsonValue(state.storage_used_kg));
    object.set("storage_capacity_kg", JsonValue(state.storage_capacity_kg));
    object.set("can_scan", JsonValue(state.can_scan));
    object.set("can_thrust", JsonValue(state.can_thrust));
    object.set("sensors_operational", JsonValue(state.sensors_operational));
    object.set("propulsion_operational", JsonValue(state.propulsion_operational));
    object.set("computation_operational", JsonValue(state.computation_operational));
    object.set("thermal_operational", JsonValue(state.thermal_operational));
    object.set("is_scanning", JsonValue(state.is_scanning));
    object.set("active_scan_target_id", JsonValue(state.active_scan_target_id));
    object.set("scan_remaining_s", JsonValue(state.scan_remaining_s));
    object.set("power_capacity_w", JsonValue(state.power_capacity_w));
    object.set("power_allocated_sensors_w", JsonValue(state.power_allocated_sensors_w));
    object.set("power_allocated_propulsion_w", JsonValue(state.power_allocated_propulsion_w));
    object.set("power_allocated_computation_w", JsonValue(state.power_allocated_computation_w));
    object.set("power_allocated_thermal_w", JsonValue(state.power_allocated_thermal_w));
    return object;
}

// Fields intentionally not round-tripped: compound_collision_envelope is a
// fixed shape constant on ProbeStateSnapshot (not per-save state), and
// is_energy_depleted/is_overheated/can_scan/can_thrust are re-derived below
// rather than trusted verbatim from the save, since they are the very
// invariants restore_from_snapshot exists to protect against a hand-edited
// contradiction (e.g. can_scan=true with sensors_operational=false).
[[nodiscard]] inline ProbeStateSnapshot probe_state_from_json(const JsonValue& value) {
    ProbeStateSnapshot state;
    state.probe_id = value.require("probe_id").as_string();
    const std::int64_t generation = value.require("generation").as_int64();
    if (generation < 0 || generation > static_cast<std::int64_t>(std::numeric_limits<std::uint32_t>::max())) {
        throw std::runtime_error("generation out of range for a 32-bit generation counter");
    }
    state.generation = static_cast<std::uint32_t>(generation);
    state.position_m = vector3d_from_json(value.require("position_m"));
    state.velocity_mps = vector3d_from_json(value.require("velocity_mps"));
    state.attitude_degrees = attitude_from_json(value.require("attitude_degrees"));
    state.mass_kg = value.require("mass_kg").as_double();
    state.collision_envelope_radius_m = value.require("collision_envelope_radius_m").as_double();
    state.has_contact_history = value.require("has_contact_history").as_bool();
    state.last_contact_body_id = value.require("last_contact_body_id").as_string();
    state.last_contact_point_m = vector3d_from_json(value.require("last_contact_point_m"));
    state.last_contact_surface_normal = vector3d_from_json(value.require("last_contact_surface_normal"));
    state.last_contact_relative_velocity_mps =
        vector3d_from_json(value.require("last_contact_relative_velocity_mps"));
    state.last_contact_normal_speed_mps = value.require("last_contact_normal_speed_mps").as_double();
    state.last_contact_tick = int64_from_json(value.require("last_contact_tick"));
    state.stored_energy_j = value.require("stored_energy_j").as_double();
    state.energy_capacity_j = value.require("energy_capacity_j").as_double();
    state.energy_generation_w = value.require("energy_generation_w").as_double();
    state.temperature_k = value.require("temperature_k").as_double();
    state.thermal_capacity_j_per_k = value.require("thermal_capacity_j_per_k").as_double();
    state.ambient_temperature_k = value.require("ambient_temperature_k").as_double();
    state.passive_cooling_w_per_k = value.require("passive_cooling_w_per_k").as_double();
    state.max_operating_temperature_k = value.require("max_operating_temperature_k").as_double();
    state.storage_used_kg = value.require("storage_used_kg").as_double();
    state.storage_capacity_kg = value.require("storage_capacity_kg").as_double();
    state.sensors_operational = value.require("sensors_operational").as_bool();
    state.propulsion_operational = value.require("propulsion_operational").as_bool();
    state.computation_operational = value.require("computation_operational").as_bool();
    state.thermal_operational = value.require("thermal_operational").as_bool();
    state.is_scanning = value.require("is_scanning").as_bool();
    state.active_scan_target_id = value.require("active_scan_target_id").as_string();
    state.scan_remaining_s = value.require("scan_remaining_s").as_double();
    state.power_capacity_w = value.require("power_capacity_w").as_double();
    state.power_allocated_sensors_w = value.require("power_allocated_sensors_w").as_double();
    state.power_allocated_propulsion_w = value.require("power_allocated_propulsion_w").as_double();
    state.power_allocated_computation_w = value.require("power_allocated_computation_w").as_double();
    state.power_allocated_thermal_w = value.require("power_allocated_thermal_w").as_double();

    // Re-derive rather than trust verbatim (see comment above).
    state.is_energy_depleted = state.stored_energy_j <= 0.0;
    state.is_overheated = state.temperature_k >= state.max_operating_temperature_k;
    const bool locked_out = state.is_overheated || state.is_energy_depleted;
    state.can_scan = !locked_out && state.sensors_operational;
    state.can_thrust = !locked_out && state.propulsion_operational;
    return state;
}

[[nodiscard]] inline JsonValue component_integrity_to_json(const ComponentIntegritySnapshot& integrity) {
    JsonValue object = JsonValue::make_object();
    object.set("sensors", JsonValue(integrity.sensors));
    object.set("propulsion", JsonValue(integrity.propulsion));
    object.set("computation", JsonValue(integrity.computation));
    object.set("thermal", JsonValue(integrity.thermal));
    return object;
}

[[nodiscard]] inline ComponentIntegritySnapshot component_integrity_from_json(const JsonValue& value) {
    ComponentIntegritySnapshot integrity;
    integrity.sensors = value.require("sensors").as_double();
    integrity.propulsion = value.require("propulsion").as_double();
    integrity.computation = value.require("computation").as_double();
    integrity.thermal = value.require("thermal").as_double();
    return integrity;
}

} // namespace detail

[[nodiscard]] inline JsonValue probe_save_data_to_json(const ProbeSaveData& data) {
    JsonValue object = JsonValue::make_object();
    object.set("probe", detail::probe_state_to_json(data.probe));
    object.set("integrity", detail::component_integrity_to_json(data.integrity));
    JsonValue static_bodies = JsonValue::make_array();
    for (const auto& body : data.static_bodies) {
        static_bodies.push_back(detail::static_body_to_json(body));
    }
    object.set("static_bodies", std::move(static_bodies));
    object.set("policy", data.policy.has_value() ? detail::policy_to_json(*data.policy) : JsonValue());
    object.set("selected_target_id", JsonValue(data.selected_target_id));
    object.set("port_manipulator_arm", detail::manipulator_arm_state_to_json(data.port_manipulator_arm));
    object.set(
        "starboard_manipulator_arm",
        detail::manipulator_arm_state_to_json(data.starboard_manipulator_arm));
    return object;
}

[[nodiscard]] inline ProbeSaveData probe_save_data_from_json(const JsonValue& value) {
    ProbeSaveData data;
    data.probe = detail::probe_state_from_json(value.require("probe"));
    data.integrity = detail::component_integrity_from_json(value.require("integrity"));
    for (const auto& body_value : value.require("static_bodies").as_array()) {
        data.static_bodies.push_back(detail::static_body_from_json(body_value));
    }
    const JsonValue& policy_value = value.require("policy");
    if (!policy_value.is_null()) {
        data.policy = detail::policy_from_json(policy_value);
    }
    data.selected_target_id = value.require("selected_target_id").as_string();
    data.port_manipulator_arm =
        detail::manipulator_arm_state_from_json(value.require("port_manipulator_arm"));
    data.starboard_manipulator_arm =
        detail::manipulator_arm_state_from_json(value.require("starboard_manipulator_arm"));
    return data;
}

// Reads every field this schema version writes, then requires the parsed
// value to carry no other top-level keys. This makes an unrecognized
// top-level field (most plausibly a save written by a newer save_version
// this build does not know how to migrate) fail loudly at the version
// check below rather than being silently ignored.
[[nodiscard]] inline JsonValue save_game_to_json(const SaveGameV1& save) {
    JsonValue object = JsonValue::make_object();
    object.set("save_version", JsonValue(static_cast<std::int64_t>(save.save_version)));
    object.set("simulation_tick", detail::int64_to_json(save.simulation_tick));
    JsonValue probes = JsonValue::make_array();
    for (const auto& probe : save.probes) {
        probes.push_back(probe_save_data_to_json(probe));
    }
    object.set("probes", std::move(probes));
    return object;
}

[[nodiscard]] inline SaveGameV1 save_game_from_json(const JsonValue& value) {
    const std::int64_t save_version = value.require("save_version").as_int64();
    if (save_version != kSaveFormatVersion) {
        throw std::runtime_error(
            "unsupported save_version " + std::to_string(save_version) +
            " (this build supports version " + std::to_string(kSaveFormatVersion) + " only)");
    }
    SaveGameV1 save;
    save.save_version = static_cast<int>(save_version);
    save.simulation_tick = detail::int64_from_json(value.require("simulation_tick"));
    for (const auto& probe_value : value.require("probes").as_array()) {
        save.probes.push_back(probe_save_data_from_json(probe_value));
    }
    return save;
}

[[nodiscard]] inline std::string serialize_save_game(const SaveGameV1& save) {
    return save_game_to_json(save).dump();
}

[[nodiscard]] inline SaveGameV1 deserialize_save_game(const std::string& text) {
    return save_game_from_json(JsonValue::parse(text));
}

// Bridges the persisted schema to the live runtime. capture_probe_save_data
// reads only through DamageAwareProbeRuntime's (and, for the manipulator
// arms, ManipulatorRig's) existing public accessors; restore_probe_runtime
// and restore_manipulator_rig write only through SimulationCore/
// ImpactDamageModel/ManipulatorRig's dedicated restore_from_snapshot
// factories plus the same validated mutators (add_static_sphere_body,
// install_policy, select_target) live gameplay uses, so restored state
// carries nothing that could not have been reached by legitimate play.
//
// ManipulatorRig is not composed inside DamageAwareProbeRuntime today (see
// manipulator.hpp), so it is captured/restored as a separate optional
// parameter/function rather than folded into the runtime bridge above --
// matching how a caller (e.g. the Unreal adapter) already holds both
// objects side by side rather than one owning the other.
[[nodiscard]] inline ProbeSaveData capture_probe_save_data(
        const DamageAwareProbeRuntime& runtime,
        const ManipulatorRig& manipulator_rig = ManipulatorRig{}) {
    ProbeSaveData data;
    data.probe = runtime.snapshot();
    data.integrity = runtime.component_integrity();
    data.static_bodies = runtime.static_bodies();
    if (const SoftwarePolicy* active = runtime.active_policy(); active != nullptr) {
        data.policy = *active;
    }
    const TargetSelectionStatus selection = runtime.selected_target_status();
    if (selection.has_selection) {
        data.selected_target_id = selection.body_id;
    }
    data.port_manipulator_arm = manipulator_rig.arm(ManipulatorArmId::Port);
    data.starboard_manipulator_arm = manipulator_rig.arm(ManipulatorArmId::Starboard);
    return data;
}

[[nodiscard]] inline DamageAwareProbeRuntime restore_probe_runtime(
        const ProbeSaveData& data,
        std::int64_t simulation_tick) {
    SimulationCore core = SimulationCore::restore_from_snapshot(data.probe, simulation_tick);
    ImpactDamageModel damage = ImpactDamageModel::restore_from_snapshot(
        data.integrity, data.probe.has_contact_history, data.probe.last_contact_tick);
    DamageAwareProbeRuntime runtime =
        DamageAwareProbeRuntime::restore_from_snapshot(ProbeRuntime(std::move(core)), std::move(damage));

    for (const auto& body : data.static_bodies) {
        runtime.add_static_sphere_body(body);
    }
    if (data.policy.has_value()) {
        runtime.install_policy(*data.policy);
    }
    if (!data.selected_target_id.empty()) {
        runtime.select_target(data.selected_target_id);
    }
    return runtime;
}

// self_collision_guard is not persisted (see ProbeSaveData's comment): pass
// the same guard the caller would otherwise construct the rig with (e.g.
// manipulator_hull_contact.hpp's hull-aware guard), or omit it for a
// default-constructed rig with no guard, matching ManipulatorRig's own
// default-construction behavior.
[[nodiscard]] inline ManipulatorRig restore_manipulator_rig(
        const ProbeSaveData& data,
        ManipulatorRig::SelfCollisionGuard self_collision_guard = ManipulatorRig::SelfCollisionGuard{}) {
    return ManipulatorRig::restore_from_snapshot(
        data.port_manipulator_arm, data.starboard_manipulator_arm, std::move(self_collision_guard));
}

} // namespace everward::simulation
