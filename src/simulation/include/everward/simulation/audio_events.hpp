#pragma once

#include "everward/simulation/sensorium.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_set>

namespace everward::simulation {

// Issue #355 (Production slice A0): the immutable boundary between
// authoritative game truth and audio rendering. GENERATIVE_AUDIO_PRODUCTION_PLAN.md
// requires this envelope to carry a stable source event ID/tick/seed, optional
// read-only mechanical measurements, and a perception-gating flag, so the
// exact same authoritative event determines mechanics, HUD and audio -- the
// renderer/middleware can never invent a sound event on its own.
//
// This header is engine-independent, matching the convention established by
// mining.hpp/fix_it.hpp/material_processing.hpp: Unreal only presents sound
// triggers derived from envelopes this process already approved; it does not
// decide gating or dedup itself.
enum class AudioEventSchemaVersion : std::uint32_t {
    V1 = 1
};

// A small canonical action set covering A0 (essential/UI) now, with the
// mechanical categories A1 (tactile mining/Fix_It) will need next already
// named so #360 can extend this envelope rather than inventing a parallel
// one. Adding a new action is additive; removing or renumbering an existing
// one would break replay of already-recorded event fixtures.
enum class AudioEventAction {
    EssentialAlert,
    MenuConfirmation,
    Contact,
    MaterialExtracted,
    MaterialStored,
    RepairStageChanged
};

// Everything after event_id/simulation_tick/stable_audio_seed/source_entity_id
// is read-only descriptive context for synthesis parameters, per the plan's
// "Source events from game truth" table. None of it is ever written back to
// simulation state by audio code.
struct AudioEventEnvelope {
    std::uint32_t schema_version{static_cast<std::uint32_t>(AudioEventSchemaVersion::V1)};
    // Caller-assigned stable identity for one authoritative occurrence (for
    // example "contact:EV-0001:4821" or a UUID). Two envelopes sharing an
    // event_id are defined to be the same occurrence, however many times they
    // are submitted -- this is what makes dedup possible.
    std::string event_id;
    std::int64_t simulation_tick{0};
    // A reproducible variation seed, not a fresh random draw: the same event
    // replayed later (save/reload, deterministic test fixture) must derive
    // the same seed so synthesis variation stays reproducible rather than
    // re-rolled.
    std::uint64_t stable_audio_seed{0};
    std::string source_entity_id;
    AudioEventAction action{AudioEventAction::EssentialAlert};
    // Essential alerts and basic UI/menu feedback are never progression-gated
    // (GENERATIVE_AUDIO_PRODUCTION_PLAN.md's creative non-negotiables): they
    // bypass the sensorium perception check entirely. Everything else is
    // gated on whether the installed sensorium actually perceives it.
    bool bypass_perception_gate{false};
    AudioPerceptionLayer required_perception_layer{AudioPerceptionLayer::EssentialInterface};
    std::optional<std::string> material_id;
    std::optional<double> contact_force_n;
    std::optional<double> tool_load_fraction;
    std::optional<double> damage_delta;
    std::optional<double> integrity_delta;
    // Clamped to [0, 1]; see AudioEventGate::admit.
    double significance{0.0};
};

enum class AudioEventAdmission {
    // First time this event_id has been seen and its perception gate passed:
    // the caller should trigger exactly one audio cue.
    Admitted,
    // This event_id was already admitted before; the delivery is idempotent
    // and must not trigger a second (phantom/duplicate) cue.
    DuplicateSuppressed,
    // The installed sensorium does not yet perceive this event's required
    // layer (and it is not an essential/bypass cue): correctly silent, not
    // an error.
    GatedBySensorium
};

// Deterministic, read-only-with-respect-to-simulation-state source gating
// and dedup adapter. It never mutates SimulationCore/ProbeRuntime and never
// decides mechanics -- it only decides whether an already-true event may
// become one audio trigger.
class AudioEventGate {
public:
    [[nodiscard]] AudioEventAdmission admit(const AudioEventEnvelope& raw_event, const AudioPerceptionState& perception) {
        const AudioEventEnvelope event = validate(raw_event);
        if (delivered_event_ids_.contains(event.event_id)) {
            return AudioEventAdmission::DuplicateSuppressed;
        }
        if (!event.bypass_perception_gate && !perceives(perception, event.required_perception_layer)) {
            // Not yet delivered and not (yet) perceivable: correctly silent
            // for now, but not recorded as delivered, so the same real
            // occurrence can still admit once perception improves instead
            // of being permanently lost the first time it arrived too early.
            return AudioEventAdmission::GatedBySensorium;
        }
        delivered_event_ids_.insert(event.event_id);
        return AudioEventAdmission::Admitted;
    }

    [[nodiscard]] std::size_t delivered_event_count() const noexcept {
        return delivered_event_ids_.size();
    }

    // Completion criteria (#355): "invalid values clamped". event_id and
    // source_entity_id identify *which* occurrence this is, so an empty one
    // cannot be silently coerced into a usable identity -- it is rejected
    // closed, consistent with every other authoritative-boundary validator
    // in this module (material_processing.hpp, fix_it.hpp). Numeric
    // measurement fields instead degrade to a safe, finite value rather than
    // aborting an otherwise-valid real event. Exposed publicly (and used
    // internally by admit()) so a caller/renderer can read back the exact
    // sanitized values that will actually drive synthesis parameters.
    [[nodiscard]] static AudioEventEnvelope validate(const AudioEventEnvelope& raw_event) {
        if (raw_event.event_id.empty()) {
            throw std::invalid_argument("audio event_id must not be empty");
        }
        if (raw_event.source_entity_id.empty()) {
            throw std::invalid_argument("audio event source_entity_id must not be empty");
        }
        if (raw_event.simulation_tick < 0) {
            throw std::invalid_argument("audio event simulation_tick must be non-negative");
        }

        AudioEventEnvelope event = raw_event;
        event.significance = clamp01(event.significance);
        event.contact_force_n = clamp_non_negative_finite(event.contact_force_n);
        event.tool_load_fraction = clamp01_optional(event.tool_load_fraction);
        event.damage_delta = clamp_finite(event.damage_delta);
        event.integrity_delta = clamp_finite(event.integrity_delta);
        return event;
    }

private:
    [[nodiscard]] static bool perceives(const AudioPerceptionState& perception, AudioPerceptionLayer layer) noexcept {
        switch (layer) {
            case AudioPerceptionLayer::EssentialInterface:
                return perception.essential_interface;
            case AudioPerceptionLayer::InternalMachine:
                return perception.internal_machine;
            case AudioPerceptionLayer::ContactVibration:
                return perception.contact_vibration;
            case AudioPerceptionLayer::ElectromagneticSonification:
                return perception.electromagnetic_sonification;
            case AudioPerceptionLayer::ScientificSonification:
                return perception.scientific_sonification;
            case AudioPerceptionLayer::AdaptiveAmbience:
                return perception.adaptive_ambience;
            case AudioPerceptionLayer::GenerativeMusic:
                return perception.generative_music;
            case AudioPerceptionLayer::VocalSynthesis:
                return perception.vocal_synthesis;
            case AudioPerceptionLayer::Songwriting:
                return perception.songwriting;
        }
        return false;
    }

    [[nodiscard]] static double clamp01(double value) noexcept {
        if (!std::isfinite(value)) return 0.0;
        return std::min(1.0, std::max(0.0, value));
    }

    [[nodiscard]] static std::optional<double> clamp01_optional(std::optional<double> value) noexcept {
        if (!value.has_value()) return std::nullopt;
        return clamp01(*value);
    }

    [[nodiscard]] static std::optional<double> clamp_non_negative_finite(std::optional<double> value) noexcept {
        if (!value.has_value()) return std::nullopt;
        if (!std::isfinite(*value)) return 0.0;
        return std::max(0.0, *value);
    }

    [[nodiscard]] static std::optional<double> clamp_finite(std::optional<double> value) noexcept {
        if (!value.has_value()) return std::nullopt;
        if (!std::isfinite(*value)) return 0.0;
        return value;
    }

    std::unordered_set<std::string> delivered_event_ids_{};
};

} // namespace everward::simulation
