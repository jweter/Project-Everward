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
    ClassDB::bind_method(D_METHOD("add_deposit", "id", "x", "y", "z", "radius", "material_id", "display_name",
                                  "remaining_kg", "kg_per_cycle"),
                         &EverwardSim::add_deposit);
    ClassDB::bind_method(D_METHOD("is_surveyed", "id"), &EverwardSim::is_surveyed);
    ClassDB::bind_method(D_METHOD("get_world_deposit_remaining_kg", "id"), &EverwardSim::get_world_deposit_remaining_kg);
    ClassDB::bind_method(D_METHOD("start_scan", "target_id", "duration_s"), &EverwardSim::start_scan);
    ClassDB::bind_method(D_METHOD("cancel_scan"), &EverwardSim::cancel_scan);
    ClassDB::bind_method(D_METHOD("is_scanning"), &EverwardSim::is_scanning);
    ClassDB::bind_method(D_METHOD("get_scan_remaining_s"), &EverwardSim::get_scan_remaining_s);
    ClassDB::bind_method(D_METHOD("get_scan_target"), &EverwardSim::get_scan_target);
    ClassDB::bind_method(D_METHOD("toggle_arm", "arm"), &EverwardSim::toggle_arm);
    ClassDB::bind_method(D_METHOD("attach_tool", "arm"), &EverwardSim::attach_tool);
    ClassDB::bind_method(D_METHOD("detach_tool", "arm"), &EverwardSim::detach_tool);
    ClassDB::bind_method(D_METHOD("set_joint_targets", "arm", "shoulder", "elbow", "wrist"), &EverwardSim::set_joint_targets);
    ClassDB::bind_method(D_METHOD("get_arm_state", "arm"), &EverwardSim::get_arm_state);
    ClassDB::bind_method(D_METHOD("get_arm_points", "arm"), &EverwardSim::get_arm_points);
    ClassDB::bind_method(D_METHOD("plan_staging", "arm", "target_id"), &EverwardSim::plan_staging);
    ClassDB::bind_method(D_METHOD("plan_arm_path", "arm", "shoulder", "elbow", "wrist"), &EverwardSim::plan_arm_path);
    ClassDB::bind_method(D_METHOD("mine_deposit", "target_id"), &EverwardSim::mine_deposit);
    ClassDB::bind_method(D_METHOD("enable_fix_it", "seed_damaged_awakening"), &EverwardSim::enable_fix_it);
    ClassDB::bind_method(D_METHOD("get_fix_it_status"), &EverwardSim::get_fix_it_status);
    ClassDB::bind_method(D_METHOD("get_integrity"), &EverwardSim::get_integrity);
    ClassDB::bind_method(D_METHOD("get_stored_energy_j"), &EverwardSim::get_stored_energy_j);
    ClassDB::bind_method(D_METHOD("drain_fix_it_events"), &EverwardSim::drain_fix_it_events);
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

static std::string to_std(const String& s) { return std::string(s.utf8().get_data()); }
static PackedFloat64Array angles3(const everward::simulation::ManipulatorArmAngles& a) {
    return triple(a.shoulder_degrees, a.elbow_degrees, a.wrist_degrees);
}

bool EverwardSim::add_deposit(const String& id, double x, double y, double z, double radius, const String& material_id,
                              const String& display_name, double remaining_kg, double kg_per_cycle) {
    return core_.add_deposit(to_std(id), x, y, z, radius, to_std(material_id), to_std(display_name), remaining_kg,
                             kg_per_cycle);
}
bool EverwardSim::is_surveyed(const String& id) const { return core_.is_surveyed(to_std(id)); }
double EverwardSim::get_world_deposit_remaining_kg(const String& id) const {
    return core_.world_deposit_remaining_kg(to_std(id));
}
bool EverwardSim::start_scan(const String& target_id, double duration_s) {
    return core_.start_scan(to_std(target_id), duration_s);
}
bool EverwardSim::cancel_scan() { return core_.cancel_scan(); }
bool EverwardSim::is_scanning() const { return core_.is_scanning(); }
double EverwardSim::get_scan_remaining_s() const { return core_.scan_remaining_s(); }
String EverwardSim::get_scan_target() const { return String::utf8(core_.scan_target().c_str()); }
bool EverwardSim::toggle_arm(int64_t arm) { return core_.toggle_arm(static_cast<int>(arm)); }
bool EverwardSim::attach_tool(int64_t arm) { return core_.attach_tool(static_cast<int>(arm)); }
bool EverwardSim::detach_tool(int64_t arm) { return core_.detach_tool(static_cast<int>(arm)); }
bool EverwardSim::set_joint_targets(int64_t arm, double shoulder, double elbow, double wrist) {
    return core_.set_joint_targets(static_cast<int>(arm), shoulder, elbow, wrist);
}
Dictionary EverwardSim::get_arm_state(int64_t arm) const {
    const auto a = core_.arm_info(static_cast<int>(arm));
    Dictionary d;
    d["deployed"] = a.deployed;
    d["deploying"] = a.deploying;
    d["stowing"] = a.stowing;
    d["tool_attached"] = a.tool_attached;
    d["accepts_joint_commands"] = a.accepts_joint_commands;
    d["deployment_fraction"] = a.deployment_fraction;
    d["angles"] = angles3(a.angles);
    d["commanded"] = angles3(a.commanded);
    return d;
}
Array EverwardSim::get_arm_points(int64_t arm) const {
    Array out;
    for (const auto& p : core_.arm_points(static_cast<int>(arm))) out.push_back(triple(p.x, p.y, p.z));
    return out;
}
Dictionary EverwardSim::plan_staging(int64_t arm, const String& target_id) const {
    const auto p = core_.plan_staging(static_cast<int>(arm), to_std(target_id));
    Dictionary d;
    d["has_solution"] = p.has_solution;
    d["position"] = triple(p.probe_position.x, p.probe_position.y, p.probe_position.z);
    d["angles"] = angles3(p.angles);
    d["tool_gap_m"] = p.tool_gap_m;
    return d;
}
Dictionary EverwardSim::plan_arm_path(int64_t arm, double shoulder, double elbow, double wrist) const {
    const auto path = core_.plan_arm_path(static_cast<int>(arm), shoulder, elbow, wrist);
    Dictionary d;
    Array waypoints;
    if (path) {
        for (const auto& w : *path) waypoints.push_back(angles3(w));
    }
    d["ok"] = path.has_value();
    d["waypoints"] = waypoints;
    return d;
}
Dictionary EverwardSim::mine_deposit(const String& target_id) {
    const auto r = core_.mine_deposit(to_std(target_id));
    Dictionary d;
    d["accepted"] = r.accepted;
    d["extracted_kg"] = r.extracted_kg;
    d["remaining_kg"] = r.remaining_kg;
    d["tool_gap_m"] = r.tool_gap_m;
    d["arm"] = r.arm;
    d["material_id"] = String::utf8(r.material_id.c_str());
    d["reason"] = String::utf8(r.reason.c_str());
    return d;
}

void EverwardSim::enable_fix_it(bool seed_damaged_awakening) { core_.enable_fix_it(seed_damaged_awakening); }
Dictionary EverwardSim::get_fix_it_status() const {
    const auto f = core_.fix_it_info();
    Dictionary d;
    d["enabled"] = f.enabled;
    d["state"] = String::utf8(f.state.c_str());
    d["stage"] = String::utf8(f.stage.c_str());
    d["subsystem"] = String::utf8(f.subsystem.c_str());
    d["reason"] = String::utf8(f.reason.c_str());
    d["progress"] = f.progress;
    d["material_required_kg"] = f.material_required_kg;
    d["material_consumed_kg"] = f.material_consumed_kg;
    d["last_completed_material_consumed_kg"] = f.last_completed_material_consumed_kg;
    d["energy_required_j"] = f.energy_required_j;
    d["energy_consumed_j"] = f.energy_consumed_j;
    d["target_integrity"] = f.target_integrity;
    d["last_event"] = String::utf8(f.last_event.c_str());
    return d;
}
Dictionary EverwardSim::get_integrity() const {
    const auto i = core_.integrity();
    Dictionary d;
    d["sensors"] = i.sensors;
    d["propulsion"] = i.propulsion;
    d["computation"] = i.computation;
    d["thermal"] = i.thermal;
    return d;
}
double EverwardSim::get_stored_energy_j() const { return core_.stored_energy_j(); }
PackedStringArray EverwardSim::drain_fix_it_events() {
    PackedStringArray out;
    for (const auto& line : core_.drain_fix_it_events()) out.push_back(String::utf8(line.c_str()));
    return out;
}

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
