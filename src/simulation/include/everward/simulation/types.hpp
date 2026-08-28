#pragma once

#include <cstdint>
#include <string>

namespace everward::simulation {

struct Vector3d {
    double x{};
    double y{};
    double z{};
};

struct EulerAttitudeDegrees {
    double yaw{};
    double pitch{};
    double roll{};
};

// Engine-independent physical body used by the first Phase-2 contact solver.
// Bodies are intentionally simple spheres for this foundation slice. More
// detailed shape/mesh support can replace or extend this representation later
// without making Unreal Engine the owner of collision truth.
struct StaticSphereBody {
    std::string body_id;
    Vector3d center_m{};
    double radius_m{1.0};
};

struct ProbeStateSnapshot {
    std::string probe_id{"EV-0001"};
    std::uint32_t generation{1};
    Vector3d position_m{};
    Vector3d velocity_mps{};
    // Authoritative Generation-1 attitude. Euler angles are sufficient for
    // the current deterministic command-driven embodiment pass; a future
    // rigid-body flight model may replace the representation behind this
    // read model without moving mechanical truth into Unreal.
    EulerAttitudeDegrees attitude_degrees{};
    double mass_kg{2500.0};

    // Conservative Prime Generation-1 blockout collision envelope. The first
    // recognizable ~15 m body is represented by an 8 m bounding sphere so
    // contact occurs near the visible fore/aft/radiator extents rather than at
    // the obsolete 0.75 m engineering-shell placeholder. A later compound
    // envelope can refine this without changing the contact telemetry contract.
    double collision_envelope_radius_m{8.0};
    bool has_contact_history{false};
    std::string last_contact_body_id{};
    Vector3d last_contact_point_m{};
    Vector3d last_contact_surface_normal{};
    Vector3d last_contact_relative_velocity_mps{};
    double last_contact_normal_speed_mps{0.0};
    std::int64_t last_contact_tick{0};

    double stored_energy_j{5.0e8};
    double energy_capacity_j{1.0e9};
    // Constant passive power supply applied every fixed step alongside
    // allocated consumption (see integrate_energy_balance() in core.hpp).
    // This models an RTG-style (radioisotope thermoelectric generator)
    // source rather than solar: solar generation would need the
    // star-distance/irradiance model that does not exist yet, while a
    // constant baseline is the self-contained addition available today.
    // Defaults to 0.0 so a bare, engine-neutral SimulationCore() keeps an
    // exact, generation-free energy/timing baseline.
    double energy_generation_w{0.0};
    // Set once stored_energy_j transitions from having stored energy to
    // having none, and cleared again once a net-positive energy balance
    // recharges it back above zero.
    bool is_energy_depleted{false};
    double temperature_k{293.15};
    double thermal_capacity_j_per_k{2.5e6};
    double ambient_temperature_k{293.15};
    double passive_cooling_w_per_k{2.0};
    double max_operating_temperature_k{373.15};
    bool is_overheated{false};
    double storage_used_kg{0.0};
    double storage_capacity_kg{500.0};
    bool can_scan{true};
    bool can_thrust{true};
    // Independent hardware-operational state for each power subsystem.
    bool sensors_operational{true};
    bool propulsion_operational{true};
    bool computation_operational{true};
    bool thermal_operational{true};
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
    ScanCancelled,
    PowerAllocationChanged,
    SubsystemOperationalStateChanged,
    EnergyDepleted,
    EnergyRestored,
    OverheatStarted,
    OverheatEnded,
    PolicyChanged,
    PolicyRuleTriggered,
    PolicyActionRejected,
    AttitudeChanged,
    ManeuverStarted,
    ManeuverCompleted,
    Contact
};

struct DomainEvent {
    std::int64_t tick{};
    DomainEventType type{DomainEventType::SimulationAdvanced};
    std::string detail;
};

} // namespace everward::simulation
