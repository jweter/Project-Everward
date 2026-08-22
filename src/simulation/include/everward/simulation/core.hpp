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
        // this simulation. A temperature limit/overheat response remains a
        // separate, currently-unspecified follow-up; see PROJECT_STATUS.md.
        const double decay_rate_per_s = cooling_w_per_k / probe_.thermal_capacity_j_per_k;
        const double equilibrium_k = probe_.ambient_temperature_k + heating_w / cooling_w_per_k;
        const double delta_from_equilibrium_k = probe_.temperature_k - equilibrium_k;
        probe_.temperature_k = equilibrium_k + delta_from_equilibrium_k * std::exp(-decay_rate_per_s * seconds);
    }

    // Temperature-limit/overheat response. Once temperature_k reaches
    // probe_.overheat_temperature_k (a placeholder threshold, in the same
    // spirit as thermal_capacity_j_per_k/passive_cooling_w_per_k/
    // ambient_temperature_k, pending real balancing), the probe degrades
    // can_scan/can_thrust to false so start_scan()/set_velocity_mps() reject
    // new commands, and emits OverheatBegan exactly once on the
    // not-overheating -> overheating transition (mirroring EnergyDepleted's
    // transition-only pattern). Once temperature_k drops back below the same
    // threshold, both capabilities are restored and OverheatEnded fires once
    // on the reverse transition. There is deliberately no separate hysteresis
    // band yet: both directions cross at the same threshold, so a
    // temperature oscillating exactly at the boundary can emit repeated
    // Began/Ended pairs; that refinement is left for a future balancing pass
    // rather than folded silently into this slice.
    //
    // This intentionally does not force-cancel an in-progress scan or zero
    // out an already-commanded velocity when overheating begins — it only
    // gates the start of new scan/thrust commands, consistent with how
    // can_scan/can_thrust already only guard command entry points elsewhere
    // in this file. Forcibly interrupting in-flight scans/maneuvers on
    // overheat, and any behavioral response to full energy depletion, remain
    // separate, currently-unspecified follow-ups.
    void integrate_overheat_response() {
        const bool was_overheating = probe_.is_overheating;
        const bool now_overheating = probe_.temperature_k >= probe_.overheat_temperature_k;

        if (now_overheating && !was_overheating) {
            probe_.is_overheating = true;
            probe_.can_scan = false;
            probe_.can_thrust = false;
            events_.push_back({clock_.tick(), DomainEventType::OverheatBegan,
                                "temperature reached overheat threshold"});
        } else if (!now_overheating && was_overheating) {
            probe_.is_overheating = false;
            probe_.can_scan = true;
            probe_.can_thrust = true;
            events_.push_back({clock_.tick(), DomainEventType::OverheatEnded,
                                "temperature dropped below overheat threshold"});
        }
    }

    SimulationClock clock_{};
    ProbeStateSnapshot probe_{};
    std::vector<DomainEvent> events_{};
};

} // namespace everward::simulation
