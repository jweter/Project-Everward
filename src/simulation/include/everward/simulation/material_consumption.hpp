#pragma once

#include <cmath>
#include <map>
#include <stdexcept>
#include <string>

namespace everward::simulation {

struct MaterialConsumptionPlan {
    std::string material_id;
    double requested_kg{0.0};
    double available_kg{0.0};
    double remaining_kg{0.0};
    bool can_consume{false};
};

// Slice 12 resource/sample-loop foundation: decide whether one explicit
// material can satisfy a use request without falling back to generic storage
// depletion. This is deliberately a pure planning primitive: SimulationCore
// remains the sole authoritative inventory mutation boundary, and a later
// consumer (processing/Fix_It/fabrication) can only apply a plan after naming
// the material it actually requires.
[[nodiscard]] inline MaterialConsumptionPlan plan_material_consumption(
        const std::map<std::string, double>& inventory_kg,
        const std::string& material_id,
        double requested_kg) {
    if (material_id.empty()) {
        throw std::invalid_argument("material_id must be non-empty");
    }
    if (!std::isfinite(requested_kg) || requested_kg <= 0.0) {
        throw std::invalid_argument("requested material consumption must be finite and positive");
    }

    MaterialConsumptionPlan plan;
    plan.material_id = material_id;
    plan.requested_kg = requested_kg;

    const auto it = inventory_kg.find(material_id);
    if (it == inventory_kg.end()) {
        return plan;
    }
    if (!std::isfinite(it->second) || it->second <= 0.0) {
        throw std::invalid_argument("inventory entry must be finite and positive");
    }

    plan.available_kg = it->second;
    if (it->second + 1e-9 < requested_kg) {
        plan.remaining_kg = it->second;
        return plan;
    }

    plan.can_consume = true;
    plan.remaining_kg = it->second - requested_kg;
    if (std::abs(plan.remaining_kg) < 1e-9) {
        plan.remaining_kg = 0.0;
    }
    return plan;
}

} // namespace everward::simulation
