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
        // this simulation. `integrate_overheat_response`, called right after
        // this from `advance_wall_ticks`, reacts to the resulting
        // `temperature_k` once it crosses `overheat_threshold_k`.
        const double decay_rate_per_s = cooling_w_per_k / probe_.thermal_capacity_j_per_k;
        const double equilibrium_k = probe_.ambient_temperature_k + heating_w / cooling_w_per_k;
        const double delta_from_equilibrium_k = probe_.temperature_k - equilibrium_k;
        probe_.temperature_k = equilibrium_k + delta_from_equilibrium_k * std::exp(-decay_rate_per_s * seconds);
    }

    // Overheat response: `overheat_threshold_k` (default 358.15 K / 85 degC)
    // is a physically plausible upper safe-operating bound for spacecraft-
    // grade electronics assemblies. It is set well above the default
    // `ambient_temperature_k` (293.15 K) so ordinary brief/low-power
    // operation never approaches it, but it is still comfortably reachable
    // within the probe's real power budget rather than an unreachable edge
    // case: given the default `passive_cooling_w_per_k` (2.0 W/K) and
    // `thermal_capacity_j_per_k`, any sustained total allocated power above
    // (overheat_threshold_k - ambient_temperature_k) * passive_cooling_w_per_k
    // = 130 W will eventually equilibrate at or above the threshold, well
    // within the default 750 W `power_capacity_w` budget.
    //
    // Crossing the threshold (ambient -> overheat transition) degrades
    // capability the same way an already-unavailable capability is gated
    // elsewhere in this class: `can_scan` and `can_thrust` are set false, so
    // `start_scan` and `set_velocity_mps` reject new commands exactly as
    // they already do when those flags are false for any other reason.
    // Neither an in-progress scan nor prior velocity are forcibly cancelled;
    // only new commands are blocked, mirroring the existing gating style.
    //
    // Recovery (the reverse transition) does not use the same threshold
    // value: it requires cooling to `overheat_threshold_k -
    // overheat_recovery_margin_k` (default 5 K below the threshold) before
    // capability is restored. This hysteresis band exists purely to prevent
    // rapid on/off toggling of `can_scan`/`can_thrust` (and the resulting
    // event spam) from small fluctuations right at the threshold, since
    // `temperature_k` can move in either direction step to step depending on
    // reallocated power.
    //
    // Each transition fires its domain event exactly once, mirroring
    // `EnergyDepleted`'s transition-only pattern: `Overheated` on entering
    // overheat, `OverheatRecovered` on the reverse transition. Neither event
    // repeats while the probe merely remains in its current state.
    void integrate_overheat_response() {
        if (!probe_.is_overheated && probe_.temperature_k >= probe_.overheat_threshold_k) {
            probe_.is_overheated = true;
            probe_.can_scan = false;
            probe_.can_thrust = false;
            events_.push_back({clock_.tick(), DomainEventType::Overheated,
                                "temperature crossed overheat threshold: can_scan/can_thrust degraded"});
        } else if (probe_.is_overheated &&
                   probe_.temperature_k <= probe_.overheat_threshold_k - probe_.overheat_recovery_margin_k) {
            probe_.is_overheated = false;
            probe_.can_scan = true;
            probe_.can_thrust = true;
            events_.push_back({clock_.tick(), DomainEventType::OverheatRecovered,
                                "temperature recovered below overheat threshold: can_scan/can_thrust restored"});
        }
    }

    SimulationClock clock_{};
    ProbeStateSnapshot probe_{};
    std::vector<DomainEvent> events_{};
};

} // namespace everward::simulation
