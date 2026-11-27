#include "everward/simulation/audio_asset_provenance.hpp"

#undef NDEBUG
#include <cassert>
#include <string>

using namespace everward::simulation;

int main() {
    AudioAssetProvenanceRegistry registry;
    const std::string digest(64, 'a');
    const std::string changed(64, 'b');
    AudioAssetProvenance record{"alert", "synthetic test source", "test creator",
                                "test license", "test review receipt", digest,
                                AudioAssetRights::Approved};
    assert(!registry.permits_use("alert", digest));
    assert(registry.insert(record));
    assert(registry.find("alert") != nullptr);
    assert(registry.permits_use("alert", digest));
    assert(!registry.permits_use("alert", changed));
    assert(!registry.permits_use("missing", digest));

    auto duplicate = record;
    duplicate.content_sha256 = changed;
    assert(!registry.insert(duplicate));
    assert(registry.permits_use("alert", digest));

    auto pending = record;
    pending.asset_id = "pending";
    pending.rights = AudioAssetRights::Pending;
    assert(registry.insert(pending));
    assert(!registry.permits_use("pending", digest));
    pending.rights = AudioAssetRights::Approved;
    assert(!registry.insert(pending));
    assert(!registry.permits_use("pending", digest));

    auto rejected = record;
    rejected.asset_id = "rejected";
    rejected.rights = AudioAssetRights::Rejected;
    assert(registry.insert(rejected));
    assert(!registry.permits_use("rejected", digest));

    auto invalid = record;
    invalid.asset_id = "invalid";
    invalid.rights_evidence = "  ";
    assert(!registry.insert(invalid));
    invalid = record;
    invalid.asset_id = "invalid";
    invalid.content_sha256 = std::string(64, 'z');
    assert(!registry.insert(invalid));
    invalid.content_sha256 = "abc";
    assert(!registry.insert(invalid));
    invalid = record;
    invalid.asset_id = "invalid";
    invalid.rights = static_cast<AudioAssetRights>(99);
    assert(!registry.insert(invalid));
    assert(registry.find("invalid") == nullptr);
    return 0;
}
