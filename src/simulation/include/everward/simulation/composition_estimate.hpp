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

[[nodiscard]] inline CompositionEstimate make_composition_estimate(
    std::string material_id,
    double fraction,
    double uncertainty,
    double confidence) {
    if (material_id.find_first_not_of(" \t\r\n") == std::string::npos) {
        throw std::invalid_argument("composition estimate material_id must not be blank");
    }
    if (!std::isfinite(fraction) || fraction < 0.0 || fraction > 1.0) {
        throw std::invalid_argument("composition estimate fraction must be finite and between 0 and 1");
    }
    if (!std::isfinite(uncertainty) || uncertainty < 0.0 || uncertainty > 1.0) {
        throw std::invalid_argument("composition estimate uncertainty must be finite and between 0 and 1");
    }
    if (!std::isfinite(confidence) || confidence < 0.0 || confidence > 1.0) {
        throw std::invalid_argument("composition estimate confidence must be finite and between 0 and 1");
    }
    return CompositionEstimate{
        std::move(material_id),
        fraction,
        uncertainty,
        confidence,
    };
}

inline void validate_composition_estimates(const std::vector<CompositionEstimate>& estimates) {
    double total_fraction = 0.0;
    for (const auto& estimate : estimates) {
        (void)make_composition_estimate(
            estimate.material_id, estimate.fraction, estimate.uncertainty, estimate.confidence);
        total_fraction += estimate.fraction;
        if (!std::isfinite(total_fraction) || total_fraction > 1.0 + 1e-12) {
            throw std::invalid_argument("composition estimate fractions must not exceed 1 in aggregate");
        }
    }
}

}  // namespace everward::simulation
