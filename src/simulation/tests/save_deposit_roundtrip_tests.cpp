// Issue #378 G08: the bootstrap deposit's remaining/extracted mass is persisted, so a save/load pair
// cannot desync the deposit from the restored inventory. These tests model what the Unreal adapter's
// mining bridge does (mine, route mass into the inventory, keep remaining/extracted counters) without
// Unreal, and prove the round trip, the older-save path and the strict validation.
#include "everward/simulation/mining.hpp"
#include "everward/simulation/save_data.hpp"

#undef NDEBUG
#include <cassert>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace everward::simulation;

namespace {

constexpr double kStartingDepositKg = 250.0;

struct MiningSession {
    DamageAwareProbeRuntime runtime = DamageAwareProbeRuntime::make_canonical_ev0001();
    ManipulatorRig rig;
    MiningSystem mining;
    double deposit_remaining_kg = kStartingDepositKg;  // adapter-side counters, like ProbeMiningBridge
    double extracted_kg = 0.0;
    std::string target = "bootstrap-rock";
};

void setup_deposit(MiningSession& session, double remaining_kg) {
    session.rig.begin_deploy(ManipulatorArmId::Port);
    session.rig.advance(ManipulatorRig::kDeployStowDurationS);
    session.rig.attach_tool(ManipulatorArmId::Port);
    const ManipulatorArmState& arm = session.rig.arm(ManipulatorArmId::Port);
    const ManipulatorArmSample tip =
        manipulator_tool_tip_contact_sample(ManipulatorArmId::Port, arm.deployment_fraction, arm.angles);
    ResourceDeposit deposit;
    deposit.body.body_id = session.target;
    deposit.body.radius_m = 0.50;
    deposit.body.center_m = {tip.center_m.x + deposit.body.radius_m + tip.radius_m + 0.20, tip.center_m.y,
                             tip.center_m.z};
    deposit.material_id = "iron_regolith";
    deposit.display_name = "Iron-bearing regolith";
    deposit.remaining_kg = remaining_kg;
    deposit.extraction_kg_per_cycle = 5.0;
    session.mining = MiningSystem{};
    session.mining.add_deposit(deposit);
    session.mining.mark_surveyed(session.target);
}

void mine_cycles(MiningSession& session, int cycles) {
    for (int i = 0; i < cycles; ++i) {
        const MiningAttemptResult r = session.mining.mine_once(
            session.target, session.rig, ProbeWorldPose{}, session.runtime.snapshot().storage_used_kg,
            session.runtime.snapshot().storage_capacity_kg);
        assert(r.accepted);
        session.runtime.add_stored_material_kg(r.extracted_kg, r.material_id);
        session.extracted_kg += r.extracted_kg;
        session.deposit_remaining_kg = r.remaining_deposit_kg;
    }
}

SaveGameV1 capture(const MiningSession& session, bool record_deposit) {
    SaveGameV1 save;
    save.simulation_tick = session.runtime.tick();
    ProbeSaveData data = capture_probe_save_data(session.runtime, session.rig);
    data.bootstrap_resource_surveyed = true;
    if (record_deposit) {
        data.bootstrap_deposit_remaining_kg = session.deposit_remaining_kg;
        data.bootstrap_extracted_kg = session.extracted_kg;
    }
    save.probes.push_back(data);
    return save;
}

bool throws_on_load(const std::string& json) {
    try {
        (void)deserialize_save_game(json);
    } catch (const std::exception&) {
        return true;
    }
    return false;
}

std::string with_field(std::string json, const std::string& replacement_for_null_field) {
    // Replaces the serialized `"bootstrap_deposit_remaining_kg":null` (absent value) with a chosen JSON fragment.
    const std::string key = "\"bootstrap_deposit_remaining_kg\": null";
    const std::size_t at = json.find(key);
    assert(at != std::string::npos);
    json.replace(at, key.size(), replacement_for_null_field);
    return json;
}

} // namespace

int main() {
    // The round trip preserves both counters exactly and the restored inventory agrees with them.
    {
        MiningSession session;
        setup_deposit(session, kStartingDepositKg);
        mine_cycles(session, 6);  // 30 kg
        assert(std::fabs(session.extracted_kg - 30.0) < 1e-9);
        assert(std::fabs(session.deposit_remaining_kg - 220.0) < 1e-9);

        const std::string json = serialize_save_game(capture(session, true));
        const SaveGameV1 loaded = deserialize_save_game(json);
        const ProbeSaveData& data = loaded.probes.at(0);
        assert(data.bootstrap_deposit_remaining_kg.has_value() && data.bootstrap_extracted_kg.has_value());
        assert(std::fabs(*data.bootstrap_deposit_remaining_kg - 220.0) < 1e-9);
        assert(std::fabs(*data.bootstrap_extracted_kg - 30.0) < 1e-9);
        // The deposit and the inventory tell one story: mass extracted equals mass now stored.
        assert(std::fabs(data.probe.material_inventory_kg.at("iron_regolith") - *data.bootstrap_extracted_kg) < 1e-9);
        assert(std::fabs(*data.bootstrap_deposit_remaining_kg + *data.bootstrap_extracted_kg - kStartingDepositKg) <
               1e-9);
    }

    // The defect this fixes, shown both ways. Mining that continues after a load picks up from the
    // SAVED remaining mass; ignoring the saved value (the old behaviour) hands back the full deposit.
    {
        MiningSession first;
        setup_deposit(first, kStartingDepositKg);
        mine_cycles(first, 4);  // 20 kg
        const SaveGameV1 saved = capture(first, true);
        mine_cycles(first, 4);  // 20 kg more after the save, then the player loads the earlier save

        // Load: inventory comes back at 20 kg; the deposit must come back at 230 kg, not stay at 210.
        const ProbeSaveData data = deserialize_save_game(serialize_save_game(saved)).probes.at(0);
        MiningSession restored;
        restored.runtime = DamageAwareProbeRuntime(restore_probe_runtime(data, saved.simulation_tick));
        restored.deposit_remaining_kg = *data.bootstrap_deposit_remaining_kg;
        restored.extracted_kg = *data.bootstrap_extracted_kg;
        setup_deposit(restored, restored.deposit_remaining_kg);
        assert(std::fabs(restored.deposit_remaining_kg - 230.0) < 1e-9);
        assert(std::fabs(restored.runtime.snapshot().material_inventory_kg.at("iron_regolith") - 20.0) < 1e-9);

        mine_cycles(restored, 2);  // 10 kg more
        assert(std::fabs(restored.deposit_remaining_kg - 220.0) < 1e-9);
        assert(std::fabs(restored.extracted_kg - 30.0) < 1e-9);
        assert(std::fabs(restored.runtime.snapshot().material_inventory_kg.at("iron_regolith") - 30.0) < 1e-9);

        // Old behaviour for contrast: not recorded, so a fresh session re-registered the deposit full.
        MiningSession legacy;
        legacy.runtime = DamageAwareProbeRuntime(restore_probe_runtime(data, saved.simulation_tick));
        setup_deposit(legacy, kStartingDepositKg);  // full deposit again
        mine_cycles(legacy, 2);
        const double stored = legacy.runtime.snapshot().material_inventory_kg.at("iron_regolith");
        // 20 kg already stored + 10 kg mined = 30 kg stored, yet the deposit claims only 10 kg gone.
        assert(std::fabs(stored - 30.0) < 1e-9);
        assert(std::fabs(legacy.deposit_remaining_kg - 240.0) < 1e-9);  // desynced: 240 remaining, 30 stored
    }

    // A save captured without the new fields (older saves, or a caller that does not record them) reads
    // back as "not recorded" for both, through the field being absent or null.
    {
        MiningSession session;
        setup_deposit(session, kStartingDepositKg);
        const std::string json = serialize_save_game(capture(session, false));
        const ProbeSaveData none = deserialize_save_game(json).probes.at(0);
        assert(!none.bootstrap_deposit_remaining_kg.has_value() && !none.bootstrap_extracted_kg.has_value());

        // Absent entirely (as in a save written before the fields existed): rename the keys so the
        // loader no longer sees them.
        std::string stripped = json;
        for (const std::string key : {"bootstrap_deposit_remaining_kg", "bootstrap_extracted_kg"}) {
            const std::size_t at = stripped.find(key);
            assert(at != std::string::npos);
            stripped.replace(at, key.size(), "legacy_unrelated_" + std::string(key.size() - 17, 'x'));
        }
        const ProbeSaveData absent = deserialize_save_game(stripped).probes.at(0);
        assert(!absent.bootstrap_deposit_remaining_kg.has_value() && !absent.bootstrap_extracted_kg.has_value());
    }

    // Strict validation: a recorded value must be a finite, non-negative number.
    {
        MiningSession session;
        setup_deposit(session, kStartingDepositKg);
        const std::string json = serialize_save_game(capture(session, false));
        assert(!throws_on_load(with_field(json, "\"bootstrap_deposit_remaining_kg\": 12.5")));
        assert(throws_on_load(with_field(json, "\"bootstrap_deposit_remaining_kg\": -1.0")));
        assert(throws_on_load(with_field(json, "\"bootstrap_deposit_remaining_kg\": \"lots\"")));
        assert(throws_on_load(with_field(json, "\"bootstrap_deposit_remaining_kg\": true")));
    }

    // A fully mined-out deposit (0 kg remaining) is a legitimate value and survives the round trip.
    {
        MiningSession session;
        setup_deposit(session, kStartingDepositKg);
        session.deposit_remaining_kg = 0.0;
        session.extracted_kg = kStartingDepositKg;
        const ProbeSaveData data = deserialize_save_game(serialize_save_game(capture(session, true))).probes.at(0);
        assert(data.bootstrap_deposit_remaining_kg.has_value() && *data.bootstrap_deposit_remaining_kg == 0.0);
        assert(*data.bootstrap_extracted_kg == kStartingDepositKg);
    }

    std::cout << "save_deposit_roundtrip_tests passed\n";
    return 0;
}
