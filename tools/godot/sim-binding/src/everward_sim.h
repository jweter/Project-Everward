#pragma once
// Godot wrapper around the engine-independent SimCore (Godot path G1). Thin on purpose: every number comes
// from the real Everward simulation; this class only converts types. Doubles stay doubles (Godot's Vector3 is
// single precision by default), so positions are returned as PackedFloat64Array.

#include "everward_sim_core.hpp"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_float64_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {

class EverwardSim : public RefCounted {
    GDCLASS(EverwardSim, RefCounted)

protected:
    static void _bind_methods();

public:
    void reset();
    bool advance_ticks(int64_t wall_ticks);
    int64_t get_tick() const;
    bool set_velocity(double x, double y, double z);
    PackedFloat64Array get_position() const;
    PackedFloat64Array get_velocity() const;
    PackedFloat64Array get_attitude() const;
    bool adjust_attitude(double yaw, double pitch, double roll);
    bool adjust_local_velocity(double x, double y, double z);
    bool add_sphere_body(const String& id, double x, double y, double z, double radius);
    void clear_bodies();
    bool move_sphere_body(const String& id, double x, double y, double z);
    Dictionary get_last_contact() const;
    int64_t drain_contact_events();
    bool setup_bootstrap_deposit(double remaining_kg);
    Dictionary mine_once();
    double get_inventory_kg(const String& material) const;
    double get_extracted_kg() const;
    double get_deposit_remaining_kg() const;
    double get_storage_used_kg() const;
    double get_storage_capacity_kg() const;
    bool has_deposit() const;
    String save_json() const;
    bool load_json(const String& json);

    // G3 first loop
    bool add_deposit(const String& id, double x, double y, double z, double radius, const String& material_id,
                     const String& display_name, double remaining_kg, double kg_per_cycle);
    bool is_surveyed(const String& id) const;
    double get_world_deposit_remaining_kg(const String& id) const;
    bool start_scan(const String& target_id, double duration_s);
    bool cancel_scan();
    bool is_scanning() const;
    double get_scan_remaining_s() const;
    String get_scan_target() const;
    bool toggle_arm(int64_t arm);
    bool attach_tool(int64_t arm);
    bool detach_tool(int64_t arm);
    bool set_joint_targets(int64_t arm, double shoulder, double elbow, double wrist);
    Dictionary get_arm_state(int64_t arm) const;
    Array get_arm_points(int64_t arm) const;
    Dictionary plan_staging(int64_t arm, const String& target_id) const;
    Dictionary plan_arm_path(int64_t arm, double shoulder, double elbow, double wrist) const;
    Dictionary mine_deposit(const String& target_id);
    void enable_fix_it(bool seed_damaged_awakening);
    Dictionary get_fix_it_status() const;
    Dictionary get_integrity() const;
    double get_stored_energy_j() const;
    PackedStringArray drain_fix_it_events();
    String get_last_scan_refusal() const;
    String get_approach_blocker(double x, double y, double z) const;
    PackedFloat64Array get_body_center(const String& id) const;
    bool allocate_power(int64_t subsystem, double watts);
    PackedFloat64Array get_power_allocations() const;
    bool install_basic_survival_policy();
    bool clear_policy();
    Dictionary get_policy_status() const;
    Dictionary get_scan_power_guidance() const;
    int64_t bounded_selection(int64_t index, int64_t delta, int64_t count) const;

private:
    everward::binding::SimCore core_;
};

}  // namespace godot
