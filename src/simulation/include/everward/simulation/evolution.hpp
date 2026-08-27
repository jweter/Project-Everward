#pragma once

#include "everward/simulation/types.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace everward::simulation {

enum class EvolutionDomain {
    Propulsion,
    Sensors,
    Computation,
    Thermal,
    Energy,
    Storage,
    Structure,
    Manipulation,
    Mining,
    Fabrication
};

struct EvolutionTrait {
    std::string id;
    std::string display_name;
    EvolutionDomain domain{EvolutionDomain::Structure};
    double maturity{1.0};
    std::vector<std::string> tags;
};

struct EvolutionContext {
    std::uint32_t generation{1};
    double computation_maturity{1.0};
    double fabrication_maturity{1.0};
    double science_maturity{1.0};
    double materials_maturity{1.0};
    std::size_t requested_option_limit{8};
    std::vector<EvolutionTrait> installed_traits;
};

struct EvolutionCandidate {
    std::string id;
    std::string title;
    std::string description;
    EvolutionDomain domain{EvolutionDomain::Structure};
    std::string source_trait_id;
    double source_maturity{1.0};
    double resulting_maturity{1.0};
    double engineering_distance{0.0};
    double mass_cost_factor{1.0};
    double power_cost_factor{1.0};
    double thermal_cost_factor{1.0};
    bool adds_new_capability{false};
    std::vector<std::string> grants_tags;

    // Refinements are intentionally uncapped. refinement_rank identifies the
    // next repeated step without imposing a maximum level, while the cost
    // scales make extreme specialization increasingly expensive rather than
    // forbidden. New-capability candidates keep the default values.
    std::uint64_t refinement_rank{0};
    double resource_cost_scale{1.0};
    double construction_time_scale{1.0};
};

struct EvolutionFrontier {
    std::vector<EvolutionCandidate> improve_existing;
    std::vector<EvolutionCandidate> add_new_capability;
};

class AdjacentEvolutionGenerator {
public:
    [[nodiscard]] EvolutionFrontier generate_frontier(const EvolutionContext& context) const {
        EvolutionFrontier frontier;

        for (const auto& trait : context.installed_traits) {
            append_refinement_candidates(trait, frontier.improve_existing);
        }
        append_adjacent_capability_candidates(context, frontier.add_new_capability);

        sort_candidates(frontier.improve_existing);
        sort_candidates(frontier.add_new_capability);

        const std::size_t option_budget = std::min(
            context.requested_option_limit,
            option_budget_from_computation(context.computation_maturity));

        // If both paths are currently reachable, always preserve at least one
        // visible choice in each category. The player is choosing between
        // "become better at what I already do" and "become able to do
        // something I cannot do yet"; refinements must not crowd all adjacent
        // inventions out of the frontier merely because their numeric step is
        // slightly smaller.
        if (!frontier.improve_existing.empty() &&
            !frontier.add_new_capability.empty() &&
            option_budget >= 2) {
            const std::size_t addition_budget = std::max<std::size_t>(1, option_budget / 3);
            const std::size_t refinement_budget = option_budget - addition_budget;
            resize_to_budget(frontier.improve_existing, refinement_budget);
            resize_to_budget(frontier.add_new_capability, addition_budget);
        } else if (!frontier.improve_existing.empty()) {
            resize_to_budget(frontier.improve_existing, option_budget);
            frontier.add_new_capability.clear();
        } else {
            frontier.improve_existing.clear();
            resize_to_budget(frontier.add_new_capability, option_budget);
        }

        return frontier;
    }

    [[nodiscard]] std::vector<EvolutionCandidate> generate(const EvolutionContext& context) const {
        EvolutionFrontier frontier = generate_frontier(context);
        std::vector<EvolutionCandidate> candidates;
        candidates.reserve(
            frontier.improve_existing.size() + frontier.add_new_capability.size());
        candidates.insert(
            candidates.end(),
            frontier.improve_existing.begin(),
            frontier.improve_existing.end());
        candidates.insert(
            candidates.end(),
            frontier.add_new_capability.begin(),
            frontier.add_new_capability.end());
        sort_candidates(candidates);
        return candidates;
    }

    [[nodiscard]] static EvolutionContext make_canonical_ev0001_context(const ProbeStateSnapshot& snapshot) {
        EvolutionContext context;
        context.generation = snapshot.generation;
        context.computation_maturity = 1.0;
        context.fabrication_maturity = 1.0;
        context.science_maturity = 1.0;
        context.materials_maturity = 1.0;
        context.requested_option_limit = 8;

        context.installed_traits = {
            {"propulsion.basic", "Basic propulsion", EvolutionDomain::Propulsion, 1.0, {"propulsion", "maneuvering"}},
            {"sensors.optical", "Optical sensor suite", EvolutionDomain::Sensors, 1.0, {"sensor", "optical"}},
            {"computation.core", "Generation-1 computation core", EvolutionDomain::Computation, 1.0, {"compute", "automation"}},
            {"thermal.radiator", "Basic thermal radiator", EvolutionDomain::Thermal, 1.0, {"thermal", "radiator"}},
            {"energy.storage", "Primary energy storage", EvolutionDomain::Energy, 1.0, {"energy", "storage"}},
            {"storage.cargo", "Material storage", EvolutionDomain::Storage, 1.0, {"cargo", "storage"}},
            {"structure.frame", "Primary structural frame", EvolutionDomain::Structure, 1.0, {"structure", "load-bearing"}},
        };
        return context;
    }

private:
    struct RefinementAxis {
        std::string_view suffix;
        std::string_view title;
        std::string_view description;
        double maturity_step;
        double mass_cost_factor;
        double power_cost_factor;
        double thermal_cost_factor;
    };

    [[nodiscard]] static std::size_t option_budget_from_computation(double maturity) {
        const double safe = std::max(1.0, maturity);
        const std::size_t bonus = static_cast<std::size_t>(std::floor(std::log2(safe) * 2.0));
        return 4 + bonus;
    }

    static void sort_candidates(std::vector<EvolutionCandidate>& candidates) {
        // Stable lexical ordering makes the result deterministic and keeps
        // save/replay behavior independent of container iteration order.
        std::stable_sort(candidates.begin(), candidates.end(), [](const auto& a, const auto& b) {
            if (a.engineering_distance != b.engineering_distance) {
                return a.engineering_distance < b.engineering_distance;
            }
            return a.id < b.id;
        });
    }

    static void resize_to_budget(
        std::vector<EvolutionCandidate>& candidates,
        std::size_t budget) {
        if (candidates.size() > budget) {
            candidates.resize(budget);
        }
    }

    [[nodiscard]] static double extreme_specialization_cost_scale(double source_maturity) noexcept {
        // No maximum level. Cost grows continuously and predictably instead of
        // turning into a hard cap. Linear growth is deliberately numerically
        // stable at very large maturity values; later economic balancing can
        // change the curve without changing the open-ended progression rule.
        const double excess = std::max(0.0, source_maturity - 1.0);
        return 1.0 + 0.50 * excess;
    }

    [[nodiscard]] static double extreme_specialization_time_scale(double source_maturity) noexcept {
        const double excess = std::max(0.0, source_maturity - 1.0);
        return 1.0 + 0.25 * excess;
    }

    [[nodiscard]] static const std::vector<RefinementAxis>& axes_for(EvolutionDomain domain) {
        static const std::vector<RefinementAxis> propulsion{
            {"efficiency", "Improve propulsion efficiency", "Reduce energy consumed for equivalent maneuvering work.", 0.08, 1.01, 0.94, 0.96},
            {"authority", "Increase maneuvering authority", "Increase available thrust and control authority with added structural and thermal burden.", 0.10, 1.06, 1.09, 1.08},
        };
        static const std::vector<RefinementAxis> sensors{
            {"sensitivity", "Increase sensor sensitivity", "Improve signal collection and detection without changing the underlying sensing modality.", 0.08, 1.02, 1.05, 1.03},
            {"resolution", "Improve sensor resolution", "Extract finer spatial or spectral detail from the installed sensor system.", 0.10, 1.03, 1.07, 1.05},
        };
        static const std::vector<RefinementAxis> computation{
            {"throughput", "Increase computation throughput", "Run more analysis, automation, and engineering evaluation concurrently.", 0.10, 1.02, 1.10, 1.10},
            {"efficiency", "Improve computation efficiency", "Perform more useful computation per unit of power and waste heat.", 0.08, 1.01, 0.94, 0.93},
        };
        static const std::vector<RefinementAxis> thermal{
            {"rejection", "Improve heat rejection", "Reject more waste heat while preserving the current radiator architecture.", 0.09, 1.04, 1.00, 0.91},
            {"control", "Improve thermal control precision", "Manage component temperatures more precisely under changing loads.", 0.07, 1.01, 1.03, 0.96},
        };
        static const std::vector<RefinementAxis> energy{
            {"density", "Increase stored-energy density", "Store more usable energy for similar system volume.", 0.08, 1.01, 1.00, 1.02},
            {"delivery", "Improve power delivery", "Supply high transient loads with lower distribution loss.", 0.09, 1.03, 0.97, 0.98},
        };
        static const std::vector<RefinementAxis> storage{
            {"capacity", "Increase material capacity", "Carry more harvested or fabricated material at the cost of additional structure.", 0.08, 1.08, 1.00, 1.00},
            {"handling", "Improve storage handling", "Reduce time and energy spent routing material through internal storage.", 0.07, 1.02, 0.97, 0.99},
        };
        static const std::vector<RefinementAxis> structure{
            {"strength", "Strengthen primary structure", "Increase load tolerance using a modestly heavier structural revision.", 0.09, 1.07, 1.00, 1.00},
            {"mass", "Reduce structural mass", "Preserve required loads with better geometry and material use.", 0.08, 0.95, 1.00, 1.00},
        };
        static const std::vector<RefinementAxis> manipulation{
            {"precision", "Improve manipulator precision", "Increase positioning accuracy and fine tool control.", 0.08, 1.02, 1.04, 1.02},
            {"force", "Increase manipulator force", "Handle larger loads with stronger actuators and joints.", 0.10, 1.06, 1.08, 1.06},
        };
        static const std::vector<RefinementAxis> mining{
            {"hardness", "Improve cutting-tool hardness", "Work harder materials with reduced tool wear.", 0.09, 1.03, 1.03, 1.04},
            {"rate", "Increase extraction rate", "Remove material faster with higher mechanical and thermal demand.", 0.10, 1.05, 1.09, 1.08},
        };
        static const std::vector<RefinementAxis> fabrication{
            {"precision", "Improve fabrication precision", "Manufacture tighter tolerances and more demanding successor components.", 0.09, 1.03, 1.07, 1.05},
            {"throughput", "Increase fabrication throughput", "Produce equivalent components faster at greater peak power demand.", 0.10, 1.05, 1.10, 1.08},
        };

        switch (domain) {
            case EvolutionDomain::Propulsion: return propulsion;
            case EvolutionDomain::Sensors: return sensors;
            case EvolutionDomain::Computation: return computation;
            case EvolutionDomain::Thermal: return thermal;
            case EvolutionDomain::Energy: return energy;
            case EvolutionDomain::Storage: return storage;
            case EvolutionDomain::Structure: return structure;
            case EvolutionDomain::Manipulation: return manipulation;
            case EvolutionDomain::Mining: return mining;
            case EvolutionDomain::Fabrication: return fabrication;
        }
        return structure;
    }

    static void append_refinement_candidates(
        const EvolutionTrait& trait,
        std::vector<EvolutionCandidate>& out) {
        for (const auto& axis : axes_for(trait.domain)) {
            EvolutionCandidate candidate;
            candidate.id = trait.id + "." + std::string(axis.suffix);
            candidate.title = std::string(axis.title);
            candidate.description = std::string(axis.description);
            candidate.domain = trait.domain;
            candidate.source_trait_id = trait.id;
            candidate.source_maturity = trait.maturity;
            candidate.resulting_maturity = trait.maturity + axis.maturity_step;
            candidate.engineering_distance = axis.maturity_step;
            candidate.mass_cost_factor = axis.mass_cost_factor;
            candidate.power_cost_factor = axis.power_cost_factor;
            candidate.thermal_cost_factor = axis.thermal_cost_factor;
            candidate.refinement_rank = static_cast<std::uint64_t>(
                std::floor(std::max(0.0, trait.maturity - 1.0) / axis.maturity_step)) + 1;
            candidate.resource_cost_scale =
                extreme_specialization_cost_scale(trait.maturity);
            candidate.construction_time_scale =
                extreme_specialization_time_scale(trait.maturity);
            out.push_back(std::move(candidate));
        }
    }

    [[nodiscard]] static bool has_tag(const EvolutionContext& context, std::string_view tag) {
        for (const auto& trait : context.installed_traits) {
            for (const auto& existing : trait.tags) {
                if (existing == tag) {
                    return true;
                }
            }
        }
        return false;
    }

    static void append_adjacent_capability_candidates(
        const EvolutionContext& context,
        std::vector<EvolutionCandidate>& out) {
        // New modalities are deliberately gated by existing nearby hardware
        // plus science/fabrication/material maturity. They are not random
        // jumps and therefore preserve the "one small generation at a time"
        // rule while still allowing genuinely new abilities to emerge.
        if (has_tag(context, "optical") && !has_tag(context, "infrared") &&
            context.science_maturity >= 1.10 && context.fabrication_maturity >= 1.05) {
            out.push_back({
                "sensors.infrared.add",
                "Add near-infrared sensing",
                "Extend the existing optical instrument family with cooled near-infrared detection.",
                EvolutionDomain::Sensors,
                "sensors.optical",
                1.0,
                1.0,
                0.12,
                1.04,
                1.10,
                1.12,
                true,
                {"sensor", "infrared"}
            });
        }

        if (has_tag(context, "structure") && has_tag(context, "automation") &&
            !has_tag(context, "manipulator") && context.fabrication_maturity >= 1.08) {
            out.push_back({
                "manipulation.basic.add",
                "Add a basic manipulator",
                "Add a low-force articulated arm that can position future tools and samples.",
                EvolutionDomain::Manipulation,
                "structure.frame",
                1.0,
                1.0,
                0.14,
                1.09,
                1.08,
                1.05,
                true,
                {"manipulator", "tool-mount"}
            });
        }

        if (has_tag(context, "manipulator") && has_tag(context, "tool-mount") &&
            !has_tag(context, "mining") && context.materials_maturity >= 1.10) {
            out.push_back({
                "mining.drill.add",
                "Add a basic drilling tool",
                "Equip the manipulator with a modest rotary cutting tool for accessible materials.",
                EvolutionDomain::Mining,
                "manipulation.basic",
                1.0,
                1.0,
                0.13,
                1.06,
                1.10,
                1.08,
                true,
                {"mining", "drill"}
            });
        }

        if (has_tag(context, "compute") && !has_tag(context, "design-simulation") &&
            context.computation_maturity >= 1.25 && context.science_maturity >= 1.15) {
            out.push_back({
                "computation.design-simulation.add",
                "Add onboard design simulation",
                "Use the stronger computation core to evaluate more successor designs before committing resources.",
                EvolutionDomain::Computation,
                "computation.core",
                context.computation_maturity,
                context.computation_maturity,
                0.11,
                1.02,
                1.08,
                1.08,
                true,
                {"compute", "design-simulation"}
            });
        }
    }
};

} // namespace everward::simulation
