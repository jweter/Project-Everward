#pragma once

#include <algorithm>
#include <map>
#include <string>
#include <string_view>
#include <utility>

namespace everward::simulation {

// A0 source records are declarations, not an automated legal determination.
// Only an explicitly reviewed, complete record can authorize production use.
enum class AudioAssetRights { Pending, Approved, Rejected };

struct AudioAssetProvenance {
    std::string asset_id;
    std::string source;
    std::string creator;
    std::string license;
    std::string rights_evidence;
    std::string content_sha256;
    AudioAssetRights rights = AudioAssetRights::Pending;
};

[[nodiscard]] inline bool valid_audio_asset_provenance(const AudioAssetProvenance& record) {
    const auto nonblank = [](const std::string& value) {
        return value.find_first_not_of(" \t\r\n") != std::string::npos;
    };
    const auto hex = [](char ch) {
        return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f');
    };
    return nonblank(record.asset_id) && nonblank(record.source) &&
           nonblank(record.creator) && nonblank(record.license) &&
           nonblank(record.rights_evidence) && record.content_sha256.size() == 64 &&
           std::all_of(record.content_sha256.begin(), record.content_sha256.end(), hex) &&
           (record.rights == AudioAssetRights::Pending ||
            record.rights == AudioAssetRights::Approved ||
            record.rights == AudioAssetRights::Rejected);
}

class AudioAssetProvenanceRegistry {
public:
    // Existing declarations cannot silently be replaced or elevated.
    [[nodiscard]] bool insert(AudioAssetProvenance record) {
        if (!valid_audio_asset_provenance(record)) return false;
        const auto id = record.asset_id;
        return records_.emplace(id, std::move(record)).second;
    }

    [[nodiscard]] const AudioAssetProvenance* find(std::string_view id) const {
        const auto it = records_.find(id);
        return it == records_.end() ? nullptr : &it->second;
    }

    // The caller supplies the independently calculated digest of the actual asset.
    [[nodiscard]] bool permits_use(std::string_view id, std::string_view sha256) const {
        const auto* record = find(id);
        return record != nullptr && record->rights == AudioAssetRights::Approved &&
               record->content_sha256 == sha256;
    }

private:
    std::map<std::string, AudioAssetProvenance, std::less<>> records_;
};

} // namespace everward::simulation
