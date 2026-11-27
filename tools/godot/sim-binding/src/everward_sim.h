#pragma once
// Godot wrapper around the engine-independent SimCore (Godot path G1). Thin on purpose: every number comes
// from the real Everward simulation; this class only converts types. Doubles stay doubles (Godot's Vector3 is
// single precision by default), so positions are returned as PackedFloat64Array.

#include "everward_sim_core.hpp"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_float64_array.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {

class EverwardSim : public RefCounted {
    GDCLASS(EverwardSim, RefCounted)

protected:
    static void _bind_methods();

public:
    void reset();
    void advance_ticks(int64_t wall_ticks);
    int64_t get_tick() const;
    void set_velocity(double x, double y, double z);
    PackedFloat64Array get_position() const;
    void setup_bootstrap_deposit(double remaining_kg);
    Dictionary mine_once();
    double get_inventory_kg(const String& material) const;
    double get_extracted_kg() const;
    double get_deposit_remaining_kg() const;
    String save_json() const;
    bool load_json(const String& json);

private:
    everward::binding::SimCore core_;
};

}  // namespace godot
