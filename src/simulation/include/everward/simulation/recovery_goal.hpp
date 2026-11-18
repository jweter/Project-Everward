#pragma once

#include "everward/simulation/fix_it.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace everward::simulation {

// Fix_It owns repair choice and execution. This is a read-only projection
// for player guidance; it never grants a command or creates a resource source.
// A currently unknown path is NOT evidence that recovery is impossible.
enum class RecoveryGoalState {
    NoRecommendation,
    Blocked,
    Ready,
    InProgress,
    Interrupted,
    Completed,
    VerificationFailed,
    FullyRestored,
    AdvisoryOnly
};

struct RecoveryGoalProjection {
    RecoveryGoalState state{RecoveryGoalState::NoRecommendation};
    bool has_goal{false};
    std::string goal_id;
    FixItDecision decision{};
    double current_integrity{0.0};
    double progress_fraction{0.0};
    double missing_material_kg{0.0};
    double missing_energy_j{0.0};
    double missing_time_s{0.0};
    bool fabrication_unavailable{false};
    // The planner provides the full rationale; no separate goal priority or
    // invented capability unlock is inferred from a numerical integrity band.
    std::string reason;
};

[[nodiscard]] inline constexpr std::string_view recovery_subsystem_name(
        PowerSubsystem subsystem) noexcept {
    switch (subsystem) {
        case PowerSubsystem::Sensors: return "sensors";
        case PowerSubsystem::Propulsion: return "propulsion";
        case PowerSubsystem::Computation: return "computation";
        case PowerSubsystem::Thermal: return "thermal";
    }
    return "unknown";
}

[[nodiscard]] inline constexpr std::string_view recovery_stage_name(
        FixItStage stage) noexcept {
    switch (stage) {
        case FixItStage::Repair: return "repair";
        case FixItStage::Replacement: return "replacement";
        case FixItStage::Upgrade: return "upgrade";
        case FixItStage::Redesign: return "redesign";
        case FixItStage::Evolution: return "evolution";
    }
    return "unknown";
}

[[nodiscard]] inline double recovery_integrity(
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

// A readable stable identifier based on the canonical planner decision.
// Current integrity, changing inventory, scan results and execution progress
// must NOT appear in the ID; a live repair keeps the same goal identity.
// Quantization in millionths is deliberate, rather than std::hash (not
// guaranteed stable across standard-library implementations or platforms).
[[nodiscard]] inline std::string recovery_goal_id(const FixItDecision& decision) {
    return "fix_it:" +
        std::string(recovery_stage_name(decision.stage)) + ":" +
        std::string(recovery_subsystem_name(decision.subsystem)) + ":" +
        std::to_string(static_cast<std::int64_t>(
            std::llround(decision.target_integrity * 1'000'000.0)));
}

// ExecutionStatus must come from the CURRENT FixItRepairExecutor or
// FixItReplacementExecutor, not a guessed completion event or a reconstructed
// save. The actor is responsible for passing an active/short-lived receipt.
[[nodiscard]] inline RecoveryGoalProjection resolve_recovery_goal(
        const ComponentIntegritySnapshot& integrity,
        const FixItResources& available,
        const std::optional<FixItDecision>& planner_decision,
        const FixItExecutionStatus* execution = nullptr) {
    RecoveryGoalProjection result;

    // A currently active or just-completed command wins over the planner's
    // next recommendation so a running step does not jump between goals.
    std::optional<FixItDecision> chosen = planner_decision;
    if (execution != nullptr && execution->decision.has_value() &&
        execution->state != FixItExecutionState::Idle) {
        chosen = execution->decision;
    }

    if (!chosen.has_value()) {
        const bool fully_restored =
            integrity.sensors >= 1.0 - 1e-9 &&
            integrity.propulsion >= 1.0 - 1e-9 &&
            integrity.computation >= 1.0 - 1e-9 &&
            integrity.thermal >= 1.0 - 1e-9;
        result.state = fully_restored
            ? RecoveryGoalState::FullyRestored
            : RecoveryGoalState::NoRecommendation;
        return result;
    }

    const FixItDecision& decision = *chosen;
    result.has_goal = true;
    result.decision = decision;
    result.goal_id = recovery_goal_id(decision);
    result.current_integrity = recovery_integrity(integrity, decision.subsystem);
    result.reason = decision.reason;

    if (execution != nullptr && execution->decision.has_value() &&
        execution->state != FixItExecutionState::Idle) {
        if (decision.time_required_s > 0.0) {
            result.progress_fraction = std::clamp(
                execution->elapsed_s / decision.time_required_s, 0.0, 1.0);
        }
        switch (execution->state) {
            case FixItExecutionState::Running:
                result.state = RecoveryGoalState::InProgress;
                return result;
            case FixItExecutionState::Interrupted:
                result.state = RecoveryGoalState::Interrupted;
                return result;
            case FixItExecutionState::Completed:
                result.state = result.current_integrity + 1e-9 >= decision.target_integrity
                    ? RecoveryGoalState::Completed
                    : RecoveryGoalState::VerificationFailed;
                return result;
            case FixItExecutionState::Idle:
                break;
        }
    }

    // Later engineering stages may be recommended but are not presently
    // executable. Do not label them READY, or promise an unavailable button.
    if (decision.stage != FixItStage::Repair &&
        decision.stage != FixItStage::Replacement) {
        result.state = RecoveryGoalState::AdvisoryOnly;
        return result;
    }

    result.fabrication_unavailable =
        decision.stage == FixItStage::Replacement &&
        !available.fabrication_available;
    result.missing_material_kg = std::max(0.0,
        decision.material_required_kg - available.material_kg);
    result.missing_energy_j = std::max(0.0,
        decision.energy_required_j - available.energy_j);
    result.missing_time_s = std::max(0.0,
        decision.time_required_s - available.available_time_s);

    result.state = result.fabrication_unavailable ||
                   result.missing_material_kg > 1e-9 ||
                   result.missing_energy_j > 1e-6 ||
                   result.missing_time_s > 1e-9
        ? RecoveryGoalState::Blocked
        : RecoveryGoalState::Ready;
    return result;
}

} // namespace everward::simulation
