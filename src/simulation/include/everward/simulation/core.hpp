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
    SimulationCore() = default;

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
        integrate_power_consumption(seconds);
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

    [[nodiscard]] double total_power_allocated_w() const noexcept {
        return probe_.power_allocated_sensors_w + probe_.power_allocated_propulsion_w +
               probe_.power_allocated_computation_w + probe_.power_allocated_thermal_w;
    }

private:
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

    // Energy-depletion response: once stored_energy_j is drawn down to zero,
    // the probe locks out scanning and propulsion the same way
    // integrate_overheat_response() does for temperature_k, via the shared
    // is_overheated/is_energy_depleted -> can_scan/can_thrust derivation in
    // refresh_capability_lockouts(). This is edge-triggered on
    // is_energy_depleted, reusing the existing EnergyDepleted event as the
    // >0 -> 0 transition marker rather than adding a second event for the
    // same transition.
    //
    // Unlike is_overheated, there is currently no mechanic that raises
    // stored_energy_j back above zero (no charge/generation slice exists
    // yet), so is_energy_depleted has no recovery branch here: it would be
    // dead code no test could reach honestly. A future energy-recharge slice
    // that can raise stored_energy_j above zero again should add the
    // corresponding 0 -> >0 transition (clearing is_energy_depleted and
    // emitting a recovery event) at that point.
    void integrate_power_consumption(double seconds) {
        const double draw_w = total_power_allocated_w();
        if (draw_w <= 0.0 || seconds <= 0.0) {
            return;
        }

        const double previous_energy_j = probe_.stored_energy_j;
        double remaining_energy_j = previous_energy_j - draw_w * seconds;
        if (remaining_energy_j < 0.0) {
            remaining_energy_j = 0.0;
        }
        probe_.stored_energy_j = remaining_energy_j;

        if (previous_energy_j > 0.0 && remaining_energy_j <= 0.0) {
            probe_.is_energy_depleted = true;
            events_.push_back({clock_.tick(), DomainEventType::EnergyDepleted,
                                "stored energy depleted by allocated power draw"});
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
    // is_energy_depleted (see integrate_power_consumption() above), so this
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
    // once per advance_wall_ticks step after both integrate_power_consumption
    // (is_energy_depleted) and integrate_overheat_response (is_overheated)
    // have updated their flags for that step, so neither cause's recovery
    // (e.g. cooling back below max_operating_temperature_k) can wrongly
    // restore capabilities while the other cause is still active (e.g.
    // stored_energy_j still at zero), and vice versa.
    void refresh_capability_lockouts() noexcept {
        const bool locked_out = probe_.is_overheated || probe_.is_energy_depleted;
        probe_.can_scan = !locked_out;
        probe_.can_thrust = !locked_out;
    }

    SimulationClock clock_{};
    ProbeStateSnapshot probe_{};
    std::vector<DomainEvent> events_{};
};

} // namespace everward::simulation
