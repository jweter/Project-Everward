#include "everward/simulation/save_data.hpp"

#undef NDEBUG
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>

namespace {

using everward::simulation::ComponentIntegritySnapshot;
using everward::simulation::DamageAwareProbeRuntime;
using everward::simulation::EulerAttitudeDegrees;
using everward::simulation::JsonValue;
using everward::simulation::ManipulatorArmId;
using everward::simulation::ManipulatorRig;
using everward::simulation::PolicyActionKind;
using everward::simulation::PolicyConditionKind;
using everward::simulation::PowerSubsystem;
using everward::simulation::ProbeSaveData;
using everward::simulation::ProbeStateSnapshot;
using everward::simulation::SaveGameV1;
using everward::simulation::SimulationCore;
using everward::simulation::SoftwarePolicy;
using everward::simulation::SoftwarePolicyRule;
using everward::simulation::StaticSphereBody;
using everward::simulation::Vector3d;
using everward::simulation::capture_probe_save_data;
using everward::simulation::deserialize_save_game;
using everward::simulation::restore_manipulator_rig;
using everward::simulation::restore_probe_runtime;
using everward::simulation::serialize_save_game;

bool nearly_equal(double a, double b, double eps = 1e-9) {
    return std::abs(a - b) <= eps;
}

bool vectors_equal(const Vector3d& a, const Vector3d& b) {
    return nearly_equal(a.x, b.x) && nearly_equal(a.y, b.y) && nearly_equal(a.z, b.z);
}

bool attitudes_equal(const EulerAttitudeDegrees& a, const EulerAttitudeDegrees& b) {
    return nearly_equal(a.yaw, b.yaw) && nearly_equal(a.pitch, b.pitch) && nearly_equal(a.roll, b.roll);
}

DamageAwareProbeRuntime build_representative_runtime() {
    DamageAwareProbeRuntime runtime = DamageAwareProbeRuntime::make_canonical_ev0001();

    runtime.add_static_sphere_body(StaticSphereBody{"rock", Vector3d{12.0, 3.0, -1.0}, 1.5});
    runtime.select_target("rock");

    runtime.set_velocity_mps(Vector3d{1.5, -0.5, 0.25});
    runtime.adjust_attitude_degrees(EulerAttitudeDegrees{30.0, 10.0, -15.0});

    runtime.allocate_power(PowerSubsystem::Propulsion, 100.0);
    runtime.allocate_power(PowerSubsystem::Thermal, 50.0);

    runtime.start_scan("rock", 10.0);
    runtime.advance_wall_ticks(2'000'000); // 2 s: scan progresses, energy/thermal integrate.

    runtime.set_subsystem_integrity(PowerSubsystem::Propulsion, 0.7);
    runtime.set_subsystem_integrity(PowerSubsystem::Thermal, 0.4);

    SoftwarePolicy policy;
    policy.id = "basic-survival-test";
    policy.enabled = true;
    SoftwarePolicyRule rule;
    rule.id = "shed-sensors-on-low-energy";
    rule.condition = PolicyConditionKind::EnergyFractionBelow;
    rule.threshold = 0.2;
    rule.action = PolicyActionKind::SetPowerAllocation;
    rule.subsystem = PowerSubsystem::Sensors;
    rule.action_watts = 0.0;
    policy.rules.push_back(rule);
    runtime.install_policy(policy);

    return runtime;
}

void test_round_trip_preserves_full_probe_state() {
    const DamageAwareProbeRuntime original = build_representative_runtime();

    const ProbeSaveData captured = capture_probe_save_data(original);
    const SaveGameV1 save{1, original.tick(), {captured}};
    const std::string json_text = serialize_save_game(save);

    const SaveGameV1 parsed = deserialize_save_game(json_text);
    assert(parsed.save_version == 1);
    assert(parsed.simulation_tick == original.tick());
    assert(parsed.probes.size() == 1);

    const DamageAwareProbeRuntime restored = restore_probe_runtime(parsed.probes[0], parsed.simulation_tick);

    const ProbeStateSnapshot& want = original.snapshot();
    const ProbeStateSnapshot& got = restored.snapshot();

    assert(restored.tick() == original.tick());
    assert(got.probe_id == want.probe_id);
    assert(got.generation == want.generation);
    assert(vectors_equal(got.position_m, want.position_m));
    assert(vectors_equal(got.velocity_mps, want.velocity_mps));
    assert(attitudes_equal(got.attitude_degrees, want.attitude_degrees));
    assert(nearly_equal(got.mass_kg, want.mass_kg));
    assert(got.has_contact_history == want.has_contact_history);
    assert(nearly_equal(got.stored_energy_j, want.stored_energy_j));
    assert(nearly_equal(got.energy_capacity_j, want.energy_capacity_j));
    assert(nearly_equal(got.energy_generation_w, want.energy_generation_w));
    assert(got.is_energy_depleted == want.is_energy_depleted);
    assert(nearly_equal(got.temperature_k, want.temperature_k));
    assert(got.is_overheated == want.is_overheated);
    assert(nearly_equal(got.storage_used_kg, want.storage_used_kg));
    assert(nearly_equal(got.storage_capacity_kg, want.storage_capacity_kg));
    assert(got.can_scan == want.can_scan);
    assert(got.can_thrust == want.can_thrust);
    assert(got.sensors_operational == want.sensors_operational);
    assert(got.propulsion_operational == want.propulsion_operational);
    assert(got.computation_operational == want.computation_operational);
    assert(got.thermal_operational == want.thermal_operational);
    assert(got.is_scanning == want.is_scanning);
    assert(got.active_scan_target_id == want.active_scan_target_id);
    assert(nearly_equal(got.scan_remaining_s, want.scan_remaining_s));
    assert(nearly_equal(got.power_capacity_w, want.power_capacity_w));
    assert(nearly_equal(got.power_allocated_sensors_w, want.power_allocated_sensors_w));
    assert(nearly_equal(got.power_allocated_propulsion_w, want.power_allocated_propulsion_w));
    assert(nearly_equal(got.power_allocated_computation_w, want.power_allocated_computation_w));
    assert(nearly_equal(got.power_allocated_thermal_w, want.power_allocated_thermal_w));

    const ComponentIntegritySnapshot& want_integrity = original.component_integrity();
    const ComponentIntegritySnapshot& got_integrity = restored.component_integrity();
    assert(nearly_equal(got_integrity.sensors, want_integrity.sensors));
    assert(nearly_equal(got_integrity.propulsion, want_integrity.propulsion));
    assert(nearly_equal(got_integrity.computation, want_integrity.computation));
    assert(nearly_equal(got_integrity.thermal, want_integrity.thermal));

    assert(restored.static_bodies().size() == original.static_bodies().size());
    assert(restored.static_bodies().at(0).body_id == "rock");
    assert(vectors_equal(restored.static_bodies().at(0).center_m, original.static_bodies().at(0).center_m));
    assert(nearly_equal(restored.static_bodies().at(0).radius_m, original.static_bodies().at(0).radius_m));

    const auto* want_policy = original.active_policy();
    const auto* got_policy = restored.active_policy();
    assert(want_policy != nullptr && got_policy != nullptr);
    assert(got_policy->id == want_policy->id);
    assert(got_policy->enabled == want_policy->enabled);
    assert(got_policy->rules.size() == want_policy->rules.size());
    assert(got_policy->rules.at(0).id == want_policy->rules.at(0).id);
    assert(got_policy->rules.at(0).condition == want_policy->rules.at(0).condition);
    assert(nearly_equal(got_policy->rules.at(0).threshold, want_policy->rules.at(0).threshold));
    assert(got_policy->rules.at(0).action == want_policy->rules.at(0).action);
    assert(got_policy->rules.at(0).subsystem == want_policy->rules.at(0).subsystem);
    assert(nearly_equal(got_policy->rules.at(0).action_watts, want_policy->rules.at(0).action_watts));

    const auto want_selection = original.selected_target_status();
    const auto got_selection = restored.selected_target_status();
    assert(want_selection.has_selection && got_selection.has_selection);
    assert(got_selection.body_id == want_selection.body_id);
}

void test_round_trip_with_no_policy_or_selection() {
    DamageAwareProbeRuntime runtime = DamageAwareProbeRuntime::make_canonical_ev0001();
    runtime.advance_wall_ticks(500'000);

    const ProbeSaveData captured = capture_probe_save_data(runtime);
    assert(!captured.policy.has_value());
    assert(captured.selected_target_id.empty());

    const std::string json_text = serialize_save_game(SaveGameV1{1, runtime.tick(), {captured}});
    const SaveGameV1 parsed = deserialize_save_game(json_text);
    const DamageAwareProbeRuntime restored = restore_probe_runtime(parsed.probes.at(0), parsed.simulation_tick);

    assert(restored.active_policy() == nullptr);
    assert(!restored.selected_target_status().has_selection);
}

void test_unsupported_save_version_fails_closed() {
    const std::string future_save = R"({"save_version": 2, "simulation_tick": 0, "probes": []})";
    bool threw = false;
    try {
        (void)deserialize_save_game(future_save);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    assert(threw);
}

void test_missing_required_field_fails_closed() {
    const std::string missing_tick = R"({"save_version": 1, "probes": []})";
    bool threw = false;
    try {
        (void)deserialize_save_game(missing_tick);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    assert(threw);
}

void test_malformed_json_fails_closed() {
    bool threw_unterminated = false;
    try {
        (void)JsonValue::parse("{\"save_version\": 1,");
    } catch (const std::runtime_error&) {
        threw_unterminated = true;
    }
    assert(threw_unterminated);

    bool threw_trailing = false;
    try {
        (void)JsonValue::parse("{}garbage");
    } catch (const std::runtime_error&) {
        threw_trailing = true;
    }
    assert(threw_trailing);
}

void test_large_tick_values_round_trip_losslessly() {
    // 2^53 + 1: the first positive integer a double cannot represent
    // exactly. Ticks are a microsecond-resolution int64 counter, so a
    // sufficiently long-running campaign can reach this range; the save
    // format must not silently round it to a neighboring even value.
    constexpr std::int64_t kLargeTick = 9'007'199'254'740'993LL;

    ProbeSaveData data = capture_probe_save_data(DamageAwareProbeRuntime::make_canonical_ev0001());
    data.probe.has_contact_history = true;
    data.probe.last_contact_body_id = "far-future-contact";
    data.probe.last_contact_point_m = Vector3d{1.0, 2.0, 3.0};
    data.probe.last_contact_surface_normal = Vector3d{0.0, 0.0, 1.0};
    data.probe.last_contact_relative_velocity_mps = Vector3d{0.1, 0.0, 0.0};
    data.probe.last_contact_normal_speed_mps = 0.1;
    data.probe.last_contact_tick = kLargeTick;

    const std::string json_text = serialize_save_game(SaveGameV1{1, kLargeTick, {data}});
    const SaveGameV1 parsed = deserialize_save_game(json_text);
    assert(parsed.simulation_tick == kLargeTick);
    assert(parsed.probes.at(0).probe.last_contact_tick == kLargeTick);

    const DamageAwareProbeRuntime restored = restore_probe_runtime(parsed.probes.at(0), parsed.simulation_tick);
    assert(restored.tick() == kLargeTick);
    assert(restored.snapshot().last_contact_tick == kLargeTick);
}

void test_generation_out_of_range_fails_closed() {
    const ProbeSaveData data = capture_probe_save_data(DamageAwareProbeRuntime::make_canonical_ev0001());
    std::string json_text = serialize_save_game(SaveGameV1{1, 0, {data}});

    const std::string needle = "\"generation\": 1";
    const std::size_t pos = json_text.find(needle);
    assert(pos != std::string::npos);
    json_text.replace(pos, needle.size(), "\"generation\": -1");

    bool threw = false;
    try {
        (void)deserialize_save_game(json_text);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    assert(threw);
}

void test_restored_contact_history_is_not_reassessed() {
    // Reproduces a real damaging contact (well above the 25 kJ light-impact
    // threshold at the canonical 2500 kg mass) via saved state, rather than
    // through the live collision solver, since this is a save/load-only
    // scenario: after loading, the persisted contact must already read as
    // assessed so it is not charged against integrity a second time.
    ProbeSaveData data = capture_probe_save_data(DamageAwareProbeRuntime::make_canonical_ev0001());
    data.probe.has_contact_history = true;
    data.probe.last_contact_body_id = "asteroid";
    data.probe.last_contact_point_m = Vector3d{5.0, 0.0, 0.0};
    data.probe.last_contact_surface_normal = Vector3d{1.0, 0.0, 0.0};
    data.probe.last_contact_relative_velocity_mps = Vector3d{-10.0, 0.0, 0.0};
    data.probe.last_contact_normal_speed_mps = 10.0;
    data.probe.last_contact_tick = 500;
    data.integrity = ComponentIntegritySnapshot{0.8, 0.8, 0.8, 0.8};

    DamageAwareProbeRuntime restored = restore_probe_runtime(data, 500);
    assert(nearly_equal(restored.component_integrity().sensors, 0.8));
    assert(nearly_equal(restored.component_integrity().propulsion, 0.8));
    assert(nearly_equal(restored.component_integrity().computation, 0.8));
    assert(nearly_equal(restored.component_integrity().thermal, 0.8));

    restored.advance_wall_ticks(0);

    // If the assessment watermark were not restored, whichever subsystem
    // the contact normal maps to would have dropped below 0.8 here.
    assert(nearly_equal(restored.component_integrity().sensors, 0.8));
    assert(nearly_equal(restored.component_integrity().propulsion, 0.8));
    assert(nearly_equal(restored.component_integrity().computation, 0.8));
    assert(nearly_equal(restored.component_integrity().thermal, 0.8));
    assert(restored.last_impact().has_value() == false);
}

void test_manipulator_arm_state_round_trips() {
    DamageAwareProbeRuntime runtime = DamageAwareProbeRuntime::make_canonical_ev0001();

    ManipulatorRig rig;
    rig.begin_deploy(ManipulatorArmId::Port);
    rig.advance(ManipulatorRig::kDeployStowDurationS);
    rig.command_joint_target_degrees(ManipulatorArmId::Port, everward::simulation::ManipulatorJoint::Elbow, 45.0);
    rig.advance(1.0);
    rig.attach_tool(ManipulatorArmId::Port);
    rig.begin_grasp(ManipulatorArmId::Port, "rock");

    const ProbeSaveData data = capture_probe_save_data(runtime, rig);
    const std::string json_text = serialize_save_game(SaveGameV1{1, runtime.tick(), {data}});
    const SaveGameV1 parsed = deserialize_save_game(json_text);

    const ManipulatorRig restored_rig = restore_manipulator_rig(parsed.probes.at(0));
    const auto& port = restored_rig.arm(ManipulatorArmId::Port);
    assert(port.is_deployed);
    assert(!port.is_deploying && !port.is_stowing);
    assert(nearly_equal(port.deployment_fraction, 1.0));
    assert(port.tool_attached);
    assert(port.grasped_target_body_id == "rock");
    assert(nearly_equal(port.angles.elbow_degrees, rig.arm(ManipulatorArmId::Port).angles.elbow_degrees));

    const auto& starboard = restored_rig.arm(ManipulatorArmId::Starboard);
    assert(!starboard.is_deployed);
    assert(nearly_equal(starboard.deployment_fraction, 0.0));
    assert(!starboard.tool_attached);
    assert(starboard.grasped_target_body_id.empty());
}

void test_manipulator_arm_out_of_range_angle_fails_closed() {
    const ProbeSaveData data = capture_probe_save_data(DamageAwareProbeRuntime::make_canonical_ev0001());
    std::string json_text = serialize_save_game(SaveGameV1{1, 0, {data}});

    const std::string needle = "\"shoulder_degrees\": 0";
    const std::size_t pos = json_text.find(needle);
    assert(pos != std::string::npos);
    json_text.replace(pos, needle.size(), "\"shoulder_degrees\": 999");

    const SaveGameV1 parsed = deserialize_save_game(json_text);
    bool threw = false;
    try {
        (void)restore_manipulator_rig(parsed.probes.at(0));
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    assert(threw);
}

void test_restore_rejects_inconsistent_snapshot() {
    ProbeStateSnapshot bad_mass = DamageAwareProbeRuntime::make_canonical_ev0001().snapshot();
    bad_mass.mass_kg = -1.0;
    bool threw_mass = false;
    try {
        (void)SimulationCore::restore_from_snapshot(bad_mass, 0);
    } catch (const std::invalid_argument&) {
        threw_mass = true;
    }
    assert(threw_mass);

    ProbeStateSnapshot bad_energy_generation = DamageAwareProbeRuntime::make_canonical_ev0001().snapshot();
    bad_energy_generation.energy_generation_w = -1.0;
    bool threw_energy_generation = false;
    try {
        (void)SimulationCore::restore_from_snapshot(bad_energy_generation, 0);
    } catch (const std::invalid_argument&) {
        threw_energy_generation = true;
    }
    assert(threw_energy_generation);

    ProbeStateSnapshot bad_thermal_capacity = DamageAwareProbeRuntime::make_canonical_ev0001().snapshot();
    bad_thermal_capacity.thermal_capacity_j_per_k = 0.0;
    bool threw_thermal_capacity = false;
    try {
        (void)SimulationCore::restore_from_snapshot(bad_thermal_capacity, 0);
    } catch (const std::invalid_argument&) {
        threw_thermal_capacity = true;
    }
    assert(threw_thermal_capacity);

    ProbeStateSnapshot bad_cooling = DamageAwareProbeRuntime::make_canonical_ev0001().snapshot();
    bad_cooling.passive_cooling_w_per_k = -1.0;
    bool threw_cooling = false;
    try {
        (void)SimulationCore::restore_from_snapshot(bad_cooling, 0);
    } catch (const std::invalid_argument&) {
        threw_cooling = true;
    }
    assert(threw_cooling);

    ProbeStateSnapshot bad_max_temperature = DamageAwareProbeRuntime::make_canonical_ev0001().snapshot();
    bad_max_temperature.max_operating_temperature_k = 0.0;
    bool threw_max_temperature = false;
    try {
        (void)SimulationCore::restore_from_snapshot(bad_max_temperature, 0);
    } catch (const std::invalid_argument&) {
        threw_max_temperature = true;
    }
    assert(threw_max_temperature);

    ProbeStateSnapshot bad_scan = DamageAwareProbeRuntime::make_canonical_ev0001().snapshot();
    bad_scan.is_scanning = true;
    bad_scan.active_scan_target_id.clear();
    bool threw_scan = false;
    try {
        (void)SimulationCore::restore_from_snapshot(bad_scan, 0);
    } catch (const std::invalid_argument&) {
        threw_scan = true;
    }
    assert(threw_scan);

    bool threw_negative_tick = false;
    try {
        (void)SimulationCore::restore_from_snapshot(
            DamageAwareProbeRuntime::make_canonical_ev0001().snapshot(), -1);
    } catch (const std::invalid_argument&) {
        threw_negative_tick = true;
    }
    assert(threw_negative_tick);
}

} // namespace

int main() {
    test_round_trip_preserves_full_probe_state();
    test_round_trip_with_no_policy_or_selection();
    test_unsupported_save_version_fails_closed();
    test_missing_required_field_fails_closed();
    test_malformed_json_fails_closed();
    test_large_tick_values_round_trip_losslessly();
    test_generation_out_of_range_fails_closed();
    test_restored_contact_history_is_not_reassessed();
    test_manipulator_arm_state_round_trips();
    test_manipulator_arm_out_of_range_angle_fails_closed();
    test_restore_rejects_inconsistent_snapshot();

    std::puts("save_data_tests: all tests passed");
    return 0;
}
