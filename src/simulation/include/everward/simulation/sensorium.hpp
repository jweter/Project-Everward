#pragma once

#include "everward/simulation/evolution.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace everward::simulation {

enum class AudioPerceptionLayer {
    EssentialInterface,
    InternalMachine,
    ContactVibration,
    ElectromagneticSonification,
    ScientificSonification,
    AdaptiveAmbience,
    GenerativeMusic,
    VocalSynthesis,
    Songwriting
};

struct AudioPerceptionState {
    bool essential_interface{true};
    bool internal_machine{false};
    bool contact_vibration{false};
    bool electromagnetic_sonification{false};
    bool scientific_sonification{false};
    bool adaptive_ambience{false};
    bool generative_music{false};
    bool vocal_synthesis{false};
    bool songwriting{false};
};

class EvolvingSensorium {
public:
    [[nodiscard]] static AudioPerceptionState evaluate(const EvolutionContext& context) {
        AudioPerceptionState state;

        // Accessibility/safety cues are never progression-gated. Everything
        // below this line describes the probe's in-world sensory richness.
        state.internal_machine = has_tag(context, "internal-vibration") || context.generation > 1;
        state.contact_vibration = has_tag(context, "vibration-sensing");
        state.electromagnetic_sonification = has_tag(context, "electromagnetic-sensing") || has_tag(context, "radio");
        state.scientific_sonification = has_tag(context, "scientific-sonification") ||
            (has_tag(context, "sensor") && context.science_maturity >= 1.5);
        state.adaptive_ambience = has_tag(context, "adaptive-audio") ||
            (state.scientific_sonification && context.computation_maturity >= 2.0);
        state.generative_music = has_tag(context, "music-generation") ||
            (state.adaptive_ambience && context.computation_maturity >= 4.0);
        state.vocal_synthesis = has_tag(context, "vocal-synthesis") ||
            (state.generative_music && context.computation_maturity >= 8.0);
        state.songwriting = has_tag(context, "songwriting") ||
            (state.vocal_synthesis && has_tag(context, "design-simulation") && context.computation_maturity >= 12.0);
        return state;
    }

    [[nodiscard]] static std::vector<AudioPerceptionLayer> active_layers(const EvolutionContext& context) {
        const auto state = evaluate(context);
        std::vector<AudioPerceptionLayer> layers{AudioPerceptionLayer::EssentialInterface};
        if (state.internal_machine) layers.push_back(AudioPerceptionLayer::InternalMachine);
        if (state.contact_vibration) layers.push_back(AudioPerceptionLayer::ContactVibration);
        if (state.electromagnetic_sonification) layers.push_back(AudioPerceptionLayer::ElectromagneticSonification);
        if (state.scientific_sonification) layers.push_back(AudioPerceptionLayer::ScientificSonification);
        if (state.adaptive_ambience) layers.push_back(AudioPerceptionLayer::AdaptiveAmbience);
        if (state.generative_music) layers.push_back(AudioPerceptionLayer::GenerativeMusic);
        if (state.vocal_synthesis) layers.push_back(AudioPerceptionLayer::VocalSynthesis);
        if (state.songwriting) layers.push_back(AudioPerceptionLayer::Songwriting);
        return layers;
    }

    // Sensorium upgrades participate in the same "deepen or expand" philosophy
    // as the rest of Everward. These are spoiler-safe adjacent possibilities,
    // not a complete published audio technology tree.
    [[nodiscard]] static std::vector<EvolutionCandidate> adjacent_candidates(const EvolutionContext& context) {
        std::vector<EvolutionCandidate> out;

        if (has_tag(context, "sensor") && !has_tag(context, "vibration-sensing") &&
            context.fabrication_maturity >= 1.05) {
            out.push_back({
                "sensorium.vibration.add",
                "Add vibration sensing",
                "Instrument the probe structure so internal machinery and physical contact become perceivable as sound.",
                EvolutionDomain::Sensors,
                "sensors.optical",
                1.0, 1.0, 0.08,
                1.02, 1.03, 1.01,
                true,
                {"sensor", "vibration-sensing", "internal-vibration"}
            });
        }

        if (has_tag(context, "radio") && !has_tag(context, "scientific-sonification") &&
            context.science_maturity >= 1.5 && context.computation_maturity >= 1.5) {
            out.push_back({
                "sensorium.scientific-sonification.add",
                "Add scientific sonification",
                "Translate selected scientific and electromagnetic measurements into an audible machine sensorium.",
                EvolutionDomain::Computation,
                "computation.core",
                context.computation_maturity, context.computation_maturity, 0.10,
                1.01, 1.05, 1.04,
                true,
                {"scientific-sonification"}
            });
        }

        if (has_tag(context, "scientific-sonification") && !has_tag(context, "adaptive-audio") &&
            context.computation_maturity >= 2.0) {
            out.push_back({
                "sensorium.adaptive-audio.add",
                "Add adaptive audio interpretation",
                "Synthesize contextual ambience from sensed machine and environmental state.",
                EvolutionDomain::Computation,
                "computation.core",
                context.computation_maturity, context.computation_maturity, 0.10,
                1.01, 1.07, 1.06,
                true,
                {"adaptive-audio"}
            });
        }

        if (has_tag(context, "adaptive-audio") && !has_tag(context, "music-generation") &&
            context.computation_maturity >= 4.0) {
            out.push_back({
                "sensorium.music-generation.add",
                "Develop generative music",
                "Move beyond utilitarian sonification and compose adaptive music from experience, memory and environment.",
                EvolutionDomain::Computation,
                "computation.core",
                context.computation_maturity, context.computation_maturity, 0.12,
                1.00, 1.10, 1.08,
                true,
                {"music-generation"}
            });
        }

        if (has_tag(context, "music-generation") && !has_tag(context, "vocal-synthesis") &&
            context.computation_maturity >= 8.0) {
            out.push_back({
                "sensorium.vocal-synthesis.add",
                "Develop vocal synthesis",
                "Create controllable vocal timbre for humming, voiced expression and eventually song.",
                EvolutionDomain::Computation,
                "computation.core",
                context.computation_maturity, context.computation_maturity, 0.12,
                1.00, 1.08, 1.07,
                true,
                {"vocal-synthesis"}
            });
        }

        if (has_tag(context, "vocal-synthesis") && has_tag(context, "design-simulation") &&
            !has_tag(context, "songwriting") && context.computation_maturity >= 12.0) {
            out.push_back({
                "sensorium.songwriting.add",
                "Develop songwriting",
                "Compose persistent songs, motifs and lineage music rather than only reactive ambience.",
                EvolutionDomain::Computation,
                "computation.design-simulation",
                context.computation_maturity, context.computation_maturity, 0.13,
                1.00, 1.10, 1.08,
                true,
                {"songwriting"}
            });
        }

        return out;
    }

private:
    [[nodiscard]] static bool has_tag(const EvolutionContext& context, std::string_view tag) {
        for (const auto& trait : context.installed_traits) {
            if (std::find(trait.tags.begin(), trait.tags.end(), tag) != trait.tags.end()) {
                return true;
            }
        }
        return false;
    }
};

} // namespace everward::simulation
