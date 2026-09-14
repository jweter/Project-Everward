#pragma once

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace everward::simulation {

// Data-oriented campaign identity for successor probes. This deliberately
// contains no manufacturing/evolution rules: persistence records lineage
// facts only after authoritative gameplay creates them.
struct ProbeLineageRecord {
    std::string probe_id{};
    std::string lineage_id{};
    std::string parent_probe_id{};
    int generation{1};
};

inline void validate_probe_lineages(
        const std::vector<ProbeLineageRecord>& lineages,
        const std::unordered_set<std::string>& persisted_probe_ids) {
    std::unordered_set<std::string> lineage_probe_ids;
    for (const auto& record : lineages) {
        if (record.probe_id.empty()) {
            throw std::runtime_error("lineage probe_id must not be empty");
        }
        if (record.lineage_id.empty()) {
            throw std::runtime_error("lineage_id must not be empty");
        }
        if (record.generation < 1) {
            throw std::runtime_error("lineage generation must be positive");
        }
        if (!persisted_probe_ids.contains(record.probe_id)) {
            throw std::runtime_error(
                "lineage references unknown persisted probe_id: " + record.probe_id);
        }
        if (!lineage_probe_ids.insert(record.probe_id).second) {
            throw std::runtime_error(
                "duplicate lineage record for probe_id: " + record.probe_id);
        }
        if (record.parent_probe_id == record.probe_id) {
            throw std::runtime_error("lineage parent_probe_id must not reference itself");
        }
        if (!record.parent_probe_id.empty() &&
            !persisted_probe_ids.contains(record.parent_probe_id)) {
            throw std::runtime_error(
                "lineage parent_probe_id references unknown persisted probe: " +
                record.parent_probe_id);
        }
    }

    // A successor's generation must be strictly greater than its own parent's
    // generation whenever the parent also has a lineage record in this same
    // save -- a parent record with no lineage entry of its own (e.g. a
    // pre-lineage save's implicit root) cannot be cross-checked and is left
    // to the unknown-parent-reference check above.
    std::unordered_map<std::string, int> generation_by_probe_id;
    generation_by_probe_id.reserve(lineages.size());
    for (const auto& record : lineages) {
        generation_by_probe_id.emplace(record.probe_id, record.generation);
    }
    for (const auto& record : lineages) {
        if (record.parent_probe_id.empty()) {
            continue;
        }
        const auto parent_it = generation_by_probe_id.find(record.parent_probe_id);
        if (parent_it != generation_by_probe_id.end() &&
            record.generation <= parent_it->second) {
            throw std::runtime_error(
                "lineage generation must exceed parent generation for probe_id: " +
                record.probe_id);
        }
    }
}

} // namespace everward::simulation
