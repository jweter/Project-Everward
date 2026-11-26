#include "everward/simulation/audio_silence_policy.hpp"

#undef NDEBUG
#include <cassert>
#include <iostream>

using namespace everward::simulation;

namespace {

AudioEventEnvelope event_with(AudioEventAction action) {
    AudioEventEnvelope event;
    event.event_id = "evt:1";
    event.simulation_tick = 1;
    event.source_entity_id = "EV-0001";
    event.action = action;
    event.bypass_perception_gate = action == AudioEventAction::EssentialAlert;
    return event;
}

bool only_essential_and_interface_allowed(const AudioPerceptionState& perception) {
    for (std::size_t i = 0; i < kAudioBusCount; ++i) {
        const AudioBus bus = static_cast<AudioBus>(i);
        const bool expected = bus == AudioBus::EssentialAlerts || bus == AudioBus::Interface;
        if (bus_allowed(perception, bus) != expected) return false;
    }
    return true;
}

} // namespace

int main() {
    // Gen-1 default: only essential alerts and interface feedback can sound.
    {
        const AudioPerceptionState gen1{};
        assert(only_essential_and_interface_allowed(gen1));
    }

    // Essential alerts and the interface stay allowed even if the essential flag is cleared.
    {
        AudioPerceptionState stripped{};
        stripped.essential_interface = false;
        assert(bus_allowed(stripped, AudioBus::EssentialAlerts));
        assert(bus_allowed(stripped, AudioBus::Interface));
    }

    // Each perception flag opens exactly its own bus and no other.
    {
        AudioPerceptionState state{};
        state.internal_machine = true;
        assert(bus_allowed(state, AudioBus::Mechanical));
        assert(!bus_allowed(state, AudioBus::Sonification));
        assert(!bus_allowed(state, AudioBus::Ambience));
        assert(!bus_allowed(state, AudioBus::Music));
    }
    {
        AudioPerceptionState state{};
        state.contact_vibration = true;
        assert(bus_allowed(state, AudioBus::Mechanical));
        assert(!bus_allowed(state, AudioBus::Music));
    }
    {
        AudioPerceptionState state{};
        state.electromagnetic_sonification = true;
        assert(bus_allowed(state, AudioBus::Sonification));
        assert(!bus_allowed(state, AudioBus::Mechanical));
    }
    {
        AudioPerceptionState state{};
        state.scientific_sonification = true;
        assert(bus_allowed(state, AudioBus::Sonification));
        assert(!bus_allowed(state, AudioBus::Ambience));
    }
    {
        AudioPerceptionState state{};
        state.adaptive_ambience = true;
        assert(bus_allowed(state, AudioBus::Ambience));
        assert(!bus_allowed(state, AudioBus::Music));
    }
    {
        AudioPerceptionState state{};
        state.generative_music = true;
        assert(bus_allowed(state, AudioBus::Music));
        assert(!bus_allowed(state, AudioBus::Ambience));
    }
    {
        AudioPerceptionState state{};
        state.vocal_synthesis = true;
        assert(bus_allowed(state, AudioBus::Music));
    }
    {
        AudioPerceptionState state{};
        state.songwriting = true;
        assert(bus_allowed(state, AudioBus::Music));
    }

    // An invalid bus value is never allowed.
    assert(!bus_allowed(AudioPerceptionState{}, AudioBus::Count));

    // Policy silences a disallowed bus but never an essential alert, and keeps its text.
    {
        const AudioMixSettings mix;
        const AudioPerceptionState gen1{};
        const AudioDelivery contact = apply_silence_policy(mix.deliver(event_with(AudioEventAction::Contact), ""), gen1);
        assert(contact.bus == AudioBus::Mechanical);
        assert(contact.gain == 0.0 && !contact.audible);

        const AudioDelivery alert =
            apply_silence_policy(mix.deliver(event_with(AudioEventAction::EssentialAlert), "Thermal limit exceeded"), gen1);
        assert(alert.audible && alert.gain == 1.0);
        assert(alert.text_equivalent_required);
        assert(alert.text_equivalent == "Thermal limit exceeded");

        const AudioDelivery menu = apply_silence_policy(mix.deliver(event_with(AudioEventAction::MenuConfirmation), ""), gen1);
        assert(menu.audible && menu.gain == 1.0);
    }

    // Once perceived, the mechanical bus is audible again; mix gain is preserved.
    {
        AudioMixSettings mix;
        mix.bus(AudioBus::Mechanical).volume = 0.5;
        AudioPerceptionState perceiving{};
        perceiving.contact_vibration = true;
        const AudioDelivery contact = apply_silence_policy(mix.deliver(event_with(AudioEventAction::Contact), ""), perceiving);
        assert(contact.audible && contact.gain == 0.5);
    }

    // A silenced essential alert (user muted the master) still carries its text equivalent.
    {
        AudioMixSettings mix;
        mix.master.muted = true;
        const AudioDelivery alert =
            apply_silence_policy(mix.deliver(event_with(AudioEventAction::EssentialAlert), "Hull breach"), AudioPerceptionState{});
        assert(!alert.audible);
        assert(alert.text_equivalent_required && alert.text_equivalent == "Hull breach");
    }

    std::cout << "audio_silence_policy_tests passed\n";
    return 0;
}
