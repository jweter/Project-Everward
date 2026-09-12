#pragma once

#include "everward/simulation/science_knowledge.hpp"

#include <array>
#include <cstdint>
#include <map>
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

// Authoritative local-space contact samples for the Prime Generation-1 body.
// This is the first step away from the temporary 8 m bounding sphere. The
// samples deliberately approximate the long central hull plus the two broad
// radiator/wing regions while remaining simple enough for deterministic swept
// sphere-vs-sphere contact tests in the engine-independent runtime.
struct ProbeCollisionSphereSample {
    Vector3d local_center_m{};
    double radius_m{1.0};
};

struct ProbeCompoundCollisionEnvelope {
    static constexpr std::size_t SampleCount = 5;

    std::array<ProbeCollisionSphereSample, SampleCount> samples{{
        // Forward science/hull section.
        {{5.0, 0.0, 0.0}, 1.35},
        // Central computation/reactor hull.
        {{0.0, 0.0, 0.0}, 1.60},
        // Aft propulsion/hull section.
        {{-5.0, 0.0, 0.0}, 1.50},
        // Port radiator/wing region.
        {{-0.5, -3.0, 0.0}, 1.00},
        // Starboard radiator/wing region.
        {{-0.5, 3.0, 0.0}, 1.00},
    }};
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

    // Authoritative shape description. ProbeRuntime's swept-contact solver
    // (see software_policy.hpp) sweeps these five local-space samples,
    // rotated by attitude_degrees, against physical bodies instead of one
    // oversized bounding sphere.
    ProbeCompoundCollisionEnvelope compound_collision_envelope{};

    // Legacy single-sphere radius. The swept-contact solver no longer
    // consumes this value; it is retained only as a coarse diagnostic/
    // telemetry figure (e.g. the existing Unreal collision-envelope display)
    // until presentation-side consumers migrate to the compound envelope.
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
    // Per-material breakdown of storage_used_kg by material_id (e.g.
    // "raw_regolith"). The sum of values must always equal storage_used_kg;
    // SimulationCore::add_stored_material_kg()/consume_stored_material_kg()
    // are the sole mutation boundary that keeps this invariant, the same way
    // storage_used_kg itself is never touched outside those methods. A
    // std::map (rather than unordered_map) keeps save serialization and any
    // future inventory readout deterministically ordered by material_id.
    // Save data captured before this field existed has no entry here; see
    // save_data.hpp's material_inventory_kg deserialization for how a
    // pre-existing storage_used_kg is inferred into this breakdown.
    std::map<std::string, double> material_inventory_kg{};
    // Slice 11 ("Science as gameplay") foundation: per-target accumulated
    // knowledge from observation, keyed by target_id. science_knowledge.hpp
    // is the engine-independent read/mutate model; SimulationCore's
    // integrate_scan() is the sole authoritative accumulation point (see its
    // observe_active_scan_progress()), the same way material_inventory_kg
    // above is only ever mutated through add/consume_stored_material_kg().
    // A std::map keeps save serialization deterministically ordered by
    // target_id. Absent for any target never observed, and absent entirely
    // on saves captured before this field existed (see save_data.hpp).
    std::map<std::string, TargetKnowledgeState> target_knowledge{};
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
    Contact,
    MaterialStored,
    MaterialConsumed,
    EnergyConsumed
};

struct DomainEvent {
    std::int64_t tick{};
    DomainEventType type{DomainEventType::SimulationAdvanced};
    std::string detail;
};

} // namespace everward::simulation
