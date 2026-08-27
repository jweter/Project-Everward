#include "everward/simulation/software_policy.hpp"

#undef NDEBUG
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using everward::simulation::DomainEvent;
using everward::simulation::DomainEventType;
using everward::simulation::PolicyActionKind;
using everward::simulation::PolicyConditionKind;
using everward::simulation::PowerSubsystem;
using everward::simulation::ProbeRuntime;
using everward::simulation::SimulationClock;
using everward::simulation::SoftwarePolicy;

namespace {
bool has_event(const std::vector<DomainEvent>& events, DomainEventType type) {
    for (const auto& event : events) {
        if (event.type == type) {
            return true;
        }
    }
    return false;
}

bool has_event_detail(
    const std::vector<DomainEvent>& events,
    DomainEventType type,
    const std::string& expected_fragment) {
    for (const auto& event : events) {
        if (event.type == type && event.detail.find(expected_fragment) != std::string::npos) {
            return true;
        }
    }
    return false;
}

SoftwarePolicy make_basic_sensor_shed_policy() {
    SoftwarePolicy policy;
    policy.id = "gen1_basic_survival_test";
    policy.rules = {
        {
            "shed_sensors_below_60_percent_energy",
            PolicyConditionKind::EnergyFractionBelow,
            0.60,
            PolicyActionKind::SetPowerAllocation,
            PowerSubsystem::Sensors,
            0.0,
        },
    };
    return policy;
}
}

int main() {
    // The canonical embodied Generation-1 probe begins with enough sensor
    // power to scan and enough computation power to run its tiny policy slot.
    // The two allocations exactly match its 75 W passive source.
    {
        ProbeRuntime runtime = ProbeRuntime::make_canonical_ev0001();
        assert(runtime.snapshot().power_allocated_sensors_w ==
               ProbeRuntime::kGeneration1MinimumSensorPowerW);
        assert(runtime.snapshot().power_allocated_computation_w ==
               ProbeRuntime::kGeneration1MinimumPolicyComputationPowerW);
        assert(runtime.total_power_allocated_w() == 75.0);
        assert(runtime.policy_status().executor_available);

        runtime.start_scan("power-ready-target", 5.0);
        assert(runtime.snapshot().is_scanning);
    }

    // Sensor allocation is now a real mechanical consequence. Dropping below
    // the Generation-1 operating floor aborts an active scan and rejects new
    // scans until sufficient power is restored.
    {
        ProbeRuntime runtime = ProbeRuntime::make_canonical_ev0001();
        runtime.start_scan("power-loss-target", 10.0);
        (void)runtime.drain_events();

        runtime.allocate_power(
            PowerSubsystem::Sensors,
            ProbeRuntime::kGeneration1MinimumSensorPowerW - 25.0);
        assert(!runtime.snapshot().is_scanning);

        const auto underpower_events = runtime.drain_events();
        assert(has_event(underpower_events, DomainEventType::PowerAllocationChanged));
        assert(has_event(underpower_events, DomainEventType::ScanCancelled));

        bool rejected = false;
        std::string rejection;
        try {
            runtime.start_scan("still-underpowered", 5.0);
        } catch (const std::runtime_error& error) {
            rejected = true;
            rejection = error.what();
        }
        assert(rejected);
        assert(rejection.find("minimum operating power") != std::string::npos);
        assert(rejection.find("50 W") != std::string::npos);

        runtime.allocate_power(
            PowerSubsystem::Sensors,
            ProbeRuntime::kGeneration1MinimumSensorPowerW);
        runtime.start_scan("restored-power-target", 5.0);
        assert(runtime.snapshot().is_scanning);
    }

    // Automation uses the same runtime power command and therefore creates
    // the same sensor consequence. It also emits a player-readable cause and
    // effect statement instead of an opaque rule-id-only event.
    {
        ProbeRuntime runtime = ProbeRuntime::make_canonical_ev0001();
        runtime.start_scan("automation-target", 10.0);
        runtime.install_policy(make_basic_sensor_shed_policy());
        (void)runtime.drain_events();

        // EV-0001 begins at 50% stored energy, so the <60% rule fires after
        // the next deterministic runtime step.
        runtime.advance_wall_ticks(SimulationClock::TicksPerSecond);
        assert(runtime.snapshot().power_allocated_sensors_w == 0.0);
        assert(!runtime.snapshot().is_scanning);

        const auto events = runtime.drain_events();
        assert(has_event(events, DomainEventType::ScanCancelled));
        assert(has_event_detail(
            events,
            DomainEventType::PolicyRuleTriggered,
            "AUTOMATION: Sensors 50 W -> 0 W // energy reserve below 60%"));
    }

    std::cout << "Everward subsystem consequence tests passed\n";
    return 0;
}