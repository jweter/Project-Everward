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
    test_restore_rejects_inconsistent_snapshot();

    std::puts("save_data_tests: all tests passed");
    return 0;
}
