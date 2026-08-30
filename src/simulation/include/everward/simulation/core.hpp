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
    static constexpr double kCanonicalEv0001EnergyGenerationW = 75.0;

    SimulationCore() = default;

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
        require_finite_vector(velocity, "velocity");
        if (!probe_.can_thrust) {
            throw std::runtime_error("propulsion unavailable");
        }
        probe_.velocity_mps = velocity;
        events_.push_back({clock_.tick(), DomainEventType::ManeuverStarted, "velocity command accepted"});
    }

    void adjust_attitude_degrees(EulerAttitudeDegrees delta) {
        require_finite_attitude(delta);
        if (!probe_.can_thrust) {
            throw std::runtime_error("propulsion unavailable");
        }

        probe_.attitude_degrees.yaw =
            normalize_degrees(probe_.attitude_degrees.yaw + delta.yaw);
        probe_.attitude_degrees.pitch =
            normalize_degrees(probe_.attitude_degrees.pitch + delta.pitch);
        probe_.attitude_degrees.roll =
            normalize_degrees(probe_.attitude_degrees.roll + delta.roll);
        events_.push_back({
            clock_.tick(),
            DomainEventType::AttitudeChanged,
            "attitude trim accepted"
        });
    }

    void adjust_local_velocity_mps(Vector3d local_delta_velocity) {
        require_finite_vector(local_delta_velocity, "local velocity trim");
        if (!probe_.can_thrust) {
            throw std::runtime_error("propulsion unavailable");
        }

        const Vector3d world_delta = rotate_local_to_world(
            local_delta_velocity,
            probe_.attitude_degrees);
        probe_.velocity_mps.x += world_delta.x;
        probe_.velocity_mps.y += world_delta.y;
        probe_.velocity_mps.z += world_delta.z;
        events_.push_back({
            clock_.tick(),
            DomainEventType::ManeuverStarted,
            "probe-relative velocity trim accepted"
        });
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

    void cancel_scan() {
        if (!probe_.is_scanning) {
            throw std::runtime_error("no scan in progress");
        }

        const std::string cancelled_target = probe_.active_scan_target_id;
        probe_.is_scanning = false;
        probe_.active_scan_target_id.clear();
        probe_.scan_remaining_s = 0.0;
        events_.push_back({clock_.tick(), DomainEventType::ScanCancelled, "scan cancelled: " + cancelled_target});
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

    void set_energy_generation_w(double watts) {
        if (watts < 0.0) {
            throw std::invalid_argument("watts must be non-negative");
        }
        probe_.energy_generation_w = watts;
    }

    void set_passive_cooling_w_per_k(double watts_per_k) {
        if (watts_per_k < 0.0) {
            throw std::invalid_argument("watts_per_k must be non-negative");
        }
        probe_.passive_cooling_w_per_k = watts_per_k;
    }

    void set_max_operating_temperature_k(double kelvin) {
        if (!(kelvin > 0.0)) {
            throw std::invalid_argument("kelvin must be positive");
        }
        probe_.max_operating_temperature_k = kelvin;
    }

    // Contact detection lives in the engine-independent ProbeRuntime, while
    // this method is the sole mutation boundary for applying the resulting
    // physical resolution to authoritative probe state. Unreal may visualize
    // the same envelopes, but it does not author the mechanical outcome.
    void resolve_contact(
        const std::string& body_id,
        Vector3d resolved_position_m,
        Vector3d contact_point_m,
        Vector3d surface_normal,
        Vector3d relative_velocity_mps,
        double normal_speed_mps,
        Vector3d resolved_velocity_mps) {
        if (body_id.empty()) {
            throw std::invalid_argument("contact body_id must not be empty");
        }
        require_finite_vector(resolved_position_m, "resolved contact position");
        require_finite_vector(contact_point_m, "contact point");
        require_finite_vector(surface_normal, "contact surface normal");
        require_finite_vector(relative_velocity_mps, "contact relative velocity");
        require_finite_vector(resolved_velocity_mps, "resolved contact velocity");
        if (!std::isfinite(normal_speed_mps) || normal_speed_mps < 0.0) {
            throw std::invalid_argument("contact normal speed must be finite and non-negative");
        }

        probe_.position_m = resolved_position_m;
        probe_.velocity_mps = resolved_velocity_mps;
        probe_.has_contact_history = true;
        probe_.last_contact_body_id = body_id;
        probe_.last_contact_point_m = contact_point_m;
        probe_.last_contact_surface_normal = surface_normal;
        probe_.last_contact_relative_velocity_mps = relative_velocity_mps;
        probe_.last_contact_normal_speed_mps = normal_speed_mps;
        probe_.last_contact_tick = clock_.tick();
        events_.push_back({
            clock_.tick(),
            DomainEventType::Contact,
            "contact: " + body_id + " normal speed " + std::to_string(normal_speed_mps) + " m/s"
        });
    }

    // Mining and any future sample-collection mechanics must route extracted
    // mass through this authoritative mutator rather than accumulating it in
    // an adapter/engine-side counter: storage_used_kg is read directly by the
    // Unreal systems-panel HUD (see ProbeSimulationAdapter.cpp's telemetry
    // conversion), so simulation-owned state is the only place that can move
    // it without creating a truth split between what the HUD reports and
    // what was actually mined.
    void add_stored_material_kg(double kilograms) {
        if (!std::isfinite(kilograms) || kilograms < 0.0) {
            throw std::invalid_argument("stored material delta must be finite and non-negative");
        }
        const double updated_kg = probe_.storage_used_kg + kilograms;
        if (updated_kg > probe_.storage_capacity_kg + 1e-6) {
            throw std::runtime_error("stored material would exceed storage capacity");
        }
        probe_.storage_used_kg = updated_kg;
        events_.push_back({clock_.tick(), DomainEventType::MaterialStored,
                            "stored material increased by " + std::to_string(kilograms) + " kg"});
    }

    [[nodiscard]] double total_power_allocated_w() const noexcept {
        return probe_.power_allocated_sensors_w + probe_.power_allocated_propulsion_w +
               probe_.power_allocated_computation_w + probe_.power_allocated_thermal_w;
    }

private:
    static constexpr double kDegreesToRadians =
        3.141592653589793238462643383279502884 / 180.0;

    static void require_finite_vector(Vector3d value, const char* label) {
        if (!std::isfinite(value.x) || !std::isfinite(value.y) ||
            !std::isfinite(value.z)) {
            throw std::invalid_argument(std::string(label) + " must be finite");
        }
    }

    static void require_finite_attitude(EulerAttitudeDegrees value) {
        if (!std::isfinite(value.yaw) || !std::isfinite(value.pitch) ||
            !std::isfinite(value.roll)) {
            throw std::invalid_argument("attitude trim must be finite");
        }
    }

    [[nodiscard]] static double normalize_degrees(double degrees) noexcept {
        double normalized = std::fmod(degrees, 360.0);
        if (normalized >= 180.0) {
            normalized -= 360.0;
        } else if (normalized < -180.0) {
            normalized += 360.0;
        }
        return normalized;
    }

    [[nodiscard]] static Vector3d rotate_local_to_world(
        Vector3d local,
        EulerAttitudeDegrees attitude) noexcept {
        const double yaw = attitude.yaw * kDegreesToRadians;
        const double pitch = attitude.pitch * kDegreesToRadians;
        const double roll = attitude.roll * kDegreesToRadians;

        const double cy = std::cos(yaw);
        const double sy = std::sin(yaw);
        const double cp = std::cos(pitch);
        const double sp = std::sin(pitch);
        const double cr = std::cos(roll);
        const double sr = std::sin(roll);

        return {
            (cp * cy) * local.x + (sr * sp * cy - cr * sy) * local.y +
                (-(cr * sp * cy + sr * sy)) * local.z,
            (cp * sy) * local.x + (sr * sp * sy + cr * cy) * local.y +
                (cy * sr - cr * sp * sy) * local.z,
            sp * local.x + (-sr * cp) * local.y + (cr * cp) * local.z,
        };
    }

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
        if (!probe_.is_scanning || !probe_.can_scan) {
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
            if (heating_w > 0.0) {
                probe_.temperature_k += (heating_w * seconds) / probe_.thermal_capacity_j_per_k;
            }
            return;
        }

        const double decay_rate_per_s = cooling_w_per_k / probe_.thermal_capacity_j_per_k;
        const double equilibrium_k = probe_.ambient_temperature_k + heating_w / cooling_w_per_k;
        const double delta_from_equilibrium_k = probe_.temperature_k - equilibrium_k;
        probe_.temperature_k = equilibrium_k + delta_from_equilibrium_k * std::exp(-decay_rate_per_s * seconds);
    }

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
