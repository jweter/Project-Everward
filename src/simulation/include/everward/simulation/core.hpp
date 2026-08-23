#pragma once

#include "everward/simulation/clock.hpp"
#include "everward/simulation/types.hpp"

#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace everward::simulation {

class SimulationCore {
public:
    // Order-of-magnitude RTG (radioisotope thermoelectric generator) output
    // for the canonical EV-0001 probe's real hardware loadout — see
    // make_canonical_ev0001() below. Deliberately modest relative to the
    // wattages already exercised by this file's own test suite (100-750 W)
    // and to power_capacity_w (750 W): the probe still needs to budget power
    // deliberately rather than this trickle alone eliminating energy-
    // management tension, while still being enough to matter for genuinely
    // idle/low-power stretches. Comparable in order of magnitude to real
    // multi-mission RTGs (e.g. MMRTG, ~110 We), not a modeled specific unit.
    static constexpr double kCanonicalEv0001EnergyGenerationW = 75.0;

    SimulationCore() = default;

    // Returns a SimulationCore configured with the canonical EV-0001 probe's
    // real hardware loadout (currently just its passive generation source),
    // as distinct from the bare SimulationCore() default used everywhere
    // else today — including this file's own test suite, and
    // UProbeSimulationAdapter::BeginPlay() in unreal/, which still
    // constructs SimulationCore() directly. Switching that Unreal call site
    // to this factory is deliberately left for a future run with Unreal
    // build/verification capability (see PHASE2_KICKOFF_SCAFFOLD.md); this
    // sandboxed environment cannot compile/verify unreal/Source/ changes.
    // ProbeStateSnapshot::energy_generation_w's own struct default
    // deliberately stays at 0.0 rather than being edited directly: many
    // existing tests (EnergyConsumption, EnergyDepletionResponse, the
    // combined-lockout interaction test) construct a bare SimulationCore()
    // and rely on an exact, generation-free energy/timing baseline for their
    // closed-form expected-value math. This factory gives the canonical
    // probe's real loadout its own nonzero value without touching that
    // baseline or any of those tests.
    [[nodiscard]] static SimulationCore make_canonical_ev0001() {
        SimulationCore core;
        core.set_energy_generation_w(kCanonicalEv0001EnergyGenerationW);
        return core;
    }

    [[nodiscard]] const ProbeStateSnapshot& snapshot() const noexcept { return probe_; }
    [[nodiscard]] std::int64_t tick() const noexcept { return clock_.tick(); }

    void advance_wall_ticks(std::int64_t wall_ticks) {
        if (wall_ticks < 0) {
            throw std::invalid_argument("wall_ticks must be non-negative");
        }
        clock_.advance_by(wall_ticks);
        const double seconds = ticks_to_seconds(wall_ticks);
        integrate_probe(seconds);
        integrate_scan(seconds);
        integrate_energy_balance(seconds);
        integrate_thermal_load(seconds);
        integrate_overheat_response();
        refresh_capability_lockouts();
        events_.push_back({clock_.tick(), DomainEventType::SimulationAdvanced, "fixed step"});
    }

    [[nodiscard]] std::vector<DomainEvent> drain_events() {
        auto out = std::move(events_);
        events_.clear();
        return out;
    }

    void set_velocity_mps(Vector3d velocity) {
        if (!probe_.can_thrust) {
            throw std::runtime_error("propulsion unavailable");
        }
        probe_.velocity_mps = velocity;
        events_.push_back({clock_.tick(), DomainEventType::ManeuverStarted, "velocity command accepted"});
    }

    void start_scan(const std::string& target_id, double duration_s) {
        if (target_id.empty()) {
            throw std::invalid_argument("target_id must not be empty");
        }
        if (!(duration_s > 0.0)) {
            throw std::invalid_argument("duration_s must be positive");
        }
        if (!probe_.can_scan) {
            throw std::runtime_error("scanning unavailable");
        }
        if (probe_.is_scanning) {
            throw std::runtime_error("scan already in progress");
        }

        probe_.is_scanning = true;
        probe_.active_scan_target_id = target_id;
        probe_.scan_remaining_s = duration_s;
        events_.push_back({clock_.tick(), DomainEventType::ScanStarted, "scan started: " + target_id});
    }

    void allocate_power(PowerSubsystem subsystem, double watts) {
        if (watts < 0.0) {
            throw std::invalid_argument("watts must be non-negative");
        }
        if (watts > 0.0 && !subsystem_operational(subsystem)) {
            throw std::runtime_error(subsystem_name(subsystem) + " subsystem unavailable");
        }

        double sensors = probe_.power_allocated_sensors_w;
        double propulsion = probe_.power_allocated_propulsion_w;
        double computation = probe_.power_allocated_computation_w;
        double thermal = probe_.power_allocated_thermal_w;

        switch (subsystem) {
            case PowerSubsystem::Sensors:
                sensors = watts;
                break;
            case PowerSubsystem::Propulsion:
                propulsion = watts;
                break;
            case PowerSubsystem::Computation:
                computation = watts;
                break;
            case PowerSubsystem::Thermal:
                thermal = watts;
                break;
        }

        const double requested_total = sensors + propulsion + computation + thermal;
        if (requested_total > probe_.power_capacity_w) {
            throw std::runtime_error("power allocation exceeds capacity");
        }

        probe_.power_allocated_sensors_w = sensors;
        probe_.power_allocated_propulsion_w = propulsion;
        probe_.power_allocated_computation_w = computation;
        probe_.power_allocated_thermal_w = thermal;

        events_.push_back({clock_.tick(), DomainEventType::PowerAllocationChanged,
                            subsystem_name(subsystem) + " allocation set to " + std::to_string(watts) + " W"});
    }

    // Sets explicit hardware-operational state without inventing a wear or
    // damage trigger before Phase 2 defines one. This is the component-level
    // equivalent of set_energy_generation_w(): a deterministic configuration
    // hook that later mechanics can call. A failure immediately sheds that
    // subsystem's allocation and blocks positive reallocation; sensors and
    // propulsion also feed the command capability derivation below.
    void set_subsystem_operational(PowerSubsystem subsystem, bool operational) {
        bool& current = subsystem_operational_ref(subsystem);
        if (current == operational) {
            return;
        }

        current = operational;
        if (!operational) {
            subsystem_allocation_ref(subsystem) = 0.0;
        }
        refresh_capability_lockouts();
        events_.push_back({clock_.tick(), DomainEventType::SubsystemOperationalStateChanged,
                            subsystem_name(subsystem) + (operational ? " restored" : " failed")});
    }

    // Configures the probe's constant passive power supply (see
    // ProbeStateSnapshot::energy_generation_w in types.hpp for what this
    // models and why it defaults to 0.0). This is a hardware/loadout
    // configuration hook rather than a player-facing maneuver command like
    // allocate_power(), so it does not itself emit a domain event. The
    // meaningful event is integrate_energy_balance()'s EnergyRestored,
    // fired when a resulting net-positive balance actually recharges
    // stored_energy_j back above zero.
    void set_energy_generation_w(double watts) {
        if (watts < 0.0) {
            throw std::invalid_argument("watts must be non-negative");
        }
        probe_.energy_generation_w = watts;
    }

    [[nodiscard]] double total_power_allocated_w() const noexcept {
        return probe_.power_allocated_sensors_w + probe_.power_allocated_propulsion_w +
               probe_.power_allocated_computation_w + probe_.power_allocated_thermal_w;
    }

private:
    [[nodiscard]] bool subsystem_operational(PowerSubsystem subsystem) const noexcept {
        switch (subsystem) {
            case PowerSubsystem::Sensors:
                return probe_.sensors_operational;
            case PowerSubsystem::Propulsion:
                return probe_.propulsion_operational;
            case PowerSubsystem::Computation:
                return probe_.computation_operational;
            case PowerSubsystem::Thermal:
                return probe_.thermal_operational;
        }
        return false;
    }

    bool& subsystem_operational_ref(PowerSubsystem subsystem) noexcept {
        switch (subsystem) {
            case PowerSubsystem::Sensors:
                return probe_.sensors_operational;
            case PowerSubsystem::Propulsion:
                return probe_.propulsion_operational;
            case PowerSubsystem::Computation:
                return probe_.computation_operational;
            case PowerSubsystem::Thermal:
                return probe_.thermal_operational;
        }
        return probe_.computation_operational;
    }

    double& subsystem_allocation_ref(PowerSubsystem subsystem) noexcept {
        switch (subsystem) {
            case PowerSubsystem::Sensors:
                return probe_.power_allocated_sensors_w;
            case PowerSubsystem::Propulsion:
                return probe_.power_allocated_propulsion_w;
            case PowerSubsystem::Computation:
                return probe_.power_allocated_computation_w;
            case PowerSubsystem::Thermal:
                return probe_.power_allocated_thermal_w;
        }
        return probe_.power_allocated_computation_w;
    }

    [[nodiscard]] static std::string subsystem_name(PowerSubsystem subsystem) {
        switch (subsystem) {
            case PowerSubsystem::Sensors:
                return "sensors";
            case PowerSubsystem::Propulsion:
                return "propulsion";
            case PowerSubsystem::Computation:
                return "computation";
            case PowerSubsystem::Thermal:
                return "thermal";
        }
        return "unknown";
    }
    [[nodiscard]] static double ticks_to_seconds(std::int64_t ticks) noexcept {
        return static_cast<double>(ticks) / static_cast<double>(SimulationClock::TicksPerSecond);
    }

    void integrate_probe(double seconds) noexcept {
        probe_.position_m.x += probe_.velocity_mps.x * seconds;
        probe_.position_m.y += probe_.velocity_mps.y * seconds;
        probe_.position_m.z += probe_.velocity_mps.z * seconds;
    }

    void integrate_scan(double seconds) {
        if (!probe_.is_scanning) {
            return;
        }

        probe_.scan_remaining_s -= seconds;
        if (probe_.scan_remaining_s <= 0.0) {
            const std::string completed_target = probe_.active_scan_target_id;
            probe_.is_scanning = false;
            probe_.active_scan_target_id.clear();
            probe_.scan_remaining_s = 0.0;
            events_.push_back({clock_.tick(), DomainEventType::ScanCompleted, "scan complete: " + completed_target});
        }
    }

    // Energy balance: allocated power draws down stored_energy_j exactly as
    // before, but the probe's constant passive generation source
    // (energy_generation_w — see its own doc comment in types.hpp for why
    // this models an RTG-style constant supply rather than solar) now
    // offsets that draw every fixed step. The net of the two determines
    // whether stored_energy_j falls, rises, or stays exactly put over the
    // elapsed step: consumption exceeding generation still depletes as
    // before (clamped at zero); generation exceeding consumption now
    // recharges (clamped at energy_capacity_j); and an exact match is a
    // genuine no-op, matching this integration's original zero-consumption
    // no-op case (the canonical probe's default energy_generation_w of 0.0
    // means that original no-op case is unchanged in practice).
    //
    // Energy-depletion/restoration response: once stored_energy_j is drawn
    // down to zero, the probe locks out scanning and propulsion the same way
    // integrate_overheat_response() does for temperature_k, via the shared
    // is_overheated/is_energy_depleted -> can_scan/can_thrust derivation in
    // refresh_capability_lockouts(). This is edge-triggered: EnergyDepleted
    // fires once on the > 0 -> 0 transition and the new EnergyRestored fires
    // once on the 0 -> > 0 transition (mirroring OverheatStarted/
    // OverheatEnded), not on every step spent steadily at either end. This
    // closes the one-way-lockout gap the prior slice deliberately left open
    // pending this mechanic: see ProbeStateSnapshot::is_energy_depleted's own
    // comment in types.hpp.
    void integrate_energy_balance(double seconds) {
        const double net_rate_w = probe_.energy_generation_w - total_power_allocated_w();
        if (net_rate_w == 0.0 || seconds <= 0.0) {
            return;
        }

        const double previous_energy_j = probe_.stored_energy_j;
        double updated_energy_j = previous_energy_j + net_rate_w * seconds;
        if (updated_energy_j < 0.0) {
            updated_energy_j = 0.0;
        } else if (updated_energy_j > probe_.energy_capacity_j) {
            updated_energy_j = probe_.energy_capacity_j;
        }
        probe_.stored_energy_j = updated_energy_j;

        if (previous_energy_j > 0.0 && updated_energy_j <= 0.0) {
            probe_.is_energy_depleted = true;
            events_.push_back({clock_.tick(), DomainEventType::EnergyDepleted,
                                "stored energy depleted by allocated power draw"});
        } else if (previous_energy_j <= 0.0 && updated_energy_j > 0.0) {
            probe_.is_energy_depleted = false;
            events_.push_back({clock_.tick(), DomainEventType::EnergyRestored,
                                "stored energy restored above zero by passive generation"});
        }
    }

    void integrate_thermal_load(double seconds) noexcept {
        if (seconds <= 0.0) {
            return;
        }

        const double heating_w = total_power_allocated_w();
        const double cooling_w_per_k = probe_.passive_cooling_w_per_k;

        if (cooling_w_per_k <= 0.0) {
            // No passive cooling pathway configured: fall back to the pure
            // waste-heat accumulation used before this slice.
            if (heating_w > 0.0) {
                probe_.temperature_k += (heating_w * seconds) / probe_.thermal_capacity_j_per_k;
            }
            return;
        }

        // All allocated power is treated as waste heat dissipated into the
        // probe's thermal mass (thermal_capacity_j_per_k), the same
        // simplification used for the stored-energy draw above. Passive
        // radiative/conductive cooling toward probe_.ambient_temperature_k is
        // modeled as Newtonian cooling proportional to the temperature
        // difference (passive_cooling_w_per_k, in watts per kelvin above
        // ambient):
        //
        //   dT/dt = (heating_w - cooling_w_per_k * (T - T_ambient)) / thermal_capacity_j_per_k
        //
        // This has a closed-form solution that decays exponentially toward a
        // fixed equilibrium temperature (T_ambient + heating_w / cooling_w_per_k)
        // under sustained heating, rather than climbing unbounded. Solving it
        // exactly (instead of a fixed-step Euler update) keeps the result
        // correct and step-size-independent for any elapsed duration,
        // matching how large `advance_wall_ticks` calls behave elsewhere in
        // this simulation. See integrate_overheat_response() below for the
        // behavioral response once this crosses max_operating_temperature_k.
        const double decay_rate_per_s = cooling_w_per_k / probe_.thermal_capacity_j_per_k;
        const double equilibrium_k = probe_.ambient_temperature_k + heating_w / cooling_w_per_k;
        const double delta_from_equilibrium_k = probe_.temperature_k - equilibrium_k;
        probe_.temperature_k = equilibrium_k + delta_from_equilibrium_k * std::exp(-decay_rate_per_s * seconds);
    }

    // Temperature-limit/overheat response: once temperature_k reaches or
    // exceeds max_operating_temperature_k, the probe locks out scanning and
    // propulsion (mirrors the ScanCommand/allocate_power capability-gating
    // pattern) until it cools back below the threshold. This is edge-triggered
    // on the is_overheated flag rather than re-applied every step, matching
    // EnergyDepleted's transition-only event pattern: OverheatStarted fires
    // once on crossing into the lockout and OverheatEnded once on recovering
    // from it, not on every step spent at/above or below the threshold.
    //
    // can_scan/can_thrust now have a second independent source of truth,
    // is_energy_depleted (see integrate_energy_balance() above), so this
    // no longer restores them directly: it only updates is_overheated and
    // its transition events, and leaves the actual capability derivation to
    // refresh_capability_lockouts(), which combines both causes.
    void integrate_overheat_response() {
        const bool exceeds_limit = probe_.temperature_k >= probe_.max_operating_temperature_k;

        if (exceeds_limit && !probe_.is_overheated) {
            probe_.is_overheated = true;
            events_.push_back({clock_.tick(), DomainEventType::OverheatStarted,
                                "temperature_k reached max_operating_temperature_k"});
        } else if (!exceeds_limit && probe_.is_overheated) {
            probe_.is_overheated = false;
            events_.push_back({clock_.tick(), DomainEventType::OverheatEnded,
                                "temperature_k dropped below max_operating_temperature_k"});
        }
    }

    // Derives can_scan/can_thrust from every current lockout cause. Called
    // once per advance_wall_ticks step after both integrate_energy_balance
    // (is_energy_depleted) and integrate_overheat_response (is_overheated)
    // have updated their flags for that step, so neither cause's recovery
    // (e.g. cooling back below max_operating_temperature_k) can wrongly
    // restore capabilities while the other cause is still active (e.g.
    // stored_energy_j still at zero), and vice versa.
    void refresh_capability_lockouts() noexcept {
        const bool probe_locked_out = probe_.is_overheated || probe_.is_energy_depleted;
        probe_.can_scan = !probe_locked_out && probe_.sensors_operational;
        probe_.can_thrust = !probe_locked_out && probe_.propulsion_operational;
    }

    SimulationClock clock_{};
    ProbeStateSnapshot probe_{};
    std::vector<DomainEvent> events_{};
};

} // namespace everward::simulation
