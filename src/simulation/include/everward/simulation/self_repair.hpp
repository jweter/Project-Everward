#pragma once

#include "everward/simulation/impact_damage.hpp"

#include <array>
#include <cmath>
#include <optional>
#include <stdexcept>
#include <string>

namespace everward::simulation {

// Why the planner picked a target (see docs/STARTER_AWAKENING_AND_SELF_REPAIR.md,
// "Smart repair priority rule"). Lower numeric value = higher priority.
enum class SelfRepairPriority {
    PreserveExistence,   // core/thermal below minimum useful operation
    RestoreOffline,      // offline or sub-minimum system -> minimum useful operation
    ReassessWeakest      // everything online: improve weakest/highest-value system
};

struct SelfRepairResources {
    double material_kg{0.0};
    double energy_j{0.0};
};

struct SelfRepairStep {
    PowerSubsystem subsystem{PowerSubsystem::Computation};
    SelfRepairPriority priority{SelfRepairPriority::ReassessWeakest};
    double integrity_before{0.0};
    double integrity_after{0.0};
    double material_kg{0.0};
    double energy_j{0.0};
    double duration_s{0.0};
    // False when the player lacks the material/energy for this step; the
    // shortfalls say what must be gathered. The planner never silently
    // substitutes a lesser repair, so the HUD can explain what is needed.
    bool affordable{false};
    double material_shortfall_kg{0.0};
    double energy_shortfall_j{0.0};
};

// Deterministic, engine-independent Self Repair planner. It only recommends
// and applies integrity changes; it does not own resource stockpiles, so
// callers deduct `material_kg`/`energy_j` from their own inventory/power state.
class SelfRepairPlanner {
public:
    // Provisional Generation-1 calibration (design targets, not final balance):
    // cost to restore one subsystem from 0% to 100%.
    static constexpr double kMaterialPerFullRepairKg = 4.0;
    static constexpr double kEnergyPerFullRepairJ = 5'000'000.0;
    static constexpr double kSecondsPerFullRepair = 600.0;

    // First functional band; matches the top of IntegrityBand::Critical.
    static constexpr double kMinimumOperationalIntegrity = 0.25;
    static constexpr std::array<double, 4> kStageTargets{0.25, 0.50, 0.75, 1.0};
    static constexpr double kDoneEpsilon = 1e-9;

    // Recommends the next single repair step, or nullopt when every
    // subsystem is at 100%. Call again after each completed step.
    [[nodiscard]] static std::optional<SelfRepairStep> next_step(
            const ComponentIntegritySnapshot& integrity,
            const SelfRepairResources& available) {
        validate_resources(available);
        const std::array<PowerSubsystem, 4> by_importance = {
            PowerSubsystem::Computation,
            PowerSubsystem::Thermal,
            PowerSubsystem::Sensors,
            PowerSubsystem::Propulsion,
        };

        std::optional<PowerSubsystem> chosen;
        SelfRepairPriority priority = SelfRepairPriority::ReassessWeakest;

        // Priority 1: core viability and thermal safety.
        for (PowerSubsystem s : {PowerSubsystem::Computation, PowerSubsystem::Thermal}) {
            if (value(integrity, s) < kMinimumOperationalIntegrity) {
                chosen = s;
                priority = SelfRepairPriority::PreserveExistence;
                break;
            }
        }

        // Priorities 2-3: capability before perfection -- the most broken
        // sub-minimum system first (offline outranks merely low), ties by
        // strategic importance.
        if (!chosen) {
            chosen = weakest(integrity, by_importance, [](double v) {
                return v < kMinimumOperationalIntegrity;
            });
            if (chosen) priority = SelfRepairPriority::RestoreOffline;
        }

        // Priorities 4-5: everything minimally online; improve the weakest.
        if (!chosen) {
            chosen = weakest(integrity, by_importance, [](double v) {
                return v < 1.0 - kDoneEpsilon;
            });
        }
        if (!chosen) {
            return std::nullopt;
        }

        const double before = value(integrity, *chosen);
        const double after = next_stage_target(before);
        const double delta = after - before;

        SelfRepairStep step;
        step.subsystem = *chosen;
        step.priority = priority;
        step.integrity_before = before;
        step.integrity_after = after;
        step.material_kg = delta * kMaterialPerFullRepairKg;
        step.energy_j = delta * kEnergyPerFullRepairJ;
        step.duration_s = delta * kSecondsPerFullRepair;
        step.material_shortfall_kg = std::max(0.0, step.material_kg - available.material_kg);
        step.energy_shortfall_j = std::max(0.0, step.energy_j - available.energy_j);
        step.affordable = step.material_shortfall_kg <= 0.0 && step.energy_shortfall_j <= 0.0;
        return step;
    }

    // Applies a completed step to the authoritative integrity model. Never
    // lowers integrity, so a stale step cannot cause damage.
    static void apply(ImpactDamageModel& model, const SelfRepairStep& step) {
        const double current = model.subsystem_integrity(step.subsystem);
        if (step.integrity_after > current) {
            model.set_subsystem_integrity(step.subsystem, step.integrity_after);
        }
    }

    // Starter-zone departure gate: every required subsystem at 100%.
    [[nodiscard]] static bool is_fully_restored(const ComponentIntegritySnapshot& i) noexcept {
        return i.sensors >= 1.0 - kDoneEpsilon && i.propulsion >= 1.0 - kDoneEpsilon &&
               i.computation >= 1.0 - kDoneEpsilon && i.thermal >= 1.0 - kDoneEpsilon;
    }

    [[nodiscard]] static const char* priority_name(SelfRepairPriority p) noexcept {
        switch (p) {
            case SelfRepairPriority::PreserveExistence: return "PRESERVE EXISTENCE";
            case SelfRepairPriority::RestoreOffline: return "RESTORE CAPABILITY";
            case SelfRepairPriority::ReassessWeakest: return "REPAIR WEAKEST";
        }
        return "UNKNOWN";
    }

private:
    static void validate_resources(const SelfRepairResources& r) {
        if (!std::isfinite(r.material_kg) || r.material_kg < 0.0 ||
            !std::isfinite(r.energy_j) || r.energy_j < 0.0) {
            throw std::invalid_argument("repair resources must be finite and non-negative");
        }
    }

    [[nodiscard]] static double value(const ComponentIntegritySnapshot& i, PowerSubsystem s) {
        double v = 0.0;
        switch (s) {
            case PowerSubsystem::Sensors: v = i.sensors; break;
            case PowerSubsystem::Propulsion: v = i.propulsion; break;
            case PowerSubsystem::Computation: v = i.computation; break;
            case PowerSubsystem::Thermal: v = i.thermal; break;
        }
        if (!std::isfinite(v) || v < 0.0 || v > 1.0) {
            throw std::invalid_argument("component integrity must be finite and within [0, 1]");
        }
        return v;
    }

    template <typename Pred>
    [[nodiscard]] static std::optional<PowerSubsystem> weakest(
            const ComponentIntegritySnapshot& i,
            const std::array<PowerSubsystem, 4>& by_importance,
            Pred eligible) {
        std::optional<PowerSubsystem> best;
        double best_value = 2.0;
        for (PowerSubsystem s : by_importance) {
            const double v = value(i, s);
            if (eligible(v) && v < best_value) {  // strict: importance order breaks ties
                best = s;
                best_value = v;
            }
        }
        return best;
    }

    [[nodiscard]] static double next_stage_target(double before) {
        for (double target : kStageTargets) {
            if (target > before + kDoneEpsilon) return target;
        }
        return 1.0;
    }
};

}  // namespace everward::simulation
