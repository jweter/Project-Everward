#include "everward_sim.h"

#include <godot_cpp/core/class_db.hpp>

namespace godot {

void EverwardSim::_bind_methods() {
    ClassDB::bind_method(D_METHOD("reset"), &EverwardSim::reset);
    ClassDB::bind_method(D_METHOD("advance_ticks", "wall_ticks"), &EverwardSim::advance_ticks);
    ClassDB::bind_method(D_METHOD("get_tick"), &EverwardSim::get_tick);
    ClassDB::bind_method(D_METHOD("set_velocity", "x", "y", "z"), &EverwardSim::set_velocity);
    ClassDB::bind_method(D_METHOD("get_position"), &EverwardSim::get_position);
    ClassDB::bind_method(D_METHOD("get_velocity"), &EverwardSim::get_velocity);
    ClassDB::bind_method(D_METHOD("get_attitude"), &EverwardSim::get_attitude);
    ClassDB::bind_method(D_METHOD("adjust_attitude", "yaw", "pitch", "roll"), &EverwardSim::adjust_attitude);
    ClassDB::bind_method(D_METHOD("adjust_local_velocity", "x", "y", "z"), &EverwardSim::adjust_local_velocity);
    ClassDB::bind_method(D_METHOD("add_sphere_body", "id", "x", "y", "z", "radius"), &EverwardSim::add_sphere_body);
    ClassDB::bind_method(D_METHOD("clear_bodies"), &EverwardSim::clear_bodies);
    ClassDB::bind_method(D_METHOD("move_sphere_body", "id", "x", "y", "z"), &EverwardSim::move_sphere_body);
    ClassDB::bind_method(D_METHOD("get_last_contact"), &EverwardSim::get_last_contact);
    ClassDB::bind_method(D_METHOD("drain_contact_events"), &EverwardSim::drain_contact_events);
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
bool EverwardSim::advance_ticks(int64_t wall_ticks) { return core_.advance_ticks(wall_ticks); }
int64_t EverwardSim::get_tick() const { return core_.tick(); }
bool EverwardSim::set_velocity(double x, double y, double z) { return core_.set_velocity(x, y, z); }

PackedFloat64Array EverwardSim::get_position() const {
    const auto p = core_.position();
    PackedFloat64Array out;
    out.push_back(p.x);
    out.push_back(p.y);
    out.push_back(p.z);
    return out;
}

static PackedFloat64Array triple(double a, double b, double c) {
    PackedFloat64Array out;
    out.push_back(a);
    out.push_back(b);
    out.push_back(c);
    return out;
}

PackedFloat64Array EverwardSim::get_velocity() const {
    const auto v = core_.velocity();
    return triple(v.x, v.y, v.z);
}
PackedFloat64Array EverwardSim::get_attitude() const {
    const auto a = core_.attitude();
    return triple(a.yaw, a.pitch, a.roll);
}
bool EverwardSim::adjust_attitude(double yaw, double pitch, double roll) { return core_.adjust_attitude(yaw, pitch, roll); }
bool EverwardSim::adjust_local_velocity(double x, double y, double z) { return core_.adjust_local_velocity(x, y, z); }
bool EverwardSim::add_sphere_body(const String& id, double x, double y, double z, double radius) {
    return core_.add_sphere_body(std::string(id.utf8().get_data()), x, y, z, radius);
}
void EverwardSim::clear_bodies() { core_.clear_bodies(); }
bool EverwardSim::move_sphere_body(const String& id, double x, double y, double z) {
    return core_.move_sphere_body(std::string(id.utf8().get_data()), x, y, z);
}
Dictionary EverwardSim::get_last_contact() const {
    const auto c = core_.last_contact();
    Dictionary d;
    d["has_contact"] = c.has_contact;
    d["body_id"] = String::utf8(c.body_id.c_str());
    d["point"] = triple(c.point.x, c.point.y, c.point.z);
    d["normal"] = triple(c.normal.x, c.normal.y, c.normal.z);
    d["normal_speed"] = c.normal_speed;
    d["tick"] = c.tick;
    return d;
}
int64_t EverwardSim::drain_contact_events() { return core_.drain_contact_events(); }

bool EverwardSim::setup_bootstrap_deposit(double remaining_kg) { return core_.setup_bootstrap_deposit(remaining_kg); }

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
