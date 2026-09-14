#include "everward/simulation/material_consumption.hpp"

#include <cassert>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>

using everward::simulation::plan_material_consumption;

int main() {
    const std::map<std::string, double> inventory{
        {"carbonaceous_regolith", 3.0},
        {"iron_bearing_silicate_regolith", 5.0},
    };

    const auto accepted = plan_material_consumption(
        inventory, "iron_bearing_silicate_regolith", 2.0);
    assert(accepted.can_consume);
    assert(accepted.material_id == "iron_bearing_silicate_regolith");
    assert(accepted.requested_kg == 2.0);
    assert(accepted.available_kg == 5.0);
    assert(accepted.remaining_kg == 3.0);

    const auto exact = plan_material_consumption(
        inventory, "carbonaceous_regolith", 3.0);
    assert(exact.can_consume);
    assert(exact.remaining_kg == 0.0);

    const auto insufficient = plan_material_consumption(
        inventory, "carbonaceous_regolith", 3.5);
    assert(!insufficient.can_consume);
    assert(insufficient.available_kg == 3.0);
    assert(insufficient.remaining_kg == 3.0);

    const auto absent = plan_material_consumption(inventory, "water_ice", 0.5);
    assert(!absent.can_consume);
    assert(absent.available_kg == 0.0);
    assert(absent.remaining_kg == 0.0);

    bool rejected = false;
    try {
        (void)plan_material_consumption(inventory, "", 1.0);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    assert(rejected);

    rejected = false;
    try {
        (void)plan_material_consumption(inventory, "carbonaceous_regolith", 0.0);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    assert(rejected);

    rejected = false;
    try {
        (void)plan_material_consumption(
            inventory,
            "carbonaceous_regolith",
            std::numeric_limits<double>::quiet_NaN());
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    assert(rejected);

    const std::map<std::string, double> invalid_inventory{
        {"carbonaceous_regolith", std::numeric_limits<double>::infinity()},
    };
    rejected = false;
    try {
        (void)plan_material_consumption(invalid_inventory, "carbonaceous_regolith", 1.0);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    assert(rejected);

    return 0;
}
