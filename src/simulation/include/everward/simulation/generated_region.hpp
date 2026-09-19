#pragma once

// Generated-world persistence contract (issue #270, the decomposed
// generated-world remainder of #167). Implements docs/SAVE_FORMAT.md's
// procedural-region rule (also stated in docs/ARCHITECTURE.md):
//
//   BaseRegion  = Generate(universe_seed, spatial_coordinate, algorithm_version)
//   LoadedRegion = BaseRegion + PersistedObservations + PersistedModifications
//                  + ActiveEntities
//
// No live procedural galaxy/region generator is wired into gameplay yet --
// Phase 2 remains a single hardcoded vertical slice, and nothing here adds a
// new player-facing mechanic. This header establishes the data contract and
// a first deterministic algorithm_version so a future generation/exploration
// system has an authoritative, already-tested foundation to build on. That
// is the same "data-only, ahead of the mechanic" precedent probe_lineage.hpp
// already set: persistence records world facts an authoritative system will
// someday create, rather than inventing that system here.

#include "everward/simulation/science_knowledge.hpp"
#include "everward/simulation/types.hpp"

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace everward::simulation {

// A coarse spatial grid cell. Exact scale/mapping to in-simulation meters is
// deliberately undefined here -- docs/ARCHITECTURE.md's spatial-model
// prototype is still open -- this is only the stable coordinate identity a
// region is generated and persisted against.
struct RegionCoordinate {
    std::int64_t x{0};
    std::int64_t y{0};
    std::int64_t z{0};
};

[[nodiscard]] inline bool operator==(const RegionCoordinate& a, const RegionCoordinate& b) noexcept {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

// Stable identity derived purely from coordinate -- never a counter or
// transient engine object address (docs/ARCHITECTURE.md's "Entity identity"
// rule: "IDs must survive save/load and must not be derived from transient
// engine object addresses").
[[nodiscard]] inline std::string derive_region_id(const RegionCoordinate& coordinate) {
    return "region:" + std::to_string(coordinate.x) + ":" + std::to_string(coordinate.y) + ":" +
           std::to_string(coordinate.z);
}

namespace detail {

// SplitMix64 (public domain; Vigna 2015). Used as this module's entire PRNG
// -- both to mix (universe_seed, coordinate, algorithm_version) into a
// starting state and to produce the output stream -- rather than std::hash
// or <random>'s engines. std::hash's output is implementation-defined and
// not guaranteed identical across standard library implementations, and
// this codebase's portable ctest suite (built here with whatever toolchain
// this environment has) must derive byte-identical region content from the
// same inputs as the packaged Windows/Unreal build, or "region =
// Generate(...)" stops being reproducible across the environments that
// actually need to agree. A small, fully-specified-by-this-file algorithm
// sidesteps that risk entirely.
[[nodiscard]] inline std::uint64_t splitmix64_next(std::uint64_t& state) noexcept {
    state += 0x9E3779B97F4A7C15ULL;
    std::uint64_t result = state;
    result = (result ^ (result >> 30)) * 0xBF58476D1CE4E5B9ULL;
    result = (result ^ (result >> 27)) * 0x94D049BB133111EBULL;
    return result ^ (result >> 31);
}

[[nodiscard]] inline std::uint64_t derive_region_seed(
        std::int64_t universe_seed,
        const RegionCoordinate& coordinate,
        int algorithm_version) {
    std::uint64_t state = static_cast<std::uint64_t>(universe_seed);
    const std::uint64_t inputs[4] = {
        static_cast<std::uint64_t>(coordinate.x),
        static_cast<std::uint64_t>(coordinate.y),
        static_cast<std::uint64_t>(coordinate.z),
        static_cast<std::uint64_t>(algorithm_version),
    };
    for (const std::uint64_t input : inputs) {
        state ^= input;
        state = splitmix64_next(state);
    }
    return state;
}

} // namespace detail

// Deterministic minor-body content for one as-yet-unvisited region. Reuses
// StaticSphereBody -- the same engine-independent physical-body type a
// probe's own registered targets already use -- instead of inventing a
// parallel entity shape: a generated region's minor bodies are exactly that
// kind of object, just not yet registered onto any specific probe's
// runtime. center_m is region-local (meters from the region's own nominal
// origin), matching StaticSphereBody's existing local-space convention.
//
// Only algorithm_version 1 exists today. A future algorithm_version is a
// genuinely different generator, not a tuning tweak to this one -- bump the
// version and add a new branch rather than changing this branch's output,
// or every region already persisted under version 1 would silently
// regenerate different baseline content on load.
[[nodiscard]] inline std::vector<StaticSphereBody> generate_region_baseline(
        std::int64_t universe_seed,
        const RegionCoordinate& coordinate,
        int algorithm_version) {
    if (algorithm_version != 1) {
        throw std::runtime_error(
            "unsupported region generation_algorithm_version: " + std::to_string(algorithm_version));
    }

    const std::string region_id = derive_region_id(coordinate);
    std::uint64_t state = detail::derive_region_seed(universe_seed, coordinate, algorithm_version);

    const auto next_unit_double = [&state]() -> double {
        // Top 53 bits of a 64-bit draw give a double uniformly in [0, 1).
        return static_cast<double>(detail::splitmix64_next(state) >> 11) * (1.0 / 9007199254740992.0);
    };

    static constexpr const char* kMaterialIds[] = {
        "iron_bearing_silicate_regolith", "carbonaceous_regolith", "raw_regolith", ""};

    const int entity_count = 3 + static_cast<int>(next_unit_double() * 4.0); // [3, 6]
    std::vector<StaticSphereBody> entities;
    entities.reserve(static_cast<std::size_t>(entity_count));
    for (int i = 0; i < entity_count; ++i) {
        StaticSphereBody body;
        body.body_id = region_id + "#body-" + std::to_string(i);
        body.center_m = Vector3d{
            (next_unit_double() - 0.5) * 1000.0,
            (next_unit_double() - 0.5) * 1000.0,
            (next_unit_double() - 0.5) * 1000.0,
        };
        body.radius_m = 0.5 + next_unit_double() * 4.5;
        const int material_index = static_cast<int>(next_unit_double() * 4.0); // [0, 3]
        body.material_id = kMaterialIds[material_index];
        body.sample_mass_kg = body.material_id.empty() ? 0.0 : 10.0 + next_unit_double() * 90.0;
        entities.push_back(std::move(body));
    }
    return entities;
}

// A generated entity's departure from its deterministic baseline: either
// entirely removed (consumed/destroyed) or its remaining sampleable mass
// reduced below what generation alone would produce. No gameplay system
// creates these yet -- mining/manipulator collection today only depletes a
// probe's own already-registered static bodies, not a not-yet-generated
// region -- this is the same "data contract ahead of the mechanic"
// precedent as ProbeLineageRecord.
struct RegionEntityModificationRecord {
    std::string body_id{};
    bool removed{false};
    std::optional<double> remaining_sample_mass_kg{};
};

// One region's persisted delta against its own deterministic baseline: the
// PersistedObservations/PersistedModifications half of this file's header
// comment's LoadedRegion rule. generate_region_baseline() above is
// Generate(...); reconstruct_region_entities() below recombines them.
// ActiveEntities (autonomous agents or other probes physically present in
// the region) do not exist in the simulation yet and are therefore not
// represented here, matching save_data.hpp's own precedent of not
// inventing persistence for mechanics that do not exist.
//
// observations reuses science_knowledge.hpp's TargetKnowledgeState (the
// same player-retained-knowledge model a probe's own scan targets already
// use) keyed by the generated body_id it describes, rather than inventing a
// second knowledge representation.
struct GeneratedRegionRecord {
    RegionCoordinate coordinate{};
    std::string region_id{};
    std::unordered_map<std::string, TargetKnowledgeState> observations{};
    std::vector<RegionEntityModificationRecord> modifications{};
};

// Fails closed on exactly the tamper/corruption classes a hand-edited save
// could introduce: a region_id inconsistent with its own coordinate, a
// duplicated region, or an observation/modification naming a body_id that
// generate_region_baseline() (the authoritative Generate(...) function)
// would never produce for that region -- the concrete tie-back to this
// file's LoadedRegion rule, mirroring probe_lineage.hpp's
// validate_probe_lineages precedent.
inline void validate_generated_regions(
        const std::vector<GeneratedRegionRecord>& regions,
        std::int64_t universe_seed,
        int generation_algorithm_version) {
    std::unordered_set<std::string> seen_region_ids;
    for (const auto& region : regions) {
        if (region.region_id.empty()) {
            throw std::runtime_error("persisted region_id must not be empty");
        }
        if (region.region_id != derive_region_id(region.coordinate)) {
            throw std::runtime_error("persisted region_id does not match its own coordinate: " + region.region_id);
        }
        if (!seen_region_ids.insert(region.region_id).second) {
            throw std::runtime_error("duplicate persisted region_id: " + region.region_id);
        }

        const std::vector<StaticSphereBody> baseline =
            generate_region_baseline(universe_seed, region.coordinate, generation_algorithm_version);
        std::unordered_map<std::string, const StaticSphereBody*> baseline_by_id;
        baseline_by_id.reserve(baseline.size());
        for (const auto& body : baseline) {
            baseline_by_id.emplace(body.body_id, &body);
        }

        for (const auto& [entity_id, knowledge] : region.observations) {
            if (entity_id.empty()) {
                throw std::runtime_error("region observation entity_id must not be empty");
            }
            if (!baseline_by_id.contains(entity_id)) {
                throw std::runtime_error(
                    "region observation references an entity generation would never produce: " + entity_id +
                    " in " + region.region_id);
            }
            if (knowledge.target_id != entity_id) {
                throw std::runtime_error(
                    "region observation target_id must match its own map key: " + entity_id);
            }
        }

        std::unordered_set<std::string> modified_body_ids;
        for (const auto& modification : region.modifications) {
            if (modification.body_id.empty()) {
                throw std::runtime_error("region modification body_id must not be empty");
            }
            const auto baseline_it = baseline_by_id.find(modification.body_id);
            if (baseline_it == baseline_by_id.end()) {
                throw std::runtime_error(
                    "region modification references an entity generation would never produce: " +
                    modification.body_id + " in " + region.region_id);
            }
            if (!modified_body_ids.insert(modification.body_id).second) {
                throw std::runtime_error("duplicate region modification for body_id: " + modification.body_id);
            }
            if (modification.remaining_sample_mass_kg.has_value()) {
                const double remaining = *modification.remaining_sample_mass_kg;
                const double baseline_mass = baseline_it->second->sample_mass_kg;
                // NaN also fails the first comparison, so this rejects a
                // non-finite value too rather than silently accepting one.
                if (!(remaining >= 0.0) || remaining > baseline_mass) {
                    throw std::runtime_error(
                        "region modification remaining_sample_mass_kg out of range for body_id: " +
                        modification.body_id);
                }
            }
        }
    }
}

// Recombines a region's deterministic baseline with its persisted
// modifications: the concrete LoadedRegion computation (observations do not
// alter physical entities, only what the player is known to know about
// them, so they do not participate here). Trusts that the caller (normally
// save_data.hpp's parser) has already run validate_generated_regions over
// the full save.
[[nodiscard]] inline std::vector<StaticSphereBody> reconstruct_region_entities(
        std::int64_t universe_seed,
        int generation_algorithm_version,
        const GeneratedRegionRecord& region) {
    std::vector<StaticSphereBody> baseline =
        generate_region_baseline(universe_seed, region.coordinate, generation_algorithm_version);

    std::unordered_map<std::string, const RegionEntityModificationRecord*> modification_by_id;
    modification_by_id.reserve(region.modifications.size());
    for (const auto& modification : region.modifications) {
        modification_by_id.emplace(modification.body_id, &modification);
    }

    std::vector<StaticSphereBody> reconstructed;
    reconstructed.reserve(baseline.size());
    for (auto& entity : baseline) {
        const auto it = modification_by_id.find(entity.body_id);
        if (it == modification_by_id.end()) {
            reconstructed.push_back(std::move(entity));
            continue;
        }
        if (it->second->removed) {
            continue;
        }
        if (it->second->remaining_sample_mass_kg.has_value()) {
            entity.sample_mass_kg = *it->second->remaining_sample_mass_kg;
        }
        reconstructed.push_back(std::move(entity));
    }
    return reconstructed;
}

} // namespace everward::simulation
