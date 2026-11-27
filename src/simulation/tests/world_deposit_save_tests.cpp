// Issue #378 G08 (Godot path G3): world deposits -- remaining and extracted mass, yield and the survey earned by a
// genuine ScanCompleted event -- survive a save/load round trip, older saves without the field still load, and a
// save whose deposits do not fit its bodies is rejected instead of resuming an inconsistent world.
#include "everward/simulation/save_data.hpp"

#undef NDEBUG
#include <cassert>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace everward::simulation;

namespace {

SaveGameV1 make_save() {
    DamageAwareProbeRuntime runtime = DamageAwareProbeRuntime::make_canonical_ev0001();
    runtime.add_static_sphere_body({"ore-1", {40.0, 0.0, 0.0}, 3.0, "iron_bearing_silicate_regolith"});
    runtime.add_static_sphere_body({"ore-2", {-30.0, 12.0, 0.0}, 2.0, "nickel_regolith"});
    runtime.add_stored_material_kg(15.0, "iron_bearing_silicate_regolith");
    ManipulatorRig rig;
    SaveGameV1 save;
    save.simulation_tick = runtime.tick();
    ProbeSaveData data = capture_probe_save_data(runtime, rig);
    data.world_deposits.push_back({"ore-1", "iron_bearing_silicate_regolith", "Iron-bearing regolith", 105.0, 15.0, 5.0, true});
    data.world_deposits.push_back({"ore-2", "nickel_regolith", "Nickel regolith", 80.0, 0.0, 4.0, false});
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

std::string replaced(std::string json, const std::string& from, const std::string& to) {
    const std::size_t at = json.find(from);
    assert(at != std::string::npos);
    json.replace(at, from.size(), to);
    return json;
}

} // namespace

int main() {
    const std::string json = serialize_save_game(make_save());

    // Round trip: every field of both deposits comes back exactly, in order.
    {
        const ProbeSaveData data = deserialize_save_game(json).probes.at(0);
        assert(data.world_deposits.size() == 2);
        const WorldDepositSaveData& a = data.world_deposits[0];
        assert(a.body_id == "ore-1" && a.material_id == "iron_bearing_silicate_regolith");
        assert(a.display_name == "Iron-bearing regolith");
        assert(std::fabs(a.remaining_kg - 105.0) < 1e-12 && std::fabs(a.extracted_kg - 15.0) < 1e-12);
        assert(std::fabs(a.extraction_kg_per_cycle - 5.0) < 1e-12 && a.surveyed);
        const WorldDepositSaveData& b = data.world_deposits[1];
        assert(b.body_id == "ore-2" && !b.surveyed && std::fabs(b.remaining_kg - 80.0) < 1e-12);
        assert(std::fabs(b.extraction_kg_per_cycle - 4.0) < 1e-12);
        // the deposit and the inventory still tell one story
        assert(std::fabs(data.probe.material_inventory_kg.at("iron_bearing_silicate_regolith") - a.extracted_kg) < 1e-12);
        // and serializing the loaded save again is byte-identical
        SaveGameV1 again = deserialize_save_game(json);
        assert(serialize_save_game(again) == json);
    }

    // An older save without the field loads with no world deposits.
    {
        const std::size_t at = json.find("\"world_deposits\"");
        assert(at != std::string::npos);
        const std::size_t end = json.find(']', at);
        const std::size_t comma = json.rfind(',', at);  // the separator before the member goes with it
        assert(comma != std::string::npos);
        std::string older = json;
        older.erase(comma, end - comma + 1);
        assert(older.find("world_deposits") == std::string::npos);
        const SaveGameV1 loaded = deserialize_save_game(older);
        assert(loaded.probes.at(0).world_deposits.empty());
    }

    // Fail closed: invalid amounts, a missing body, a duplicate id, empty ids and wrong types are rejected.
    assert(throws_on_load(replaced(json, "\"remaining_kg\": 105", "\"remaining_kg\": -1")));
    assert(throws_on_load(replaced(json, "\"extracted_kg\": 15", "\"extracted_kg\": -0.5")));
    assert(throws_on_load(replaced(json, "\"extraction_kg_per_cycle\": 4", "\"extraction_kg_per_cycle\": 0")));
    assert(throws_on_load(replaced(json, "\"body_id\": \"ore-2\"", "\"body_id\": \"ghost\"")));
    assert(throws_on_load(replaced(json, "\"body_id\": \"ore-2\"", "\"body_id\": \"ore-1\"")));
    assert(throws_on_load(replaced(json, "\"body_id\": \"ore-2\"", "\"body_id\": \"\"")));
    assert(throws_on_load(replaced(json, "\"surveyed\": false", "\"surveyed\": \"no\"")));
    assert(throws_on_load(replaced(json, "\"remaining_kg\": 105", "\"remaining_kg\": \"lots\"")));

    std::cout << "world_deposit_save_tests passed\n";
    return 0;
}
