#pragma once

#include <cmath>
#include <string>
#include <string_view>

namespace everward::simulation {

// Issue #378 G09: the Aura run reported sensor power falling from 250 W to 0 W around policy
// install. The deterministic tests show that is the installed policy doing its job (a rule such
// as "shed sensors below 60% stored energy" re-applies the same authoritative power command every
// evaluation, so a manual restore is shed again). What the player lacked was an explanation and a
// recovery path. This is that explanation: a pure, engine-independent decision the HUD can show
// next to the existing "sensors below minimum operating power" scan refusal.
//
// It only reads values the caller already has; it never writes simulation state. Non-finite or
// negative inputs fail closed to "below the floor", never to "ready".
enum class SensorPowerState {
    // Sensors meet the scan floor; nothing to explain.
    Ready,
    // Below the floor and an enabled policy is installed: it may be shedding sensor power again.
    BelowFloorPolicyInstalled,
    // Below the floor with no enabled policy: the allocation is simply too low.
    BelowFloorManual
};

struct SensorPowerGuidance {
    SensorPowerState state{SensorPowerState::Ready};
    std::string message;
    // Action-first text fits both the 48-character status and 50-character command row.
    std::string compact_message;
};

[[nodiscard]] inline SensorPowerGuidance sensor_power_guidance(
    double sensor_allocation_w,
    double minimum_scan_power_w,
    bool policy_installed,
    bool policy_enabled,
    std::string_view policy_id) {
    SensorPowerGuidance guidance;
    const bool floor_valid = std::isfinite(minimum_scan_power_w) && minimum_scan_power_w > 0.0;
    const bool allocation_valid = std::isfinite(sensor_allocation_w) && sensor_allocation_w >= 0.0;
    if (floor_valid && allocation_valid && sensor_allocation_w >= minimum_scan_power_w) {
        return guidance;  // Ready
    }

    const long floor_watts = floor_valid ? std::lround(minimum_scan_power_w) : 0;
    const std::string floor_text = floor_valid ? std::to_string(floor_watts) + " W" : "the scan minimum";
    const std::string current_text =
        allocation_valid ? std::to_string(std::lround(sensor_allocation_w)) + " W" : "an unknown level";

    if (policy_installed && policy_enabled) {
        guidance.state = SensorPowerState::BelowFloorPolicyInstalled;
        guidance.compact_message = "Clear policy; raise power; scan";
        guidance.message =
            "Sensors are at " + current_text + "; scanning needs " + floor_text + ". The installed policy '" +
            std::string(policy_id) +
            "' can shed sensor power again whenever its rule fires, so raising sensors alone may not hold. "
            "To recover: 1) clear or disable the policy, 2) raise Sensors to at least " + floor_text +
            ", 3) scan again.";
        return guidance;
    }

    guidance.state = SensorPowerState::BelowFloorManual;
    guidance.compact_message = "Raise Sensors to minimum; scan";
    guidance.message =
        "Sensors are at " + current_text + "; scanning needs " + floor_text +
        ". Raise the Sensors power allocation to at least " + floor_text + " and scan again.";
    return guidance;
}

} // namespace everward::simulation
