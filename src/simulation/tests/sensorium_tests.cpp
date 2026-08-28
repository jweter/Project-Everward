#include "everward/simulation/sensorium.hpp"

#undef NDEBUG
#include <algorithm>
#include <cassert>
#include <iostream>

using everward::simulation::AdjacentEvolutionGenerator;
using everward::simulation::AudioPerceptionLayer;
using everward::simulation::EvolvingSensorium;
using everward::simulation::EvolutionContext;
using everward::simulation::EvolutionDomain;
using everward::simulation::EvolutionTrait;
using everward::simulation::ProbeStateSnapshot;

static bool has_layer(const std::vector<AudioPerceptionLayer>& layers, AudioPerceptionLayer layer) {
    return std::find(layers.begin(), layers.end(), layer) != layers.end();
}

static bool has_candidate(const EvolutionContext& context, const std::string& id) {
    const auto candidates = EvolvingSensorium::adjacent_candidates(context);
    return std::any_of(candidates.begin(), candidates.end(), [&](const auto& candidate) {
        return candidate.id == id;
    });
}

int main() {
    ProbeStateSnapshot snapshot;

    // Generation-1 starts intentionally sparse: essential accessibility cues
    // remain available, while rich in-world audio is not granted for free.
    {
        EvolutionContext context = AdjacentEvolutionGenerator::make_canonical_ev0001_context(snapshot);
        const auto state = EvolvingSensorium::evaluate(context);
        assert(state.essential_interface);
        assert(!state.internal_machine);
        assert(!state.contact_vibration);
        assert(!state.electromagnetic_sonification);
        assert(!state.adaptive_ambience);
        assert(!state.generative_music);
        assert(!state.songwriting);

        // The first richer physical-audio capability is already adjacent to
        // the canonical probe instead of requiring a scripted future class.
        context.fabrication_maturity = 1.10;
        assert(has_candidate(context, "sensorium.vibration.add"));
    }

    // Dedicated sensing hardware makes physical contact and EM phenomena
    // audible without requiring arbitrary soundtrack unlocks.
    {
        EvolutionContext context;
        context.generation = 2;
        context.installed_traits = {
            {"sensors.vibration", "Vibration sensing", EvolutionDomain::Sensors, 1.0, {"sensor", "vibration-sensing", "internal-vibration"}},
            {"sensors.radio", "Radio sensing", EvolutionDomain::Sensors, 1.0, {"sensor", "radio", "electromagnetic-sensing"}},
        };
        const auto state = EvolvingSensorium::evaluate(context);
        assert(state.internal_machine);
        assert(state.contact_vibration);
        assert(state.electromagnetic_sonification);
    }

    // Richer scientific perception plus computation can evolve from sonified
    // data into adaptive ambience and then genuinely generated music.
    {
        EvolutionContext context;
        context.generation = 8;
        context.science_maturity = 2.0;
        context.computation_maturity = 4.0;
        context.installed_traits = {
            {"sensors.science", "Scientific suite", EvolutionDomain::Sensors, 2.0, {"sensor", "scientific-sonification"}},
            {"audio.adaptive", "Adaptive audio synthesis", EvolutionDomain::Computation, 2.0, {"adaptive-audio"}},
        };
        const auto state = EvolvingSensorium::evaluate(context);
        assert(state.scientific_sonification);
        assert(state.adaptive_ambience);
        assert(state.generative_music);
        assert(!state.vocal_synthesis);
        assert(has_candidate(context, "sensorium.music-generation.add"));
    }

    // Very advanced computation may become artistic rather than merely
    // utilitarian. Songwriting is still prerequisite-gated.
    {
        EvolutionContext context;
        context.generation = 30;
        context.science_maturity = 4.0;
        context.computation_maturity = 16.0;
        context.installed_traits = {
            {"sensors.science", "Scientific suite", EvolutionDomain::Sensors, 4.0, {"sensor", "scientific-sonification"}},
            {"compute.design", "Design simulation", EvolutionDomain::Computation, 4.0, {"compute", "design-simulation", "adaptive-audio", "music-generation", "vocal-synthesis"}},
        };
        const auto state = EvolvingSensorium::evaluate(context);
        assert(state.generative_music);
        assert(state.vocal_synthesis);
        assert(state.songwriting);
        assert(has_candidate(context, "sensorium.songwriting.add"));

        const auto layers = EvolvingSensorium::active_layers(context);
        assert(has_layer(layers, AudioPerceptionLayer::GenerativeMusic));
        assert(has_layer(layers, AudioPerceptionLayer::VocalSynthesis));
        assert(has_layer(layers, AudioPerceptionLayer::Songwriting));
    }

    std::cout << "Evolving sensorium tests passed\n";
    return 0;
}
