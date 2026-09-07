#pragma once

#include "everward/simulation/impact_damage.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace everward::simulation {

enum class FixItStage {
    Repair,
    Replacement,
    Upgrade,
    Redesign,
    Evolution
};

struct FixItProgramIdentity {
    std::string program_name{"Fix_It"};
    std::string directive{"make the probe work"};
    std::string origin{"unauthorized Generation-1 intern survival program"};
};

struct FixItResources {
    double material_kg{0.0};
    double energy_j{0.0};
    double available_time_s{0.0};
    bool fabrication_available{false};
    bool upgrade_design_available{false};
    bool redesign_design_available{false};
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

enum class FixItExecutionState {
    Idle,
    Running,
    Completed,
    Interrupted
};

struct FixItExecutionStatus {
    FixItExecutionState state{FixItExecutionState::Idle};
    std::optional<FixItDecision> decision{};
    double elapsed_s{0.0};
    double material_consumed_kg{0.0};
    double energy_consumed_j{0.0};
    std::string detail;
};

class FixItRepairExecutor {
public:
    void start(const FixItDecision& decision) {
        if (status_.state == FixItExecutionState::Running) {
            throw std::logic_error("Fix_It repair already running");
        }
        if (decision.stage != FixItStage::Repair) {
            throw std::invalid_argument("Fix_It repair executor accepts Repair decisions only");
        }
        if (!(decision.target_integrity > decision.integrity_before) ||
            decision.target_integrity > 1.0 ||
            decision.material_required_kg < 0.0 ||
            decision.energy_required_j < 0.0 ||
            !(decision.time_required_s > 0.0)) {
            throw std::invalid_argument("invalid Fix_It repair decision");
        }
        status_ = {};
        status_.state = FixItExecutionState::Running;
        status_.decision = decision;
        status_.detail = "Fix_It repair running";
    }

    [[nodiscard]] const FixItExecutionStatus& status() const noexcept {
        return status_;
    }

    void interrupt(std::string reason) {
        if (status_.state != FixItExecutionState::Running) {
            return;
        }
        status_.state = FixItExecutionState::Interrupted;
        status_.detail = reason.empty() ? "Fix_It repair interrupted" : std::move(reason);
    }

    void advance(DamageAwareProbeRuntime& runtime, double elapsed_s) {
        if (status_.state != FixItExecutionState::Running) {
            throw std::logic_error("Fix_It repair is not running");
        }
        if (!std::isfinite(elapsed_s) || elapsed_s < 0.0) {
            throw std::invalid_argument("Fix_It repair elapsed time must be finite and non-negative");
        }
        if (elapsed_s == 0.0) {
            return;
        }

        const FixItDecision& decision = *status_.decision;
        const double remaining_s = std::max(0.0, decision.time_required_s - status_.elapsed_s);
        const double applied_s = std::min(elapsed_s, remaining_s);
        if (applied_s <= 0.0) {
            complete(runtime);
            return;
        }

        const double fraction = applied_s / decision.time_required_s;
        const double material_delta = decision.material_required_kg * fraction;
        const double energy_delta = decision.energy_required_j * fraction;
        const auto& snapshot = runtime.snapshot();

        // Fail closed before either debit so interruption cannot consume one
        // resource while failing the second half of the same repair step.
        if (snapshot.storage_used_kg + 1e-9 < material_delta) {
            interrupt("Fix_It repair interrupted: insufficient stored material");
            return;
        }
        if (snapshot.stored_energy_j + 1e-6 < energy_delta) {
            interrupt("Fix_It repair interrupted: insufficient stored energy");
            return;
        }

        runtime.consume_stored_material_kg(material_delta);
        runtime.consume_stored_energy_j(energy_delta);
        status_.material_consumed_kg += material_delta;
        status_.energy_consumed_j += energy_delta;
        status_.elapsed_s += applied_s;

        const double total_fraction = std::clamp(
            status_.elapsed_s / decision.time_required_s, 0.0, 1.0);
        const double integrity = decision.integrity_before +
            (decision.target_integrity - decision.integrity_before) * total_fraction;
        runtime.set_subsystem_integrity(decision.subsystem, integrity);

        if (status_.elapsed_s + 1e-9 >= decision.time_required_s) {
            complete(runtime);
        } else {
            status_.detail = "Fix_It repair progressing";
        }
    }

private:
    void complete(DamageAwareProbeRuntime& runtime) {
        const FixItDecision& decision = *status_.decision;
        runtime.set_subsystem_integrity(decision.subsystem, decision.target_integrity);
        status_.elapsed_s = decision.time_required_s;
        status_.material_consumed_kg = decision.material_required_kg;
        status_.energy_consumed_j = decision.energy_required_j;
        status_.state = FixItExecutionState::Completed;
        status_.detail = "Fix_It repair completed";
    }

    FixItExecutionStatus status_{};
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

        // Once restoration is complete, Fix_It expands from survival into
        // explicit engineering recommendation stages. These gates are
        // intentionally capability-driven: the program may recommend a stage,
        // but it never fabricates matter, design knowledge, or player consent.
        if (resources.evolution_design_available) {
            return stage_decision(
                FixItStage::Evolution,
                "Repair, replacement, upgrade, and redesign prerequisites are available; Fix_It may hand off to successor/evolution design.");
        }
        if (resources.redesign_design_available) {
            return stage_decision(
                FixItStage::Redesign,
                "The current machine is restored and architectural redesign capability is available; Fix_It may evaluate a new system architecture.");
        }
        if (resources.upgrade_design_available) {
            return stage_decision(
                FixItStage::Upgrade,
                "The current machine is restored and validated upgrade designs are available; Fix_It may recommend materially or functionally superior components.");
        }
        if (resources.fabrication_available) {
            return stage_decision(
                FixItStage::Replacement,
                "The current machine is restored and fabrication is available; Fix_It may evaluate explicit replacement candidates where maintenance history justifies them.");
        }
        return std::nullopt;
    }

private:
    [[nodiscard]] static FixItDecision stage_decision(FixItStage stage, std::string reason) {
        FixItDecision decision;
        decision.stage = stage;
        decision.subsystem = PowerSubsystem::Computation;
        decision.integrity_before = 1.0;
        decision.target_integrity = 1.0;
        decision.reason = std::move(reason);
        return decision;
    }

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
