#include "everward/simulation/generated_region.hpp"

#undef NDEBUG
#include <cassert>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

namespace {

using everward::simulation::GeneratedRegionRecord;
using everward::simulation::RegionCoordinate;
using everward::simulation::RegionEntityModificationRecord;
using everward::simulation::StaticSphereBody;
using everward::simulation::TargetKnowledgeState;
using everward::simulation::derive_region_id;
using everward::simulation::generate_region_baseline;
using everward::simulation::make_unknown_target_knowledge;
using everward::simulation::reconstruct_region_entities;
using everward::simulation::validate_generated_regions;

bool bodies_equal(const StaticSphereBody& a, const StaticSphereBody& b) {
    return a.body_id == b.body_id && a.center_m.x == b.center_m.x && a.center_m.y == b.center_m.y &&
           a.center_m.z == b.center_m.z && a.radius_m == b.radius_m && a.material_id == b.material_id &&
           a.sample_mass_kg == b.sample_mass_kg;
}

bool baselines_equal(const std::vector<StaticSphereBody>& a, const std::vector<StaticSphereBody>& b) {
    if (a.size() != b.size()) {
        return false;
    }
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (!bodies_equal(a[i], b[i])) {
            return false;
        }
    }
    return true;
}

void test_derive_region_id_is_deterministic_and_distinct() {
    const RegionCoordinate a{1, 2, 3};
    const RegionCoordinate b{1, 2, 3};
    const RegionCoordinate c{1, 2, 4};
    assert(derive_region_id(a) == derive_region_id(b));
    assert(derive_region_id(a) != derive_region_id(c));
    assert(!derive_region_id(a).empty());
}

void test_generation_is_deterministic_for_identical_inputs() {
    const RegionCoordinate coordinate{7, -3, 100};
    const auto first = generate_region_baseline(42, coordinate, 1);
    const auto second = generate_region_baseline(42, coordinate, 1);
    assert(baselines_equal(first, second));
    assert(!first.empty());
}

void test_generation_entity_count_within_expected_bounds_and_unique_ids() {
    const RegionCoordinate coordinate{0, 0, 0};
    const auto entities = generate_region_baseline(1234, coordinate, 1);
    assert(entities.size() >= 3 && entities.size() <= 6);

    std::unordered_set<std::string> ids;
    for (const auto& entity : entities) {
        assert(!entity.body_id.empty());
        assert(ids.insert(entity.body_id).second);
        assert(entity.radius_m > 0.0);
        assert(entity.sample_mass_kg >= 0.0);
        // A body with no known composition is never manipulator-collectible,
        // matching StaticSphereBody's own documented convention elsewhere.
        if (entity.material_id.empty()) {
            assert(entity.sample_mass_kg == 0.0);
        }
    }
}

void test_generation_differs_across_coordinate() {
    const auto a = generate_region_baseline(42, RegionCoordinate{0, 0, 0}, 1);
    const auto b = generate_region_baseline(42, RegionCoordinate{0, 0, 1}, 1);
    // body_id is always prefixed by the region's own derived id, so distinct
    // coordinates are guaranteed to produce distinct body_ids regardless of
    // any coincidental agreement in count/placement/material.
    assert(!a.empty() && !b.empty());
    assert(a.front().body_id != b.front().body_id);
}

void test_generation_differs_across_seed() {
    const RegionCoordinate coordinate{5, 5, 5};
    const auto a = generate_region_baseline(1, coordinate, 1);
    const auto b = generate_region_baseline(2, coordinate, 1);
    // Same coordinate, so body_id prefixes match; a different seed must
    // still change the generated content itself.
    assert(!baselines_equal(a, b));
}

void test_generation_rejects_unsupported_algorithm_version() {
    for (const int version : {-1, 0, 2}) {
        bool threw = false;
        try {
            (void)generate_region_baseline(1, RegionCoordinate{0, 0, 0}, version);
        } catch (const std::runtime_error&) {
            threw = true;
        }
        assert(threw);
    }
}

void test_reconstruct_matches_baseline_with_no_modifications() {
    const RegionCoordinate coordinate{9, 1, -4};
    GeneratedRegionRecord region;
    region.coordinate = coordinate;
    region.region_id = derive_region_id(coordinate);

    const auto baseline = generate_region_baseline(77, coordinate, 1);
    const auto reconstructed = reconstruct_region_entities(77, 1, region);
    assert(baselines_equal(baseline, reconstructed));
}

void test_reconstruct_excludes_removed_and_overrides_remaining_mass() {
    const RegionCoordinate coordinate{2, 2, 2};
    const auto baseline = generate_region_baseline(555, coordinate, 1);
    assert(baseline.size() >= 3);

    GeneratedRegionRecord region;
    region.coordinate = coordinate;
    region.region_id = derive_region_id(coordinate);
    region.modifications.push_back(RegionEntityModificationRecord{baseline.at(0).body_id, true, std::nullopt});
    const double reduced = baseline.at(1).sample_mass_kg > 1.0 ? baseline.at(1).sample_mass_kg - 1.0 : 0.0;
    region.modifications.push_back(
        RegionEntityModificationRecord{baseline.at(1).body_id, false, std::make_optional(reduced)});

    const auto reconstructed = reconstruct_region_entities(555, 1, region);
    assert(reconstructed.size() == baseline.size() - 1);
    for (const auto& entity : reconstructed) {
        assert(entity.body_id != baseline.at(0).body_id);
        if (entity.body_id == baseline.at(1).body_id) {
            assert(entity.sample_mass_kg == reduced);
        }
    }
}

void test_validate_accepts_consistent_regions() {
    const RegionCoordinate coordinate{3, -3, 3};
    const auto baseline = generate_region_baseline(999, coordinate, 1);

    GeneratedRegionRecord region;
    region.coordinate = coordinate;
    region.region_id = derive_region_id(coordinate);
    region.observations.emplace(baseline.at(0).body_id, make_unknown_target_knowledge(baseline.at(0).body_id));
    region.modifications.push_back(RegionEntityModificationRecord{baseline.at(0).body_id, false, std::nullopt});

    // Must not throw.
    validate_generated_regions({region}, 999, 1);
}

void test_validate_rejects_region_id_mismatched_with_coordinate() {
    const RegionCoordinate coordinate{4, 4, 4};
    GeneratedRegionRecord region;
    region.coordinate = coordinate;
    region.region_id = "region:not-the-real-id";

    bool threw = false;
    try {
        validate_generated_regions({region}, 1, 1);
    } catch (const std::runtime_error& error) {
        threw = std::string(error.what()).find("does not match its own coordinate") != std::string::npos;
    }
    assert(threw);
}

void test_validate_rejects_empty_region_id() {
    GeneratedRegionRecord region;
    region.coordinate = RegionCoordinate{1, 1, 1};
    region.region_id = "";

    bool threw = false;
    try {
        validate_generated_regions({region}, 1, 1);
    } catch (const std::runtime_error& error) {
        threw = std::string(error.what()).find("region_id must not be empty") != std::string::npos;
    }
    assert(threw);
}

void test_validate_rejects_duplicate_region_id() {
    const RegionCoordinate coordinate{6, 6, 6};
    GeneratedRegionRecord region;
    region.coordinate = coordinate;
    region.region_id = derive_region_id(coordinate);

    bool threw = false;
    try {
        validate_generated_regions({region, region}, 1, 1);
    } catch (const std::runtime_error& error) {
        threw = std::string(error.what()).find("duplicate persisted region_id") != std::string::npos;
    }
    assert(threw);
}

void test_validate_rejects_observation_referencing_unknown_entity() {
    const RegionCoordinate coordinate{8, 8, 8};
    GeneratedRegionRecord region;
    region.coordinate = coordinate;
    region.region_id = derive_region_id(coordinate);
    region.observations.emplace("not-a-real-body", make_unknown_target_knowledge("not-a-real-body"));

    bool threw = false;
    try {
        validate_generated_regions({region}, 1, 1);
    } catch (const std::runtime_error& error) {
        threw = std::string(error.what()).find("entity generation would never produce") != std::string::npos;
    }
    assert(threw);
}

void test_validate_rejects_observation_target_id_mismatched_with_map_key() {
    const RegionCoordinate coordinate{10, 10, 10};
    const auto baseline = generate_region_baseline(1, coordinate, 1);
    GeneratedRegionRecord region;
    region.coordinate = coordinate;
    region.region_id = derive_region_id(coordinate);
    // Map key is a real generated body_id, but the embedded target_id names
    // a different one -- a hand-edited-save inconsistency.
    region.observations.emplace(baseline.at(0).body_id, make_unknown_target_knowledge("some-other-id"));

    bool threw = false;
    try {
        validate_generated_regions({region}, 1, 1);
    } catch (const std::runtime_error& error) {
        threw = std::string(error.what()).find("must match its own map key") != std::string::npos;
    }
    assert(threw);
}

void test_validate_rejects_modification_referencing_unknown_entity() {
    const RegionCoordinate coordinate{11, 11, 11};
    GeneratedRegionRecord region;
    region.coordinate = coordinate;
    region.region_id = derive_region_id(coordinate);
    region.modifications.push_back(RegionEntityModificationRecord{"not-a-real-body", true, std::nullopt});

    bool threw = false;
    try {
        validate_generated_regions({region}, 1, 1);
    } catch (const std::runtime_error& error) {
        threw = std::string(error.what()).find("entity generation would never produce") != std::string::npos;
    }
    assert(threw);
}

void test_validate_rejects_duplicate_modification() {
    const RegionCoordinate coordinate{12, 12, 12};
    const auto baseline = generate_region_baseline(1, coordinate, 1);
    GeneratedRegionRecord region;
    region.coordinate = coordinate;
    region.region_id = derive_region_id(coordinate);
    region.modifications.push_back(RegionEntityModificationRecord{baseline.at(0).body_id, true, std::nullopt});
    region.modifications.push_back(RegionEntityModificationRecord{baseline.at(0).body_id, false, std::nullopt});

    bool threw = false;
    try {
        validate_generated_regions({region}, 1, 1);
    } catch (const std::runtime_error& error) {
        threw = std::string(error.what()).find("duplicate region modification") != std::string::npos;
    }
    assert(threw);
}

void test_validate_rejects_remaining_mass_out_of_range() {
    const RegionCoordinate coordinate{13, 13, 13};
    const auto baseline = generate_region_baseline(1, coordinate, 1);
    std::size_t collectible_index = baseline.size();
    for (std::size_t i = 0; i < baseline.size(); ++i) {
        if (baseline[i].sample_mass_kg > 0.0) {
            collectible_index = i;
            break;
        }
    }
    // Every generated body with a non-empty material_id has positive
    // sample_mass_kg (see generate_region_baseline); the fixed test seed/
    // coordinate above was not hand-picked to guarantee one exists, so skip
    // gracefully in the (unreachable, but defensive) case none do.
    if (collectible_index == baseline.size()) {
        return;
    }
    const StaticSphereBody& collectible = baseline[collectible_index];

    {
        GeneratedRegionRecord region;
        region.coordinate = coordinate;
        region.region_id = derive_region_id(coordinate);
        region.modifications.push_back(
            RegionEntityModificationRecord{collectible.body_id, false, std::make_optional(-1.0)});
        bool threw = false;
        try {
            validate_generated_regions({region}, 1, 1);
        } catch (const std::runtime_error& error) {
            threw = std::string(error.what()).find("out of range") != std::string::npos;
        }
        assert(threw);
    }
    {
        GeneratedRegionRecord region;
        region.coordinate = coordinate;
        region.region_id = derive_region_id(coordinate);
        region.modifications.push_back(RegionEntityModificationRecord{
            collectible.body_id, false, std::make_optional(collectible.sample_mass_kg + 1.0)});
        bool threw = false;
        try {
            validate_generated_regions({region}, 1, 1);
        } catch (const std::runtime_error& error) {
            threw = std::string(error.what()).find("out of range") != std::string::npos;
        }
        assert(threw);
    }
}

} // namespace

int main() {
    test_derive_region_id_is_deterministic_and_distinct();
    test_generation_is_deterministic_for_identical_inputs();
    test_generation_entity_count_within_expected_bounds_and_unique_ids();
    test_generation_differs_across_coordinate();
    test_generation_differs_across_seed();
    test_generation_rejects_unsupported_algorithm_version();
    test_reconstruct_matches_baseline_with_no_modifications();
    test_reconstruct_excludes_removed_and_overrides_remaining_mass();
    test_validate_accepts_consistent_regions();
    test_validate_rejects_region_id_mismatched_with_coordinate();
    test_validate_rejects_empty_region_id();
    test_validate_rejects_duplicate_region_id();
    test_validate_rejects_observation_referencing_unknown_entity();
    test_validate_rejects_observation_target_id_mismatched_with_map_key();
    test_validate_rejects_modification_referencing_unknown_entity();
    test_validate_rejects_duplicate_modification();
    test_validate_rejects_remaining_mass_out_of_range();

    std::puts("generated_region_tests: all tests passed");
    return 0;
}
