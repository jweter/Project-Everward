#include "everward/simulation/sensor_power_guidance.hpp"
#include "everward/simulation/software_policy.hpp"

#undef NDEBUG
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

using namespace everward::simulation;

namespace {

SoftwarePolicy sensor_conservation_policy() {
    SoftwarePolicy policy;
    policy.id = "conserve_sensor_power";
    policy.rules = {
        SoftwarePolicyRule{
            "shed_sensors_below_60_percent",
            PolicyConditionKind::EnergyFractionBelow,
            0.60,
            PolicyActionKind::SetPowerAllocation,
            PowerSubsystem::Sensors,
            0.0,
        },
    };
    return policy;
}

SensorPowerGuidance guidance_for(const ProbeRuntime& runtime) {
    return runtime.scan_power_guidance();
}

bool contains(const std::string& text, const std::string& needle) {
    return text.find(needle) != std::string::npos;
}

std::string scan_refusal(ProbeRuntime& runtime) {
    try {
        runtime.start_scan("rock", 5.0);
    } catch (const std::runtime_error& error) {
        return error.what();
    }
    return {};
}

bool scan_refused(ProbeRuntime& runtime) {
    return !scan_refusal(runtime).empty();
}

} // namespace

int main() {
    constexpr double kFloor = 50.0;

    // At or above the floor there is nothing to explain.
    {
        const SensorPowerGuidance at_floor = sensor_power_guidance(50.0, kFloor, false, false, "");
        assert(at_floor.state == SensorPowerState::Ready && at_floor.message.empty());
        const SensorPowerGuidance plenty = sensor_power_guidance(250.0, kFloor, true, true, "p");
        assert(plenty.state == SensorPowerState::Ready && plenty.message.empty());
    }

    // Below the floor with no enabled policy: a plain raise-the-allocation message.
    {
        const SensorPowerGuidance manual = sensor_power_guidance(0.0, kFloor, false, false, "");
        assert(manual.state == SensorPowerState::BelowFloorManual);
        assert(contains(manual.message, "0 W") && contains(manual.message, "50 W"));
        assert(contains(manual.message, "Raise the Sensors power allocation"));
        assert(!contains(manual.message, "policy"));

        // A policy that is installed but disabled is not blamed.
        const SensorPowerGuidance disabled = sensor_power_guidance(10.0, kFloor, true, false, "p");
        assert(disabled.state == SensorPowerState::BelowFloorManual);
    }

    // Below the floor with an enabled policy: names it and gives an ordered recovery path.
    {
        const SensorPowerGuidance policy = sensor_power_guidance(0.0, kFloor, true, true, "conserve_sensor_power");
        assert(policy.state == SensorPowerState::BelowFloorPolicyInstalled);
        assert(contains(policy.message, "conserve_sensor_power"));
        assert(contains(policy.message, "1) clear or disable the policy"));
        assert(contains(policy.message, "2) raise Sensors to at least 50 W"));
        assert(contains(policy.message, "3) scan again"));
    }

    // Bad inputs fail closed: never Ready.
    {
        const double nan = std::numeric_limits<double>::quiet_NaN();
        const double inf = std::numeric_limits<double>::infinity();
        assert(sensor_power_guidance(nan, kFloor, false, false, "").state == SensorPowerState::BelowFloorManual);
        assert(sensor_power_guidance(-5.0, kFloor, false, false, "").state == SensorPowerState::BelowFloorManual);
        assert(sensor_power_guidance(inf, kFloor, false, false, "").state == SensorPowerState::BelowFloorManual);
        assert(sensor_power_guidance(100.0, nan, false, false, "").state == SensorPowerState::BelowFloorManual);
        assert(sensor_power_guidance(100.0, 0.0, false, false, "").state == SensorPowerState::BelowFloorManual);
        assert(sensor_power_guidance(100.0, -1.0, false, false, "").state == SensorPowerState::BelowFloorManual);
        assert(contains(sensor_power_guidance(nan, kFloor, false, false, "").message, "an unknown level"));
        assert(contains(sensor_power_guidance(100.0, nan, false, false, "").message, "the scan minimum"));
    }

    // The real G09 scenario against the actual simulation: an installed policy sheds the sensors,
    // the manual restore is shed again, the scan is refused, and the guidance names the policy.
    {
        ProbeRuntime runtime;
        runtime.allocate_power(
            PowerSubsystem::Computation, ProbeRuntime::kGeneration1MinimumPolicyComputationPowerW);
        runtime.install_policy(sensor_conservation_policy());
        runtime.advance_wall_ticks(SimulationClock::TicksPerSecond);
        assert(runtime.snapshot().power_allocated_sensors_w == 0.0);

        const SensorPowerGuidance dropped = guidance_for(runtime);
        assert(dropped.state == SensorPowerState::BelowFloorPolicyInstalled);
        assert(contains(dropped.message, "conserve_sensor_power"));
        const auto refusal = scan_refusal(runtime);
        assert(contains(refusal, dropped.message));
        assert(dropped.compact_message == "Clear policy; raise power; scan");
        assert(dropped.compact_message.size() <= 48);
        // HUD command row reserves 14 characters for CMD REJECTED: and 3 for ellipsis.
        assert(("CMD REJECTED: " + refusal).substr(0, 47).find(dropped.compact_message) != std::string::npos);
        assert(contains(refusal, "1) clear or disable the policy"));
        assert(contains(refusal, "2) raise Sensors"));
        assert(contains(refusal, "3) scan again"));

        // Raising sensors alone does not hold: the policy sheds them on the next evaluation.
        runtime.allocate_power(PowerSubsystem::Sensors, 80.0);
        runtime.advance_wall_ticks(SimulationClock::TicksPerSecond);
        assert(runtime.snapshot().power_allocated_sensors_w == 0.0);
        assert(guidance_for(runtime).state == SensorPowerState::BelowFloorPolicyInstalled);

        // Following the advice in order (clear the policy, raise Sensors, scan) recovers.
        runtime.clear_policy();
        assert(!runtime.policy_status().installed);
        runtime.allocate_power(PowerSubsystem::Sensors, 100.0);
        runtime.advance_wall_ticks(SimulationClock::TicksPerSecond);
        assert(runtime.snapshot().power_allocated_sensors_w == 100.0);
        assert(guidance_for(runtime).state == SensorPowerState::Ready);
        assert(!scan_refused(runtime));
    }

    // With no policy at all, simply low sensors get the manual advice and raising them recovers.
    {
        ProbeRuntime runtime;
        runtime.allocate_power(PowerSubsystem::Sensors, 10.0);
        const SensorPowerGuidance low = guidance_for(runtime);
        assert(low.state == SensorPowerState::BelowFloorManual);
        const auto refusal = scan_refusal(runtime);
        assert(contains(refusal, low.message));
        assert(low.compact_message == "Raise Sensors to minimum; scan");
        assert(low.compact_message.size() <= 48);
        assert(("CMD REJECTED: " + refusal).substr(0, 47).find(low.compact_message) != std::string::npos);
        assert(contains(refusal, "Raise the Sensors power allocation"));
        assert(!contains(refusal, "clear or disable the policy"));

        runtime.allocate_power(PowerSubsystem::Sensors, 60.0);
        assert(guidance_for(runtime).state == SensorPowerState::Ready);
        assert(!scan_refused(runtime));
    }

    // The guidance reads state only: asking never changes allocation or policy.
    {
        ProbeRuntime runtime;
        runtime.allocate_power(PowerSubsystem::Sensors, 0.0);
        runtime.install_policy(sensor_conservation_policy());
        const double before = runtime.snapshot().power_allocated_sensors_w;
        (void)guidance_for(runtime);
        (void)guidance_for(runtime);
        assert(runtime.snapshot().power_allocated_sensors_w == before);
        assert(runtime.policy_status().installed);
    }

    std::cout << "sensor_power_guidance_tests passed\n";
    return 0;
}
