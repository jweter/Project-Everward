#pragma once

#include "everward/simulation/software_policy.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace everward::simulation {

enum class ImpactSeverity {
    Contact,
    Light,
    Damaging,
    Severe,
    Catastrophic
};

enum class IntegrityBand {
    Offline,
    Critical,
    Degraded,
    Operational,
    Nominal
};

struct ComponentIntegritySnapshot {
    double sensors{1.0};
    double propulsion{1.0};
    double computation{1.0};
    double thermal{1.0};
};

struct ImpactDamageRecord {
    std::int64_t tick{0};
    std::string body_id;
    double normal_speed_mps{0.0};
    double impact_energy_j{0.0};
    ImpactSeverity severity{ImpactSeverity::Contact};
    PowerSubsystem affected_subsystem{PowerSubsystem::Sensors};
    double integrity_before{1.0};
    double integrity_after{1.0};
};

class ImpactDamageModel {
public:
    static constexpr double kLightImpactEnergyJ = 25'000.0;
    static constexpr double kDamagingImpactEnergyJ = 100'000.0;
    static constexpr double kSevereImpactEnergyJ = 500'000.0;
    static constexpr double kCatastrophicImpactEnergyJ = 2'000'000.0;

    // First-pass Generation-1 structural calibration. This is deliberately
    // energy based rather than an arbitrary hit-point subtraction. Later
    // material/geometry models may replace this calibration while preserving
    // the component-integrity contract.
    static constexpr double kEnergyForFullComponentLossJ = 2'500'000.0;

    [[nodiscard]] const ComponentIntegritySnapshot& integrity() const noexcept {
        return integrity_;
    }

    [[nodiscard]] bool has_last_impact() const noexcept {
        return last_impact_.has_value();
    }

    [[nodiscard]] const std::optional<ImpactDamageRecord>& last_impact() const noexcept {
        return last_impact_;
    }

    [[nodiscard]] static ImpactSeverity classify_impact(double impact_energy_j) {
        if (!std::isfinite(impact_energy_j) || impact_energy_j < 0.0) {
            throw std::invalid_argument("impact energy must be finite and non-negative");
        }
        if (impact_energy_j < kLightImpactEnergyJ) {
            return ImpactSeverity::Contact;
        }
        if (impact_energy_j < kDamagingImpactEnergyJ) {
            return ImpactSeverity::Light;
        }
        if (impact_energy_j < kSevereImpactEnergyJ) {
            return ImpactSeverity::Damaging;
        }
        if (impact_energy_j < kCatastrophicImpactEnergyJ) {
            return ImpactSeverity::Severe;
        }
        return ImpactSeverity::Catastrophic;
    }

    [[nodiscard]] static double damage_fraction_for_energy(double impact_energy_j) {
        if (!std::isfinite(impact_energy_j) || impact_energy_j < 0.0) {
            throw std::invalid_argument("impact energy must be finite and non-negative");
        }
        if (impact_energy_j < kLightImpactEnergyJ) {
            return 0.0;
        }
        return std::clamp(impact_energy_j / kEnergyForFullComponentLossJ, 0.0, 1.0);
    }

    [[nodiscard]] static IntegrityBand integrity_band(double fraction) {
        validate_integrity(fraction);
        if (fraction <= 0.0) return IntegrityBand::Offline;
        if (fraction < 0.25) return IntegrityBand::Critical;
        if (fraction < 0.75) return IntegrityBand::Degraded;
        if (fraction < 1.0) return IntegrityBand::Operational;
        return IntegrityBand::Nominal;
    }

    [[nodiscard]] static bool is_functional(double fraction) {
        validate_integrity(fraction);
        return fraction > 0.0;
    }

    [[nodiscard]] double subsystem_integrity(PowerSubsystem subsystem) const noexcept {
        return integrity_ref(subsystem);
    }

    void set_subsystem_integrity(PowerSubsystem subsystem, double fraction) {
        validate_integrity(fraction);
        integrity_ref(subsystem) = fraction;
    }

    [[nodiscard]] std::optional<ImpactDamageRecord> assess_latest_contact(ProbeRuntime& runtime) {
        const ProbeStateSnapshot& snapshot = runtime.snapshot();
        if (!snapshot.has_contact_history) {
            return std::nullopt;
        }
        if (has_assessed_contact_ && snapshot.last_contact_tick == last_assessed_contact_tick_) {
            return std::nullopt;
        }

        has_assessed_contact_ = true;
        last_assessed_contact_tick_ = snapshot.last_contact_tick;

        const double impact_energy_j =
            0.5 * snapshot.mass_kg * snapshot.last_contact_normal_speed_mps *
            snapshot.last_contact_normal_speed_mps;
        const ImpactSeverity severity = classify_impact(impact_energy_j);
        const PowerSubsystem affected = affected_subsystem_for_contact(snapshot);
        const double before = subsystem_integrity(affected);
        const double damage_fraction = damage_fraction_for_energy(impact_energy_j);
        const double after = std::clamp(before - damage_fraction, 0.0, 1.0);
        set_subsystem_integrity(affected, after);

        if (before > 0.0 && after <= 0.0) {
            runtime.set_subsystem_operational(affected, false);
        }

        ImpactDamageRecord record;
        record.tick = snapshot.last_contact_tick;
        record.body_id = snapshot.last_contact_body_id;
        record.normal_speed_mps = snapshot.last_contact_normal_speed_mps;
        record.impact_energy_j = impact_energy_j;
        record.severity = severity;
        record.affected_subsystem = affected;
        record.integrity_before = before;
        record.integrity_after = after;
        last_impact_ = record;
        return record;
    }

    [[nodiscard]] static PowerSubsystem affected_subsystem_for_contact(
        const ProbeStateSnapshot& snapshot) noexcept {
        // Contact normals point from the struck body toward the probe. The
        // impacted side of the probe therefore points in the opposite
        // direction. Convert that direction into probe-local coordinates so
        // rotating the spacecraft rotates its vulnerable component zones too.
        const Vector3d impact_side_world{
            -snapshot.last_contact_surface_normal.x,
            -snapshot.last_contact_surface_normal.y,
            -snapshot.last_contact_surface_normal.z,
        };
        const Vector3d local = rotate_world_to_local(
            impact_side_world,
            snapshot.attitude_degrees);

        const double ax = std::fabs(local.x);
        const double ay = std::fabs(local.y);
        const double az = std::fabs(local.z);

        // Temporary Prime-probe damage zones for the spherical Phase-2 body:
        // forward -> sensors, aft -> propulsion, lateral -> computation,
        // dorsal/ventral -> thermal. Slice 5 will replace this coarse zoning
        // with component-corresponding geometry while retaining the contract.
        if (ax >= ay && ax >= az) {
            return local.x >= 0.0 ? PowerSubsystem::Sensors : PowerSubsystem::Propulsion;
        }
        if (az >= ay) {
            return PowerSubsystem::Thermal;
        }
        return PowerSubsystem::Computation;
    }

    [[nodiscard]] static const char* severity_name(ImpactSeverity severity) noexcept {
        switch (severity) {
            case ImpactSeverity::Contact: return "CONTACT";
            case ImpactSeverity::Light: return "LIGHT";
            case ImpactSeverity::Damaging: return "DAMAGING";
            case ImpactSeverity::Severe: return "SEVERE";
            case ImpactSeverity::Catastrophic: return "CATASTROPHIC";
        }
        return "UNKNOWN";
    }

    [[nodiscard]] static const char* integrity_band_name(IntegrityBand band) noexcept {
        switch (band) {
            case IntegrityBand::Offline: return "OFFLINE";
            case IntegrityBand::Critical: return "CRITICAL";
            case IntegrityBand::Degraded: return "DEGRADED";
            case IntegrityBand::Operational: return "OPERATIONAL";
            case IntegrityBand::Nominal: return "NOMINAL";
        }
        return "UNKNOWN";
    }

private:
    static void validate_integrity(double fraction) {
        if (!std::isfinite(fraction) || fraction < 0.0 || fraction > 1.0) {
            throw std::invalid_argument("component integrity must be finite and within [0, 1]");
        }
    }

    [[nodiscard]] double& integrity_ref(PowerSubsystem subsystem) noexcept {
        switch (subsystem) {
            case PowerSubsystem::Sensors: return integrity_.sensors;
            case PowerSubsystem::Propulsion: return integrity_.propulsion;
            case PowerSubsystem::Computation: return integrity_.computation;
            case PowerSubsystem::Thermal: return integrity_.thermal;
        }
        return integrity_.computation;
    }

    [[nodiscard]] const double& integrity_ref(PowerSubsystem subsystem) const noexcept {
        switch (subsystem) {
            case PowerSubsystem::Sensors: return integrity_.sensors;
            case PowerSubsystem::Propulsion: return integrity_.propulsion;
            case PowerSubsystem::Computation: return integrity_.computation;
            case PowerSubsystem::Thermal: return integrity_.thermal;
        }
        return integrity_.computation;
    }

    [[nodiscard]] static Vector3d rotate_world_to_local(
        Vector3d world,
        EulerAttitudeDegrees attitude) noexcept {
        constexpr double DegreesToRadians =
            3.141592653589793238462643383279502884 / 180.0;
        const double yaw = attitude.yaw * DegreesToRadians;
        const double pitch = attitude.pitch * DegreesToRadians;
        const double roll = attitude.roll * DegreesToRadians;

        const double cy = std::cos(yaw);
        const double sy = std::sin(yaw);
        const double cp = std::cos(pitch);
        const double sp = std::sin(pitch);
        const double cr = std::cos(roll);
        const double sr = std::sin(roll);

        const double r00 = cp * cy;
        const double r01 = sr * sp * cy - cr * sy;
        const double r02 = -(cr * sp * cy + sr * sy);
        const double r10 = cp * sy;
        const double r11 = sr * sp * sy + cr * cy;
        const double r12 = cy * sr - cr * sp * sy;
        const double r20 = sp;
        const double r21 = -sr * cp;
        const double r22 = cr * cp;

        return {
            r00 * world.x + r10 * world.y + r20 * world.z,
            r01 * world.x + r11 * world.y + r21 * world.z,
            r02 * world.x + r12 * world.y + r22 * world.z,
        };
    }

    ComponentIntegritySnapshot integrity_{};
    bool has_assessed_contact_{false};
    std::int64_t last_assessed_contact_tick_{0};
    std::optional<ImpactDamageRecord> last_impact_{};
};

// Transitional production wrapper for Slice 4. It composes the already-tested
// ProbeRuntime instead of duplicating collision, power, scanning, or software
// policy truth. A single call to advance_wall_ticks() therefore performs
// movement/contact first and then derives deterministic damage from the
// authoritative contact record.
class DamageAwareProbeRuntime {
public:
    DamageAwareProbeRuntime() = default;
    explicit DamageAwareProbeRuntime(ProbeRuntime runtime)
        : runtime_(std::move(runtime)) {}

    [[nodiscard]] static DamageAwareProbeRuntime make_canonical_ev0001() {
        return DamageAwareProbeRuntime(ProbeRuntime::make_canonical_ev0001());
    }

    [[nodiscard]] const ProbeStateSnapshot& snapshot() const noexcept { return runtime_.snapshot(); }
    [[nodiscard]] std::int64_t tick() const noexcept { return runtime_.tick(); }
    [[nodiscard]] const ComponentIntegritySnapshot& component_integrity() const noexcept {
        return damage_.integrity();
    }
    [[nodiscard]] const std::optional<ImpactDamageRecord>& last_impact() const noexcept {
        return damage_.last_impact();
    }

    void advance_wall_ticks(std::int64_t wall_ticks) {
        runtime_.advance_wall_ticks(wall_ticks);
        const auto damage_record = damage_.assess_latest_contact(runtime_);
        if (damage_record.has_value()) {
            pending_damage_records_.push_back(*damage_record);
        }
    }

    [[nodiscard]] std::vector<DomainEvent> drain_events() { return runtime_.drain_events(); }

    [[nodiscard]] std::vector<ImpactDamageRecord> drain_damage_records() {
        auto out = std::move(pending_damage_records_);
        pending_damage_records_.clear();
        return out;
    }

    void add_static_sphere_body(StaticSphereBody body) {
        runtime_.add_static_sphere_body(std::move(body));
    }
    void clear_static_bodies() noexcept { runtime_.clear_static_bodies(); }
    [[nodiscard]] const std::vector<StaticSphereBody>& static_bodies() const noexcept {
        return runtime_.static_bodies();
    }

    void set_velocity_mps(Vector3d velocity) { runtime_.set_velocity_mps(velocity); }

    // Damaged propulsion remains usable above zero integrity, but each manual
    // trim has proportionally less authority. A 5% propulsion system can
    // therefore move, just extremely badly, which is central to the intended
    // damaged-machine progression model.
    void adjust_attitude_degrees(EulerAttitudeDegrees delta) {
        const double effectiveness = subsystem_integrity(PowerSubsystem::Propulsion);
        delta.yaw *= effectiveness;
        delta.pitch *= effectiveness;
        delta.roll *= effectiveness;
        runtime_.adjust_attitude_degrees(delta);
    }
    void adjust_local_velocity_mps(Vector3d local_delta_velocity) {
        const double effectiveness = subsystem_integrity(PowerSubsystem::Propulsion);
        local_delta_velocity.x *= effectiveness;
        local_delta_velocity.y *= effectiveness;
        local_delta_velocity.z *= effectiveness;
        runtime_.adjust_local_velocity_mps(local_delta_velocity);
    }

    // Sensor damage increases the time needed to complete the same scan. The
    // underlying power/availability checks still run first in ProbeRuntime;
    // integrity modifies performance without creating a parallel scan system.
    void start_scan(const std::string& target_id, double duration_s) {
        const double effectiveness = std::max(
            0.05,
            subsystem_integrity(PowerSubsystem::Sensors));
        runtime_.start_scan(target_id, duration_s / effectiveness);
    }
    void cancel_scan() { runtime_.cancel_scan(); }
    void allocate_power(PowerSubsystem subsystem, double watts) {
        runtime_.allocate_power(subsystem, watts);
    }
    void set_subsystem_operational(PowerSubsystem subsystem, bool operational) {
        runtime_.set_subsystem_operational(subsystem, operational);
    }

    // This is the future awakening/self-repair bridge: the same component
    // integrity model used by impacts can initialize a damaged probe and later
    // accept staged repair results. Any positive integrity is mechanically
    // functional in Generation 1; zero integrity is offline.
    void set_subsystem_integrity(PowerSubsystem subsystem, double fraction) {
        damage_.set_subsystem_integrity(subsystem, fraction);
        runtime_.set_subsystem_operational(subsystem, fraction > 0.0);
    }

    [[nodiscard]] double subsystem_integrity(PowerSubsystem subsystem) const noexcept {
        return damage_.subsystem_integrity(subsystem);
    }

    [[nodiscard]] IntegrityBand subsystem_integrity_band(PowerSubsystem subsystem) const {
        return ImpactDamageModel::integrity_band(subsystem_integrity(subsystem));
    }

    void set_energy_generation_w(double watts) { runtime_.set_energy_generation_w(watts); }
    void set_passive_cooling_w_per_k(double watts_per_k) {
        runtime_.set_passive_cooling_w_per_k(watts_per_k);
    }
    void set_max_operating_temperature_k(double kelvin) {
        runtime_.set_max_operating_temperature_k(kelvin);
    }
    [[nodiscard]] double total_power_allocated_w() const noexcept {
        return runtime_.total_power_allocated_w();
    }
    void add_stored_material_kg(double kilograms) { runtime_.add_stored_material_kg(kilograms); }

    void install_policy(SoftwarePolicy policy) { runtime_.install_policy(std::move(policy)); }
    void clear_policy() { runtime_.clear_policy(); }
    [[nodiscard]] SoftwarePolicyStatus policy_status() const noexcept {
        return runtime_.policy_status();
    }
    [[nodiscard]] const SoftwarePolicy* active_policy() const noexcept {
        return runtime_.active_policy();
    }

    void select_nearest_target(double max_selection_range_m) {
        runtime_.select_nearest_target(max_selection_range_m);
    }
    void select_target(const std::string& body_id) { runtime_.select_target(body_id); }
    void clear_target_selection() noexcept { runtime_.clear_target_selection(); }
    [[nodiscard]] TargetSelectionStatus selected_target_status() const noexcept {
        return runtime_.selected_target_status();
    }

private:
    ProbeRuntime runtime_{};
    ImpactDamageModel damage_{};
    std::vector<ImpactDamageRecord> pending_damage_records_{};
};

} // namespace everward::simulation
