#include "everward/simulation/probe_lineage.hpp"

#undef NDEBUG
#include <cassert>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

namespace {

using everward::simulation::ProbeLineageRecord;
using everward::simulation::validate_probe_lineages;

bool rejects(const std::vector<ProbeLineageRecord>& records,
             const std::unordered_set<std::string>& probes) {
    try {
        validate_probe_lineages(records, probes);
    } catch (const std::runtime_error&) {
        return true;
    }
    return false;
}

void test_root_and_successor_lineage_are_valid() {
    const std::unordered_set<std::string> probes{"EV-0001", "EV-0002"};
    validate_probe_lineages(
        {
            {"EV-0001", "prime", "", 1},
            {"EV-0002", "prime", "EV-0001", 2},
        },
        probes);
}

void test_unknown_probe_or_parent_fails_closed() {
    const std::unordered_set<std::string> probes{"EV-0001"};
    assert(rejects({{"EV-0002", "prime", "EV-0001", 2}}, probes));
    assert(rejects({{"EV-0001", "prime", "EV-9999", 2}}, probes));
}

void test_invalid_identity_and_duplicate_records_fail_closed() {
    const std::unordered_set<std::string> probes{"EV-0001"};
    assert(rejects({{"EV-0001", "", "", 1}}, probes));
    assert(rejects({{"EV-0001", "prime", "EV-0001", 1}}, probes));
    assert(rejects({{"EV-0001", "prime", "", 0}}, probes));
    assert(rejects(
        {
            {"EV-0001", "prime", "", 1},
            {"EV-0001", "prime", "", 1},
        },
        probes));
}

void test_invalid_generation_relationship_fails_closed() {
    const std::unordered_set<std::string> probes{"EV-0001", "EV-0002"};
    // A successor at the same generation as its own parent is invalid.
    assert(rejects(
        {
            {"EV-0001", "prime", "", 2},
            {"EV-0002", "prime", "EV-0001", 2},
        },
        probes));
    // A successor at an earlier generation than its own parent is invalid.
    assert(rejects(
        {
            {"EV-0001", "prime", "", 3},
            {"EV-0002", "prime", "EV-0001", 1},
        },
        probes));
    // A parent with no lineage record of its own cannot be cross-checked and
    // is accepted here (the unknown-parent-reference check above still
    // rejects a parent_probe_id absent from persisted_probe_ids entirely).
    validate_probe_lineages({{"EV-0002", "prime", "EV-0001", 1}}, probes);
}

} // namespace

int main() {
    test_root_and_successor_lineage_are_valid();
    test_unknown_probe_or_parent_fails_closed();
    test_invalid_identity_and_duplicate_records_fail_closed();
    test_invalid_generation_relationship_fails_closed();
    return 0;
}
