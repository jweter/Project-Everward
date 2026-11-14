#include "everward/simulation/audio_events.hpp"

#undef NDEBUG
#include <cassert>
#include <iostream>
#include <limits>

using everward::simulation::AudioEventAction;
using everward::simulation::AudioEventAdmission;
using everward::simulation::AudioEventEnvelope;
using everward::simulation::AudioEventGate;
using everward::simulation::AudioPerceptionLayer;
using everward::simulation::AudioPerceptionState;

namespace {

AudioEventEnvelope essential_alert(std::string event_id) {
    AudioEventEnvelope event;
    event.event_id = std::move(event_id);
    event.simulation_tick = 10;
    event.stable_audio_seed = 42;
    event.source_entity_id = "EV-0001";
    event.action = AudioEventAction::EssentialAlert;
    event.bypass_perception_gate = true;
    event.significance = 1.0;
    return event;
}

} // namespace

int main() {
    // An essential alert is admitted exactly once; a second delivery of the
    // same authoritative event_id is suppressed, never a second trigger.
    {
        AudioEventGate gate;
        const AudioPerceptionState perception{};
        const AudioEventEnvelope event = essential_alert("alert:thermal:0001");
        assert(gate.admit(event, perception) == AudioEventAdmission::Admitted);
        assert(gate.admit(event, perception) == AudioEventAdmission::DuplicateSuppressed);
        assert(gate.admit(event, perception) == AudioEventAdmission::DuplicateSuppressed);
        assert(gate.delivered_event_count() == 1);
    }

    // Two distinct real occurrences (distinct event_id) both admit.
    {
        AudioEventGate gate;
        const AudioPerceptionState perception{};
        assert(gate.admit(essential_alert("alert:thermal:0001"), perception) == AudioEventAdmission::Admitted);
        assert(gate.admit(essential_alert("alert:thermal:0002"), perception) == AudioEventAdmission::Admitted);
        assert(gate.delivered_event_count() == 2);
    }

    // A non-essential, sensorium-gated event (contact vibration) is
    // correctly silent on the sparse Generation-1 baseline and becomes
    // audible once that perception layer is actually installed -- without
    // ever retriggering for the same event_id.
    {
        AudioEventGate gate;
        AudioEventEnvelope contact;
        contact.event_id = "contact:EV-0001:775";
        contact.simulation_tick = 775;
        contact.stable_audio_seed = 7;
        contact.source_entity_id = "EV-0001";
        contact.action = AudioEventAction::Contact;
        contact.bypass_perception_gate = false;
        contact.required_perception_layer = AudioPerceptionLayer::ContactVibration;
        contact.contact_force_n = 12.5;
        contact.significance = 0.4;

        AudioPerceptionState sparse{};
        assert(!sparse.contact_vibration);
        assert(gate.admit(contact, sparse) == AudioEventAdmission::GatedBySensorium);
        // Gating does not count as delivery: the same real occurrence must
        // still be able to admit later once perception is upgraded, rather
        // than being permanently lost the first time it arrived too early.
        assert(gate.delivered_event_count() == 0);

        AudioPerceptionState upgraded{};
        upgraded.contact_vibration = true;
        assert(gate.admit(contact, upgraded) == AudioEventAdmission::Admitted);
        assert(gate.delivered_event_count() == 1);
        // A duplicate delivery of the exact same occurrence after it was
        // admitted is still suppressed, regardless of perception state.
        assert(gate.admit(contact, upgraded) == AudioEventAdmission::DuplicateSuppressed);
    }

    // Empty identity fields refuse closed rather than silently fabricating
    // an event; audio code never gets to invent a source identity.
    {
        AudioEventGate gate;
        const AudioPerceptionState perception{};
        AudioEventEnvelope missing_id = essential_alert("");
        bool threw = false;
        try {
            (void)gate.admit(missing_id, perception);
        } catch (const std::invalid_argument&) {
            threw = true;
        }
        assert(threw);

        AudioEventEnvelope missing_source = essential_alert("alert:power:0003");
        missing_source.source_entity_id.clear();
        threw = false;
        try {
            (void)gate.admit(missing_source, perception);
        } catch (const std::invalid_argument&) {
            threw = true;
        }
        assert(threw);
        // A rejected envelope must never count as delivered.
        assert(gate.delivered_event_count() == 0);
    }

    // Invalid/NaN/out-of-range measurement values are clamped to a safe,
    // finite representation rather than aborting an otherwise-real event or
    // passing NaN downstream into synthesis parameters. validate() exposes
    // exactly the sanitized values a renderer would actually consume.
    {
        AudioEventEnvelope event = essential_alert("alert:power:0004");
        event.significance = std::numeric_limits<double>::quiet_NaN();
        event.contact_force_n = -5.0;
        event.tool_load_fraction = 3.5;
        event.damage_delta = std::numeric_limits<double>::infinity();
        event.integrity_delta = -std::numeric_limits<double>::infinity();

        const AudioEventEnvelope sanitized = AudioEventGate::validate(event);
        assert(sanitized.significance == 0.0);
        assert(sanitized.contact_force_n.has_value() && *sanitized.contact_force_n == 0.0);
        assert(sanitized.tool_load_fraction.has_value() && *sanitized.tool_load_fraction == 1.0);
        assert(sanitized.damage_delta.has_value() && *sanitized.damage_delta == 0.0);
        assert(sanitized.integrity_delta.has_value() && *sanitized.integrity_delta == 0.0);

        AudioEventGate gate;
        const AudioPerceptionState perception{};
        assert(gate.admit(event, perception) == AudioEventAdmission::Admitted);
    }

    std::cout << "Audio event gate tests passed\n";
    return 0;
}
