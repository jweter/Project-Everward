#include "everward/simulation/science_knowledge.hpp"
#include "everward/simulation/composition_estimate.hpp"

#include <cassert>
#include <cmath>
#include <limits>
#include <stdexcept>

using namespace everward::simulation;

namespace {

template <typename Fn>
bool throws_invalid_argument(Fn&& fn) {
    try {
        fn();
    } catch (const std::invalid_argument&) {
        return true;
    }
    return false;
}

}  // namespace

int main() {
    {
        const auto state = make_unknown_target_knowledge("sample-001");
        assert(state.target_id == "sample-001");
        assert(state.level == KnowledgeLevel::Unknown);
        assert(state.passive_observation_s == 0.0);
        assert(state.active_scan_s == 0.0);
        assert(state.confidence == 0.0);
        assert(state.classification.empty());
    }

    {
        auto state = make_unknown_target_knowledge("sample-002");
        apply_observation(state, {ObservationMode::Passive, 10.0, 0.10, 0.20, ""});
        assert(state.level == KnowledgeLevel::Observed);
        assert(state.passive_observation_s == 10.0);
        assert(state.active_scan_s == 0.0);
        assert(state.confidence == 0.10);

        apply_observation(state, {ObservationMode::Passive, 20.0, 0.15, 0.20, ""});
        assert(state.level == KnowledgeLevel::Observed);
        assert(state.passive_observation_s == 30.0);
        assert(state.confidence == 0.25);
    }

    {
        auto state = make_unknown_target_knowledge("sample-003");
        apply_observation(state, {ObservationMode::ActiveScan, 5.0, 0.35, 0.60, "silicate-rich rock"});
        assert(state.level == KnowledgeLevel::Characterized);
        assert(state.active_scan_s == 5.0);
        assert(state.confidence == 0.35);
        assert(state.best_instrument_resolution == 0.60);
        assert(state.classification == "silicate-rich rock");

        apply_observation(state, {ObservationMode::ActiveScan, 8.0, 0.25, 0.40, ""});
        assert(state.level == KnowledgeLevel::Characterized);
        assert(state.active_scan_s == 13.0);
        assert(state.confidence == 0.60);
        assert(state.best_instrument_resolution == 0.60);
        assert(state.classification == "silicate-rich rock");
    }

    // Slice 11 requires repeated/longer observations to improve knowledge.
    // The observing instrument remains authoritative for confidence gain and
    // resolution; this state container only accumulates supplied evidence.
    {
        auto state = make_unknown_target_knowledge("sample-repeat");
        apply_observation(state, {ObservationMode::ActiveScan, 2.0, 0.20, 0.35, ""});
        const auto first_confidence = state.confidence;
        const auto first_scan_s = state.active_scan_s;
        assert(state.level == KnowledgeLevel::Observed);

        apply_observation(state, {ObservationMode::ActiveScan, 6.0, 0.30, 0.70, "metal-rich sample"});
        assert(state.active_scan_s > first_scan_s);
        assert(state.confidence > first_confidence);
        assert(state.confidence == 0.50);
        assert(state.best_instrument_resolution == 0.70);
        assert(state.level == KnowledgeLevel::Characterized);
        assert(state.classification == "metal-rich sample");
    }

    {
        auto state = make_unknown_target_knowledge("sample-004");
        apply_observation(state, {ObservationMode::Passive, 1.0, 0.8, 0.3, ""});
        apply_observation(state, {ObservationMode::ActiveScan, 1.0, 0.8, 0.9, "metallic body"});
        assert(state.confidence == 1.0);
        assert(state.level == KnowledgeLevel::Characterized);
        assert(state.best_instrument_resolution == 0.9);
    }

    {
        auto state = make_unknown_target_knowledge("sample-resolution");
        apply_observation(state, {ObservationMode::ActiveScan, 2.0, 0.30, 0.80, "iron-rich regolith"});
        apply_observation(state, {ObservationMode::ActiveScan, 4.0, 0.20, 0.30, "silicate-rich regolith"});
        assert(state.classification == "iron-rich regolith");
        assert(state.best_instrument_resolution == 0.80);
        assert(state.active_scan_s == 6.0);
        assert(state.confidence == 0.50);

        apply_observation(state, {ObservationMode::ActiveScan, 1.0, 0.10, 0.90, "iron-nickel regolith"});
        assert(state.classification == "iron-nickel regolith");
        assert(state.best_instrument_resolution == 0.90);
        assert(state.confidence == 0.60);
    }

    {
        auto state = make_unknown_target_knowledge("sample-005");
        assert(throws_invalid_argument([&] {
            apply_observation(state, {ObservationMode::Passive, -1.0, 0.1, 0.1, ""});
        }));
        assert(throws_invalid_argument([&] {
            apply_observation(
                state,
                {ObservationMode::Passive, 1.0, std::numeric_limits<double>::quiet_NaN(), 0.1, ""});
        }));
        assert(throws_invalid_argument([&] {
            apply_observation(state, {ObservationMode::ActiveScan, 1.0, 0.1, 1.1, ""});
        }));
        state.best_instrument_resolution = std::numeric_limits<double>::quiet_NaN();
        assert(throws_invalid_argument([&] {
            apply_observation(state, {ObservationMode::Passive, 1.0, 0.1, 0.1, ""});
        }));
        assert(state.level == KnowledgeLevel::Unknown);
        assert(state.confidence == 0.0);
    }

    assert(throws_invalid_argument([] { (void)make_unknown_target_knowledge(""); }));

    // Composition estimates are explicitly uncertain observations, not simulation
    // ground truth. Preserve the supplied evidence exactly and fail closed on
    // malformed probabilities so later science systems cannot silently invent
    // certainty.
    {
        const auto estimate = make_composition_estimate("iron", 0.62, 0.18, 0.74);
        assert(estimate.material_id == "iron");
        assert(estimate.fraction == 0.62);
        assert(estimate.uncertainty == 0.18);
        assert(estimate.confidence == 0.74);
    }

    assert(throws_invalid_argument([] {
        (void)make_composition_estimate("", 0.5, 0.2, 0.8);
    }));
    assert(throws_invalid_argument([] {
        (void)make_composition_estimate("   ", 0.5, 0.2, 0.8);
    }));
    assert(throws_invalid_argument([] {
        (void)make_composition_estimate("iron", -0.01, 0.2, 0.8);
    }));
    assert(throws_invalid_argument([] {
        (void)make_composition_estimate("iron", 0.5, 1.01, 0.8);
    }));
    assert(throws_invalid_argument([] {
        (void)make_composition_estimate(
            "iron", 0.5, 0.2, std::numeric_limits<double>::quiet_NaN());
    }));
    assert(throws_invalid_argument([] {
        (void)make_composition_estimate(
            "iron", std::numeric_limits<double>::infinity(), 0.2, 0.8);
    }));
    {
        const std::vector<CompositionEstimate> plausible{
            make_composition_estimate("iron", 0.62, 0.18, 0.74),
            make_composition_estimate("silicate", 0.28, 0.22, 0.68),
        };
        validate_composition_estimates(plausible);
    }
    assert(throws_invalid_argument([] {
        const std::vector<CompositionEstimate> impossible{
            make_composition_estimate("iron", 0.70, 0.10, 0.90),
            make_composition_estimate("silicate", 0.40, 0.10, 0.90),
        };
        validate_composition_estimates(impossible);
    }));
    return 0;
}
