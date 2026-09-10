#pragma once

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>

namespace everward::simulation {

enum class ObservationMode {
    Passive,
    ActiveScan,
};

enum class KnowledgeLevel {
    Unknown,
    Observed,
    Characterized,
};

struct ObservationEvidence {
    ObservationMode mode{ObservationMode::Passive};
    double duration_s{0.0};
    // Confidence gain is supplied by the observing instrument/model. This state
    // container never invents statistical precision from elapsed time alone.
    double confidence_gain{0.0};
    // Normalized resolving power supplied by the observing instrument/model.
    double instrument_resolution{0.0};
    std::string classification{};
};

struct TargetKnowledgeState {
    std::string target_id{};
    KnowledgeLevel level{KnowledgeLevel::Unknown};
    double passive_observation_s{0.0};
    double active_scan_s{0.0};
    double confidence{0.0};
    double best_instrument_resolution{0.0};
    std::string classification{};
};

[[nodiscard]] inline TargetKnowledgeState make_unknown_target_knowledge(std::string target_id) {
    if (target_id.empty()) {
        throw std::invalid_argument("target_id must not be empty");
    }
    TargetKnowledgeState state;
    state.target_id = std::move(target_id);
    return state;
}

inline void apply_observation(TargetKnowledgeState& state, const ObservationEvidence& evidence) {
    if (state.target_id.empty()) {
        throw std::invalid_argument("target knowledge state requires a target_id");
    }
    if (!std::isfinite(evidence.duration_s) || evidence.duration_s <= 0.0) {
        throw std::invalid_argument("observation duration_s must be finite and positive");
    }
    if (!std::isfinite(evidence.confidence_gain) ||
        evidence.confidence_gain < 0.0 || evidence.confidence_gain > 1.0) {
        throw std::invalid_argument("confidence_gain must be finite and between 0 and 1");
    }
    if (!std::isfinite(evidence.instrument_resolution) ||
        evidence.instrument_resolution < 0.0 || evidence.instrument_resolution > 1.0) {
        throw std::invalid_argument("instrument_resolution must be finite and between 0 and 1");
    }
    if (!std::isfinite(state.confidence) || state.confidence < 0.0 || state.confidence > 1.0) {
        throw std::invalid_argument("existing confidence must be finite and between 0 and 1");
    }

    if (evidence.mode == ObservationMode::Passive) {
        state.passive_observation_s += evidence.duration_s;
        if (state.level == KnowledgeLevel::Unknown) {
            state.level = KnowledgeLevel::Observed;
        }
    } else {
        state.active_scan_s += evidence.duration_s;
        if (state.level == KnowledgeLevel::Unknown) {
            state.level = KnowledgeLevel::Observed;
        }
        if (!evidence.classification.empty()) {
            state.classification = evidence.classification;
            state.level = KnowledgeLevel::Characterized;
        }
    }

    state.confidence = std::min(1.0, state.confidence + evidence.confidence_gain);
    state.best_instrument_resolution =
        std::max(state.best_instrument_resolution, evidence.instrument_resolution);
}

}  // namespace everward::simulation
