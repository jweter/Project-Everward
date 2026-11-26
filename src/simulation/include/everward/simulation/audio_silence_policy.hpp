#pragma once

#include "everward/simulation/audio_mix.hpp"

namespace everward::simulation {

// Issue #355 (Production slice A0): the Gen-1 "near-silent opening" policy. The probe's
// fictional sensorium starts nearly deaf: only essential alerts and interface feedback are
// ever audible by default, and every other bus opens only when the installed sensorium
// actually perceives that kind of sound (AudioPerceptionState). Engine-independent and
// read-only: it never writes simulation state and never touches the text equivalent that
// an essential alert carries, so silencing a bus can never hide an alert from the player.

// Whether a mix bus may sound at all for the given perception state.
[[nodiscard]] inline bool bus_allowed(const AudioPerceptionState& perception, AudioBus bus) noexcept {
    switch (bus) {
        case AudioBus::EssentialAlerts:
        case AudioBus::Interface:
            // Never progression-gated (GENERATIVE_AUDIO_PRODUCTION_PLAN.md).
            return true;
        case AudioBus::Mechanical:
            return perception.internal_machine || perception.contact_vibration;
        case AudioBus::Sonification:
            return perception.electromagnetic_sonification || perception.scientific_sonification;
        case AudioBus::Ambience:
            return perception.adaptive_ambience;
        case AudioBus::Music:
            return perception.generative_music || perception.vocal_synthesis || perception.songwriting;
        case AudioBus::Count:
            break;
    }
    return false;
}

// Applies the policy to one delivery: a delivery on a disallowed bus is silenced
// (gain 0, not audible). Essential-alert text equivalents are left exactly as they were.
[[nodiscard]] inline AudioDelivery apply_silence_policy(AudioDelivery delivery, const AudioPerceptionState& perception) {
    if (!bus_allowed(perception, delivery.bus)) {
        delivery.gain = 0.0;
        delivery.audible = false;
    }
    return delivery;
}

} // namespace everward::simulation
