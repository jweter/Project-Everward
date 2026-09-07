#pragma once

#include "everward/simulation/impact_damage.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>
#include <stdexcept>
#include <string>

namespace everward::simulation {

enum class FixItStage {
    Repair,
    Replacement,
    Evolution
};

struct FixItResources {
    double material_kg{0.0};
    double energy_j{0.0};
    double available_time_s{0.0};
    bool fabrication_available{false};
    bool evolution_design_available{false};
};

struct FixItComponentPolicy {
    PowerSubsystem subsystem{PowerSubsystem::Sensors};
    bool required_for_survival{false};
    double minimum_useful_integrity{0.25};
    double material_kg_per_integrity{10.0};
    double energy_j_per_integrity{1.0e6};
    double seconds_per_integrity{120.0};
    bool replaceable{true};
};

struct FixItDecision {
    FixItStage stage{FixItStage::Repair};
    PowerSubsystem subsystem{PowerSubsystem::Sensors};
    double integrity_before{0.0};
    double target_integrity{0.0};
    double material_required_kg{0.0};
    double energy_required_j{0.0};
    double time_required_s{0.0};
    std::string reason;
};

class FixItPlanner {
public:
    [[nodiscard]] static constexpr std::array<FixItComponentPolicy, 4> canonical_generation1_policy() noexcept {
        return {{
            {PowerSubsystem::Computation, true, 0.25, 14.0, 1.8e6, 180.0, true},
            {PowerSubsystem::Thermal, true, 0.25, 11.0, 1.3e6, 150.0, true},
            {PowerSubsystem::Sensors, false, 0.25, 8.0, 0.9e6, 90.0, true},
            {PowerSubsystem::Propulsion, false, 0.25, 18.0, 2.5e6, 240.0, true},
        }};
    }

    [[nodiscard]] static std::optional<FixItDecision> plan_next(
        const ComponentIntegritySnapshot& integrity,
        const FixItResources& resources) {

        validate_resources(resources);
        const auto policy = canonical_generation1_policy();

        // Fix_It's first invariant: preserve the computation/thermal substrate
        // that allows the machine to continue existing and repairing itself.
        for (const auto& component : policy) {
            if (!component.required_for_survival) continue;
            const double current = value_for(integrity, component.subsystem);
            if (current < component.minimum_useful_integrity) {
                return build_decision(component, current, resources);
            }
        }

        // Second invariant: restore missing capabilities before polishing
        // already-functional ones. This directly implements the canonical
        // "restore capability before perfection" doctrine.
        for (const auto& component : policy) {
            const double current = value_for(integrity, component.subsystem);
            if (current <= 0.0 || current < component.minimum_useful_integrity) {
                return build_decision(component, current, resources);
            }
        }

        // Third invariant: once every subsystem is usable, improve the weakest
        // component in staged bands rather than jumping straight to 100%.
        const FixItComponentPolicy* weakest = nullptr;
        double weakest_integrity = 2.0;
        for (const auto& component : policy) {
            const double current = value_for(integrity, component.subsystem);
            if (current < 1.0 && current < weakest_integrity) {
                weakest = &component;
                weakest_integrity = current;
            }
        }
        if (weakest != nullptr) {
            return build_decision(*weakest, weakest_integrity, resources);
        }

        // Repair is complete. Replacement/evolution are separate explicit
        // stages: never silently mutate the original machine.
        if (resources.fabrication_available) {
            FixItDecision decision;
            decision.stage = FixItStage::Replacement;
            decision.subsystem = PowerSubsystem::Computation;
            decision.integrity_before = 1.0;
            decision.target_integrity = 1.0;
            decision.reason =
                "All Generation-1 components are restored; fabrication is available, so Fix_It may evaluate explicit replacement candidates.";
            if (resources.evolution_design_available) {
                decision.stage = FixItStage::Evolution;
                decision.reason =
                    "Repair and replacement prerequisites are satisfied; Fix_It may hand off to the explicit evolution/design frontier.";
            }
            return decision;
        }
        return std::nullopt;
    }

private:
    [[nodiscard]] static double value_for(
        const ComponentIntegritySnapshot& integrity,
        PowerSubsystem subsystem) noexcept {
        switch (subsystem) {
            case PowerSubsystem::Sensors: return integrity.sensors;
            case PowerSubsystem::Propulsion: return integrity.propulsion;
            case PowerSubsystem::Computation: return integrity.computation;
            case PowerSubsystem::Thermal: return integrity.thermal;
        }
        return 0.0;
    }

    [[nodiscard]] static double next_band(double current, double minimum_useful) noexcept {
        if (current < minimum_useful) return minimum_useful;
        if (current < 0.50) return 0.50;
        if (current < 0.75) return 0.75;
        return 1.0;
    }

    [[nodiscard]] static FixItDecision build_decision(
        const FixItComponentPolicy& component,
        double current,
        const FixItResources& resources) {

        const double target = next_band(current, component.minimum_useful_integrity);
        const double delta = std::max(0.0, target - current);

        FixItDecision decision;
        decision.subsystem = component.subsystem;
        decision.integrity_before = current;
        decision.target_integrity = target;
        decision.material_required_kg = delta * component.material_kg_per_integrity;
        decision.energy_required_j = delta * component.energy_j_per_integrity;
        decision.time_required_s = delta * component.seconds_per_integrity;

        if (current <= 0.0 && component.replaceable && resources.fabrication_available &&
            !can_afford(decision, resources)) {
            decision.stage = FixItStage::Replacement;
            decision.reason =
                "Component is offline and staged repair cannot currently be afforded; fabrication permits an explicit replacement path.";
            return decision;
        }

        decision.stage = FixItStage::Repair;
        if (component.required_for_survival && current < component.minimum_useful_integrity) {
            decision.reason = "Preserve existence: restore a survival-critical subsystem to minimum useful operation.";
        } else if (current < component.minimum_useful_integrity) {
            decision.reason = "Restore capability before perfection: bring an offline/degraded subsystem to minimum useful operation.";
        } else {
            decision.reason = "All required capabilities are online; improve the weakest functioning subsystem to the next useful integrity band.";
        }
        return decision;
    }

    [[nodiscard]] static bool can_afford(
        const FixItDecision& decision,
        const FixItResources& resources) noexcept {
        return resources.material_kg >= decision.material_required_kg &&
               resources.energy_j >= decision.energy_required_j &&
               resources.available_time_s >= decision.time_required_s;
    }

    static void validate_resources(const FixItResources& resources) {
        const double values[] = {resources.material_kg, resources.energy_j, resources.available_time_s};
        for (double value : values) {
            if (!std::isfinite(value) || value < 0.0) {
                throw std::invalid_argument("Fix_It resources must be finite and non-negative");
            }
        }
    }
};

} // namespace everward::simulation
