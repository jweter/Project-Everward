#include "everward/simulation/science_knowledge.hpp"

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

    {
        auto state = make_unknown_target_knowledge("sample-004");
        apply_observation(state, {ObservationMode::Passive, 1.0, 0.8, 0.3, ""});
        apply_observation(state, {ObservationMode::ActiveScan, 1.0, 0.8, 0.9, "metallic body"});
        assert(state.confidence == 1.0);
        assert(state.level == KnowledgeLevel::Characterized);
        assert(state.best_instrument_resolution == 0.9);
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
        assert(state.level == KnowledgeLevel::Unknown);
        assert(state.confidence == 0.0);
    }

    assert(throws_invalid_argument([] { (void)make_unknown_target_knowledge(""); }));
    return 0;
}
