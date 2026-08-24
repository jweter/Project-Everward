#include "everward/simulation/evolution.hpp"

#undef NDEBUG
#include <cassert>
#include <iostream>
#include <string>

using everward::simulation::AdjacentEvolutionGenerator;
using everward::simulation::EvolutionCandidate;
using everward::simulation::EvolutionContext;
using everward::simulation::EvolutionDomain;
using everward::simulation::EvolutionTrait;
using everward::simulation::ProbeStateSnapshot;

static bool contains_id(const std::vector<EvolutionCandidate>& candidates, const std::string& id) {
    for (const auto& candidate : candidates) {
        if (candidate.id == id) {
            return true;
        }
    }
    return false;
}

int main() {
    AdjacentEvolutionGenerator generator;
    ProbeStateSnapshot snapshot;

    // Canonical Generation 1 produces only small adjacent refinements.
    {
        const EvolutionContext context = AdjacentEvolutionGenerator::make_canonical_ev0001_context(snapshot);
        const auto candidates = generator.generate(context);
        assert(!candidates.empty());
        assert(candidates.size() <= context.requested_option_limit);
        for (const auto& candidate : candidates) {
            assert(candidate.engineering_distance > 0.0);
            assert(candidate.engineering_distance <= 0.15);
            assert(candidate.resulting_maturity >= candidate.source_maturity);
        }
    }

    // Generation output is deterministic for the same parent/context.
    {
        const EvolutionContext context = AdjacentEvolutionGenerator::make_canonical_ev0001_context(snapshot);
        const auto first = generator.generate(context);
        const auto second = generator.generate(context);
        assert(first.size() == second.size());
        for (std::size_t i = 0; i < first.size(); ++i) {
            assert(first[i].id == second[i].id);
            assert(first[i].engineering_distance == second[i].engineering_distance);
        }
    }

    // New sensor modalities require the adjacent parent hardware and enough
    // science/fabrication maturity. They cannot appear from nowhere.
    {
        EvolutionContext blocked = AdjacentEvolutionGenerator::make_canonical_ev0001_context(snapshot);
        blocked.requested_option_limit = 32;
        blocked.science_maturity = 1.20;
        blocked.fabrication_maturity = 1.10;
        auto available = generator.generate(blocked);
        assert(contains_id(available, "sensors.infrared.add"));

        blocked.installed_traits.erase(
            std::remove_if(
                blocked.installed_traits.begin(),
                blocked.installed_traits.end(),
                [](const EvolutionTrait& trait) { return trait.id == "sensors.optical"; }),
            blocked.installed_traits.end());
        auto missing_parent = generator.generate(blocked);
        assert(!contains_id(missing_parent, "sensors.infrared.add"));
    }

    // A manipulator can open an adjacent mining path, but a drill does not
    // appear without both a manipulator/tool mount and sufficient materials.
    {
        EvolutionContext context;
        context.requested_option_limit = 32;
        context.computation_maturity = 2.0;
        context.fabrication_maturity = 1.2;
        context.science_maturity = 1.2;
        context.materials_maturity = 1.2;
        context.installed_traits = {
            {"computation.core", "Core", EvolutionDomain::Computation, 1.5, {"compute", "automation"}},
            {"manipulation.basic", "Manipulator", EvolutionDomain::Manipulation, 1.0, {"manipulator", "tool-mount"}},
        };
        const auto candidates = generator.generate(context);
        assert(contains_id(candidates, "mining.drill.add"));

        context.materials_maturity = 1.0;
        const auto blocked = generator.generate(context);
        assert(!contains_id(blocked, "mining.drill.add"));
    }

    // More capable computation increases how many candidate directions can
    // be evaluated, but never changes the small-step engineering bound.
    {
        EvolutionContext low;
        low.requested_option_limit = 20;
        low.computation_maturity = 1.0;
        low.installed_traits = {
            {"propulsion.basic", "Propulsion", EvolutionDomain::Propulsion, 1.0, {"propulsion"}},
            {"sensors.optical", "Sensors", EvolutionDomain::Sensors, 1.0, {"sensor", "optical"}},
            {"computation.core", "Core", EvolutionDomain::Computation, 1.0, {"compute", "automation"}},
            {"thermal.radiator", "Thermal", EvolutionDomain::Thermal, 1.0, {"thermal"}},
            {"structure.frame", "Structure", EvolutionDomain::Structure, 1.0, {"structure"}},
        };
        EvolutionContext high = low;
        high.computation_maturity = 8.0;

        const auto low_candidates = generator.generate(low);
        const auto high_candidates = generator.generate(high);
        assert(high_candidates.size() > low_candidates.size());
        for (const auto& candidate : high_candidates) {
            assert(candidate.engineering_distance <= 0.15);
        }
    }

    std::cout << "Adjacent evolution generator tests passed\n";
    return 0;
}
