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
    double temperature_k{293.15};
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
