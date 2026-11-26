#include "everward/simulation/audio_mix.hpp"

#undef NDEBUG
#include <cassert>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace everward::simulation;

namespace {

AudioEventEnvelope event_with(AudioEventAction action) {
    AudioEventEnvelope event;
    event.event_id = "evt:1";
    event.simulation_tick = 5;
    event.source_entity_id = "EV-0001";
    event.action = action;
    event.bypass_perception_gate = action == AudioEventAction::EssentialAlert;
    return event;
}

template <typename Fn>
bool throws(Fn fn) {
    try {
        fn();
    } catch (const std::exception&) {
        return true;
    }
    return false;
}

} // namespace

int main() {
    // Every action maps to a bus; essential alerts and UI never share the
    // mechanical bus, so muting machine sounds cannot silence an alert.
    assert(bus_for_action(AudioEventAction::EssentialAlert) == AudioBus::EssentialAlerts);
    assert(bus_for_action(AudioEventAction::MenuConfirmation) == AudioBus::Interface);
    assert(bus_for_action(AudioEventAction::Contact) == AudioBus::Mechanical);
    assert(bus_for_action(AudioEventAction::MaterialExtracted) == AudioBus::Mechanical);
    assert(bus_for_action(AudioEventAction::MaterialStored) == AudioBus::Mechanical);
    assert(bus_for_action(AudioEventAction::RepairStageChanged) == AudioBus::Mechanical);
    assert(bus_for_layer(AudioPerceptionLayer::EssentialInterface) == AudioBus::EssentialAlerts);
    assert(bus_for_layer(AudioPerceptionLayer::ContactVibration) == AudioBus::Mechanical);
    assert(bus_for_layer(AudioPerceptionLayer::ScientificSonification) == AudioBus::Sonification);
    assert(bus_for_layer(AudioPerceptionLayer::AdaptiveAmbience) == AudioBus::Ambience);
    assert(bus_for_layer(AudioPerceptionLayer::Songwriting) == AudioBus::Music);

    // Defaults: everything audible at full volume.
    {
        const AudioMixSettings settings;
        for (std::size_t i = 0; i < kAudioBusCount; ++i) {
            assert(settings.effective_gain(static_cast<AudioBus>(i)) == 1.0);
        }
    }

    // Master and bus volume multiply; each bus mutes independently.
    {
        AudioMixSettings settings;
        settings.master.volume = 0.5;
        settings.bus(AudioBus::Mechanical).volume = 0.5;
        assert(settings.effective_gain(AudioBus::Mechanical) == 0.25);
        assert(settings.effective_gain(AudioBus::Interface) == 0.5);
        settings.bus(AudioBus::Mechanical).muted = true;
        assert(settings.effective_gain(AudioBus::Mechanical) == 0.0);
        assert(settings.effective_gain(AudioBus::Interface) == 0.5);
        settings.master.muted = true;
        for (std::size_t i = 0; i < kAudioBusCount; ++i) {
            assert(settings.effective_gain(static_cast<AudioBus>(i)) == 0.0);
        }
    }

    // Invalid volumes: clamped, and non-finite resets to the default.
    {
        assert(AudioMixSettings::sanitize_volume(-3.0) == 0.0);
        assert(AudioMixSettings::sanitize_volume(7.0) == 1.0);
        assert(AudioMixSettings::sanitize_volume(std::numeric_limits<double>::quiet_NaN()) == 1.0);
        assert(AudioMixSettings::sanitize_volume(std::numeric_limits<double>::infinity()) == 1.0);
        AudioMixSettings bad;
        bad.master.volume = std::numeric_limits<double>::quiet_NaN();
        bad.bus(AudioBus::Music).volume = -1.0;
        assert(bad.effective_gain(AudioBus::Interface) == 1.0);
        assert(bad.effective_gain(AudioBus::Music) == 0.0);
        const AudioMixSettings clean = bad.sanitized();
        assert(clean.master.volume == 1.0);
        assert(clean.bus(AudioBus::Music).volume == 0.0);
    }

    // Delivery: gain/audible follow the mix; the event is never altered.
    {
        AudioMixSettings settings;
        settings.bus(AudioBus::Mechanical).volume = 0.5;
        const AudioDelivery contact = settings.deliver(event_with(AudioEventAction::Contact), "");
        assert(contact.bus == AudioBus::Mechanical);
        assert(contact.gain == 0.5 && contact.audible);
        assert(!contact.text_equivalent_required && contact.text_equivalent.empty());
        settings.bus(AudioBus::Mechanical).muted = true;
        const AudioDelivery muted = settings.deliver(event_with(AudioEventAction::Contact), "");
        assert(muted.gain == 0.0 && !muted.audible);
    }

    // Essential alerts always carry a text equivalent, audible or not, and
    // are rejected closed without one.
    {
        AudioMixSettings settings;
        const AudioEventEnvelope alert = event_with(AudioEventAction::EssentialAlert);
        const AudioDelivery loud = settings.deliver(alert, "Thermal limit exceeded");
        assert(loud.audible && loud.text_equivalent_required);
        assert(loud.text_equivalent == "Thermal limit exceeded");
        settings.master.muted = true;
        const AudioDelivery silent = settings.deliver(alert, "Thermal limit exceeded");
        assert(!silent.audible && silent.text_equivalent_required);
        assert(silent.text_equivalent == "Thermal limit exceeded");
        assert(throws([&] { (void)settings.deliver(alert, ""); }));
    }

    // Deliver validates the envelope like the gate does.
    {
        const AudioMixSettings settings;
        AudioEventEnvelope bad = event_with(AudioEventAction::Contact);
        bad.event_id.clear();
        assert(throws([&] { (void)settings.deliver(bad, ""); }));
    }

    // Save/reload round trip, including through text.
    {
        AudioMixSettings settings;
        settings.master.volume = 0.75;
        settings.bus(AudioBus::Music).muted = true;
        settings.bus(AudioBus::Ambience).volume = 0.25;
        const std::string text = settings.to_json().dump();
        const AudioMixLoadResult loaded = load_audio_mix_or_default(text);
        assert(!loaded.used_defaults);
        assert(loaded.settings.master.volume == 0.75 && !loaded.settings.master.muted);
        assert(loaded.settings.bus(AudioBus::Music).muted);
        assert(loaded.settings.bus(AudioBus::Ambience).volume == 0.25);
        assert(loaded.settings.bus(AudioBus::Mechanical).volume == 1.0);
        assert(loaded.settings.to_json().dump() == text);
    }

    // Reload sanitizes out-of-range stored volumes; a missing bus keeps its
    // default; strict loading rejects unsupported versions and bad types.
    {
        const AudioMixSettings loaded = AudioMixSettings::from_json(JsonValue::parse(
            R"({"schema_version":1,"master":{"volume":3.0,"muted":false},"buses":{"music":{"volume":-2.0,"muted":true}}})"));
        assert(loaded.master.volume == 1.0);
        assert(loaded.bus(AudioBus::Music).volume == 0.0 && loaded.bus(AudioBus::Music).muted);
        assert(loaded.bus(AudioBus::Interface).volume == 1.0);

        assert(throws([] {
            (void)AudioMixSettings::from_json(JsonValue::parse(
                R"({"schema_version":2,"master":{"volume":1.0,"muted":false},"buses":{}})"));
        }));
        assert(throws([] {
            (void)AudioMixSettings::from_json(JsonValue::parse(
                R"({"schema_version":1,"master":{"volume":"loud","muted":false},"buses":{}})"));
        }));
        assert(throws([] {
            (void)AudioMixSettings::from_json(JsonValue::parse(R"({"schema_version":1,"buses":{}})"));
        }));
    }

    // Startup reload never blocks the game: corrupt text yields defaults.
    {
        for (const char* text : {"", "not json", "[]", R"({"schema_version":9})", "{"}) {
            const AudioMixLoadResult loaded = load_audio_mix_or_default(text);
            assert(loaded.used_defaults);
            assert(loaded.settings.effective_gain(AudioBus::EssentialAlerts) == 1.0);
        }
    }

    // Settings writing never changes game truth: delivering twice yields the
    // identical presentation and leaves the settings untouched.
    {
        const AudioMixSettings settings;
        const std::string before = settings.to_json().dump();
        const AudioEventEnvelope alert = event_with(AudioEventAction::EssentialAlert);
        const AudioDelivery first = settings.deliver(alert, "Alert");
        const AudioDelivery second = settings.deliver(alert, "Alert");
        assert(first.gain == second.gain && first.text_equivalent == second.text_equivalent);
        assert(settings.to_json().dump() == before);
    }

    std::cout << "audio_mix_tests passed\n";
    return 0;
}
