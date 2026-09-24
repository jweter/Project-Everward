#pragma once

#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>

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
    if (material_id.empty()) {
        throw std::invalid_argument("composition estimate material_id must not be empty");
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

}  // namespace everward::simulation
