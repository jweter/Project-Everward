#pragma once

#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace everward::simulation {

struct CompositionEstimate {
    std::string material_id{};
    double fraction{0.0};
    double uncertainty{1.0};
    double confidence{0.0};
};

[[nodiscard]] inline bool composition_estimate_is_valid(const CompositionEstimate& estimate) {
    return estimate.material_id.find_first_not_of(" \t\r\n") != std::string::npos &&
           std::isfinite(estimate.fraction) && estimate.fraction >= 0.0 && estimate.fraction <= 1.0 &&
           std::isfinite(estimate.uncertainty) && estimate.uncertainty >= 0.0 &&
           estimate.uncertainty <= 1.0 &&
           std::isfinite(estimate.confidence) && estimate.confidence >= 0.0 &&
           estimate.confidence <= 1.0;
}

inline void validate_composition_estimates(const std::vector<CompositionEstimate>& estimates) {
    double fraction_sum = 0.0;
    for (const auto& estimate : estimates) {
        if (!composition_estimate_is_valid(estimate)) {
            throw std::invalid_argument("composition estimate contains invalid evidence");
        }
        fraction_sum += estimate.fraction;
    }
    if (fraction_sum > 1.0 + 1e-12) {
        throw std::invalid_argument("composition estimate fractions must not exceed 1 in total");
    }
}

[[nodiscard]] inline CompositionEstimate make_composition_estimate(
    std::string material_id,
    double fraction,
    double uncertainty,
    double confidence) {
    CompositionEstimate estimate{std::move(material_id), fraction, uncertainty, confidence};
    if (!composition_estimate_is_valid(estimate)) {
        throw std::invalid_argument("composition estimate values must be finite, bounded, and material_id non-blank");
    }
    return estimate;
}

}  // namespace everward::simulation
