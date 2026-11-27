#include "everward_sim.h"

#include <godot_cpp/core/class_db.hpp>

namespace godot {

void EverwardSim::_bind_methods() {
    ClassDB::bind_method(D_METHOD("reset"), &EverwardSim::reset);
    ClassDB::bind_method(D_METHOD("advance_ticks", "wall_ticks"), &EverwardSim::advance_ticks);
    ClassDB::bind_method(D_METHOD("get_tick"), &EverwardSim::get_tick);
    ClassDB::bind_method(D_METHOD("set_velocity", "x", "y", "z"), &EverwardSim::set_velocity);
    ClassDB::bind_method(D_METHOD("get_position"), &EverwardSim::get_position);
    ClassDB::bind_method(D_METHOD("setup_bootstrap_deposit", "remaining_kg"), &EverwardSim::setup_bootstrap_deposit);
    ClassDB::bind_method(D_METHOD("mine_once"), &EverwardSim::mine_once);
    ClassDB::bind_method(D_METHOD("get_inventory_kg", "material"), &EverwardSim::get_inventory_kg);
    ClassDB::bind_method(D_METHOD("get_extracted_kg"), &EverwardSim::get_extracted_kg);
    ClassDB::bind_method(D_METHOD("get_deposit_remaining_kg"), &EverwardSim::get_deposit_remaining_kg);
    ClassDB::bind_method(D_METHOD("get_storage_used_kg"), &EverwardSim::get_storage_used_kg);
    ClassDB::bind_method(D_METHOD("get_storage_capacity_kg"), &EverwardSim::get_storage_capacity_kg);
    ClassDB::bind_method(D_METHOD("has_deposit"), &EverwardSim::has_deposit);
    ClassDB::bind_method(D_METHOD("save_json"), &EverwardSim::save_json);
    ClassDB::bind_method(D_METHOD("load_json", "json"), &EverwardSim::load_json);
}

void EverwardSim::reset() { core_.reset(); }
void EverwardSim::advance_ticks(int64_t wall_ticks) { core_.advance_ticks(wall_ticks); }
int64_t EverwardSim::get_tick() const { return core_.tick(); }
void EverwardSim::set_velocity(double x, double y, double z) { core_.set_velocity(x, y, z); }

PackedFloat64Array EverwardSim::get_position() const {
    const auto p = core_.position();
    PackedFloat64Array out;
    out.push_back(p.x);
    out.push_back(p.y);
    out.push_back(p.z);
    return out;
}

void EverwardSim::setup_bootstrap_deposit(double remaining_kg) { core_.setup_bootstrap_deposit(remaining_kg); }

Dictionary EverwardSim::mine_once() {
    const auto r = core_.mine_once();
    Dictionary d;
    d["accepted"] = r.accepted;
    d["extracted_kg"] = r.extracted_kg;
    d["remaining_kg"] = r.remaining_kg;
    d["material_id"] = String::utf8(r.material_id.c_str());
    d["reason"] = String::utf8(r.reason.c_str());
    return d;
}

double EverwardSim::get_inventory_kg(const String& material) const {
    return core_.inventory_kg(std::string(material.utf8().get_data()));
}
double EverwardSim::get_extracted_kg() const { return core_.extracted_kg(); }
double EverwardSim::get_deposit_remaining_kg() const { return core_.deposit_remaining_kg(); }
double EverwardSim::get_storage_used_kg() const { return core_.storage_used_kg(); }
double EverwardSim::get_storage_capacity_kg() const { return core_.storage_capacity_kg(); }
bool EverwardSim::has_deposit() const { return core_.has_deposit(); }
String EverwardSim::save_json() const { return String::utf8(core_.save_json().c_str()); }
bool EverwardSim::load_json(const String& json) { return core_.load_json(std::string(json.utf8().get_data())); }

}  // namespace godot
