#pragma once

#include "everward/simulation/audio_events.hpp"
#include "everward/simulation/json_value.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace everward::simulation {

// Issue #355 (Production slice A0), step 3: mix buses, independent volume and
// mute, and a text equivalent for essential alerts. Engine-independent like
// audio_events.hpp: this header decides *how loud and with what visible
// equivalent* an already-admitted AudioEventEnvelope should be presented. It
// never reads or writes simulation state, never admits or suppresses events
// (AudioEventGate owns that), and music/sound settings stay fully under
// player control (GENERATIVE_AUDIO_PRODUCTION_PLAN.md).
enum class AudioBus : std::size_t {
    EssentialAlerts = 0,
    Interface,
    Mechanical,
    Sonification,
    Ambience,
    Music,
    Count
};

inline constexpr std::size_t kAudioBusCount = static_cast<std::size_t>(AudioBus::Count);
inline constexpr std::uint32_t kAudioMixSchemaVersion = 1;

struct AudioBusSetting {
    double volume{1.0};
    bool muted{false};
};

// Maps an authoritative event action to its mix bus. Adding an action to
// AudioEventAction requires deciding its bus here (the switch has no default
// so the compiler flags a missing case).
[[nodiscard]] inline AudioBus bus_for_action(AudioEventAction action) noexcept {
    switch (action) {
        case AudioEventAction::EssentialAlert:
            return AudioBus::EssentialAlerts;
        case AudioEventAction::MenuConfirmation:
            return AudioBus::Interface;
        case AudioEventAction::Contact:
        case AudioEventAction::MaterialExtracted:
        case AudioEventAction::MaterialStored:
        case AudioEventAction::RepairStageChanged:
            return AudioBus::Mechanical;
    }
    return AudioBus::Mechanical;
}

// Maps an unlockable perception layer to the bus that carries it, so later
// slices (A2/A3) share the same mixer.
[[nodiscard]] inline AudioBus bus_for_layer(AudioPerceptionLayer layer) noexcept {
    switch (layer) {
        case AudioPerceptionLayer::EssentialInterface:
            return AudioBus::EssentialAlerts;
        case AudioPerceptionLayer::InternalMachine:
        case AudioPerceptionLayer::ContactVibration:
            return AudioBus::Mechanical;
        case AudioPerceptionLayer::ElectromagneticSonification:
        case AudioPerceptionLayer::ScientificSonification:
            return AudioBus::Sonification;
        case AudioPerceptionLayer::AdaptiveAmbience:
            return AudioBus::Ambience;
        case AudioPerceptionLayer::GenerativeMusic:
        case AudioPerceptionLayer::VocalSynthesis:
        case AudioPerceptionLayer::Songwriting:
            return AudioBus::Music;
    }
    return AudioBus::Music;
}

[[nodiscard]] inline const char* bus_name(AudioBus bus) noexcept {
    switch (bus) {
        case AudioBus::EssentialAlerts: return "essential_alerts";
        case AudioBus::Interface: return "interface";
        case AudioBus::Mechanical: return "mechanical";
        case AudioBus::Sonification: return "sonification";
        case AudioBus::Ambience: return "ambience";
        case AudioBus::Music: return "music";
        case AudioBus::Count: break;
    }
    return "invalid";
}

// What the presentation layer should do for one admitted event.
struct AudioDelivery {
    AudioBus bus{AudioBus::Mechanical};
    // Final linear gain in [0, 1] after master and bus volume/mute. Zero means
    // the renderer must not start (and must stop) the cue.
    double gain{0.0};
    bool audible{false};
    // True for essential alerts regardless of volume or mute: the same
    // authoritative event must also reach the player through a visual/text
    // channel, so an alert is never audio-only.
    bool text_equivalent_required{false};
    std::string text_equivalent;
};

class AudioMixSettings {
public:
    AudioBusSetting master{};

    [[nodiscard]] AudioBusSetting& bus(AudioBus which) { return buses_[index(which)]; }
    [[nodiscard]] const AudioBusSetting& bus(AudioBus which) const { return buses_[index(which)]; }

    // Invalid player-supplied values degrade to a safe value instead of
    // aborting: non-finite volume resets to the default (1.0), out-of-range
    // volume clamps to [0, 1].
    [[nodiscard]] static double sanitize_volume(double volume) noexcept {
        if (!std::isfinite(volume)) return 1.0;
        return std::min(1.0, std::max(0.0, volume));
    }

    [[nodiscard]] AudioMixSettings sanitized() const {
        AudioMixSettings result = *this;
        result.master.volume = sanitize_volume(master.volume);
        for (auto& setting : result.buses_) {
            setting.volume = sanitize_volume(setting.volume);
        }
        return result;
    }

    [[nodiscard]] double effective_gain(AudioBus which) const noexcept {
        const AudioBusSetting& bus_setting = buses_[index(which)];
        if (master.muted || bus_setting.muted) return 0.0;
        return sanitize_volume(master.volume) * sanitize_volume(bus_setting.volume);
    }

    // Presents one *already admitted* event. An essential alert without a
    // non-empty text equivalent is rejected closed (std::invalid_argument),
    // because the plan requires every essential message to have an
    // equivalent visual/text channel. Other events carry no required text.
    [[nodiscard]] AudioDelivery deliver(const AudioEventEnvelope& event, const std::string& text_equivalent) const {
        const AudioEventEnvelope valid = AudioEventGate::validate(event);
        AudioDelivery delivery;
        delivery.bus = bus_for_action(valid.action);
        delivery.gain = effective_gain(delivery.bus);
        delivery.audible = delivery.gain > 0.0;
        delivery.text_equivalent_required = valid.action == AudioEventAction::EssentialAlert;
        if (delivery.text_equivalent_required) {
            if (text_equivalent.empty()) {
                throw std::invalid_argument("essential alert requires a text equivalent");
            }
            delivery.text_equivalent = text_equivalent;
        }
        return delivery;
    }

    [[nodiscard]] JsonValue to_json() const {
        JsonValue root = JsonValue::make_object();
        root.set("schema_version", static_cast<std::int64_t>(kAudioMixSchemaVersion));
        const AudioMixSettings clean = sanitized();
        root.set("master", setting_to_json(clean.master));
        JsonValue buses = JsonValue::make_object();
        for (std::size_t i = 0; i < kAudioBusCount; ++i) {
            buses.set(bus_name(static_cast<AudioBus>(i)), setting_to_json(clean.buses_[i]));
        }
        root.set("buses", std::move(buses));
        return root;
    }

    // Strict reload: unknown schema version or wrong field types throw
    // (matching save_data.hpp's "fail clearly" rule). A missing bus keeps its
    // default. Out-of-range or non-finite volumes are sanitized.
    [[nodiscard]] static AudioMixSettings from_json(const JsonValue& root) {
        if (root.require("schema_version").as_int64() != static_cast<std::int64_t>(kAudioMixSchemaVersion)) {
            throw std::runtime_error("unsupported audio mix schema_version");
        }
        AudioMixSettings settings;
        settings.master = setting_from_json(root.require("master"));
        const JsonValue& buses = root.require("buses");
        for (std::size_t i = 0; i < kAudioBusCount; ++i) {
            if (const JsonValue* found = buses.find(bus_name(static_cast<AudioBus>(i)))) {
                settings.buses_[i] = setting_from_json(*found);
            }
        }
        return settings.sanitized();
    }

private:
    [[nodiscard]] static std::size_t index(AudioBus which) {
        const std::size_t i = static_cast<std::size_t>(which);
        if (i >= kAudioBusCount) {
            throw std::out_of_range("invalid AudioBus");
        }
        return i;
    }

    [[nodiscard]] static JsonValue setting_to_json(const AudioBusSetting& setting) {
        JsonValue object = JsonValue::make_object();
        object.set("volume", setting.volume);
        object.set("muted", setting.muted);
        return object;
    }

    [[nodiscard]] static AudioBusSetting setting_from_json(const JsonValue& value) {
        AudioBusSetting setting;
        setting.volume = value.require("volume").as_double();
        setting.muted = value.require("muted").as_bool();
        return setting;
    }

    std::array<AudioBusSetting, kAudioBusCount> buses_{};
};

// Forgiving reload for startup: any corrupt or unsupported settings text
// yields defaults (everything audible at full volume, nothing muted) and
// reports it, rather than blocking the game from starting.
struct AudioMixLoadResult {
    AudioMixSettings settings;
    bool used_defaults{false};
};

[[nodiscard]] inline AudioMixLoadResult load_audio_mix_or_default(const std::string& text) {
    try {
        return {AudioMixSettings::from_json(JsonValue::parse(text)), false};
    } catch (const std::exception&) {
        return {AudioMixSettings{}, true};
    }
}

} // namespace everward::simulation
