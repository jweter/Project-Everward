#pragma once

#include <cstdint>
#include <string>

namespace everward::simulation {

struct Vector3d {
    double x{};
    double y{};
    double z{};
};

struct ProbeStateSnapshot {
    std::string probe_id{"EV-0001"};
    std::uint32_t generation{1};
    Vector3d position_m{};
    Vector3d velocity_mps{};
    double mass_kg{2500.0};
    double stored_energy_j{5.0e8};
    double energy_capacity_j{1.0e9};
    // Constant passive power supply applied every fixed step alongside
    // allocated consumption (see integrate_energy_balance() in core.hpp).
    // This models an RTG-style (radioisotope thermoelectric generator)
    // source rather than solar: solar generation would need the
    // star-distance/irradiance model that does not exist yet, while a
    // constant baseline is the self-contained addition available today.
    // Defaults to 0.0 so a bare, engine-neutral SimulationCore() (used
    // throughout the test suite, and still by UProbeSimulationAdapter in
    // unreal/) keeps an exact, generation-free energy/timing baseline.
    // set_energy_generation_w() configures it directly (e.g. for tests), and
    // SimulationCore::make_canonical_ev0001() in core.hpp is the one place
    // the canonical EV-0001 probe's real nonzero hardware-loadout value
    // lives, without disturbing this struct-level default.
    double energy_generation_w{0.0};
    // Set once stored_energy_j transitions from having stored energy to
    // having none, and cleared again once a net-positive energy balance
    // (energy_generation_w exceeding allocated consumption) recharges it
    // back above zero — see integrate_energy_balance() in core.hpp, which
    // also mirrors is_overheated's capability-lockout pattern below. With
    // the canonical probe's default energy_generation_w of 0.0 this remains
    // a one-way transition in practice today, exactly as before this field
    // existed, but the mechanism itself is no longer inherently one-way.
    bool is_energy_depleted{false};
    double temperature_k{293.15};
    double thermal_capacity_j_per_k{2.5e6};
    double ambient_temperature_k{293.15};
    double passive_cooling_w_per_k{2.0};
    // Order-of-magnitude placeholder for the temperature at which onboard
    // components can no longer operate safely (roughly 100C / 373.15K), not
    // a modeled material or component-specific failure point. Crossing it
    // triggers the overheat lockout below.
    double max_operating_temperature_k{373.15};
    // is_overheated and is_energy_depleted are two independent causes of the
    // same can_scan/can_thrust capability lockout; SimulationCore derives
    // both flags from `is_overheated || is_energy_depleted` each step so
    // that one cause clearing (e.g. cooling below max_operating_temperature_k)
    // does not wrongly restore capabilities while the other cause is still
    // active (e.g. stored_energy_j still at zero).
    bool is_overheated{false};
    double storage_used_kg{0.0};
    double storage_capacity_kg{500.0};
    bool can_scan{true};
    bool can_thrust{true};
    bool is_scanning{false};
    std::string active_scan_target_id{};
    double scan_remaining_s{0.0};
    double power_capacity_w{750.0};
    double power_allocated_sensors_w{0.0};
    double power_allocated_propulsion_w{0.0};
    double power_allocated_computation_w{0.0};
    double power_allocated_thermal_w{0.0};
};

enum class PowerSubsystem {
    Sensors,
    Propulsion,
    Computation,
    Thermal
};

enum class DomainEventType {
    SimulationAdvanced,
    ScanStarted,
    ScanCompleted,
    PowerAllocationChanged,
    EnergyDepleted,
    EnergyRestored,
    OverheatStarted,
    OverheatEnded,
    PolicyChanged,
    ManeuverStarted,
    ManeuverCompleted
};

struct DomainEvent {
    std::int64_t tick{};
    DomainEventType type{DomainEventType::SimulationAdvanced};
    std::string detail;
};

} // namespace everward::simulation
