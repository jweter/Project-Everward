#pragma once

#include "everward/simulation/compound_contact.hpp"
#include "everward/simulation/core.hpp"
#include "everward/simulation/target_selection.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace everward::simulation {

enum class PolicyConditionKind {
    EnergyFractionBelow,
    EnergyFractionAbove,
    TemperatureAboveKelvin,
    TemperatureBelowKelvin
};

enum class PolicyActionKind {
    SetPowerAllocation
};

struct SoftwarePolicyRule {
    std::string id;
    PolicyConditionKind condition{PolicyConditionKind::EnergyFractionBelow};
    double threshold{0.0};
    PolicyActionKind action{PolicyActionKind::SetPowerAllocation};
    PowerSubsystem subsystem{PowerSubsystem::Sensors};
    double action_watts{0.0};
};

struct SoftwarePolicy {
    std::string id;
    bool enabled{true};
    std::vector<SoftwarePolicyRule> rules;
};

struct SoftwarePolicyStatus {
    bool installed{false};
    bool enabled{false};
    bool executor_available{false};
    std::string policy_id;
    std::size_t rule_count{0};
    double minimum_computation_power_w{0.0};
};

// Slice 7 (PHASE2_VERTICAL_SLICE_PLAN.md) runtime/telemetry wiring over
// target_selection.hpp's engine-independent math. has_selection is false for
// no selection, an empty/unknown requested id, or a selection whose body has
// since been deregistered -- selected_target_status() never reports a stale
// id as still selected, matching this codebase's other fail-closed
// registered-body lookups (see manipulator_hull_contact.hpp).
struct TargetSelectionStatus {
    bool has_selection{false};
    std::string body_id;
    double surface_range_m{0.0};
    double closing_speed_mps{0.0};
};

// Authoritative One-Probe runtime wrapper. SimulationCore owns the probe's
// mechanical state and integration. ProbeRuntime adds world-contact constraints
// and software policy evaluation without moving either responsibility into
// Unreal Engine. Static sphere bodies are intentionally the first simple shape
// representation; later planetary/mesh bodies can extend this contract.
class ProbeRuntime {
public:
    static constexpr std::size_t kGeneration1MaxPolicyRules = 2;
    static constexpr double kGeneration1MinimumPolicyComputationPowerW = 25.0;
    static constexpr double kGeneration1MinimumSensorPowerW = 50.0;

    ProbeRuntime() = default;
    explicit ProbeRuntime(SimulationCore core) : core_(std::move(core)) {}

    [[nodiscard]] static ProbeRuntime make_canonical_ev0001() {
        ProbeRuntime runtime(SimulationCore::make_canonical_ev0001());
        runtime.core_.allocate_power(PowerSubsystem::Sensors, kGeneration1MinimumSensorPowerW);
        runtime.core_.allocate_power(
            PowerSubsystem::Computation,
            kGeneration1MinimumPolicyComputationPowerW);
        (void)runtime.core_.drain_events();
        return runtime;
    }

    [[nodiscard]] const ProbeStateSnapshot& snapshot() const noexcept { return core_.snapshot(); }
    [[nodiscard]] std::int64_t tick() const noexcept { return core_.tick(); }

    void advance_wall_ticks(std::int64_t wall_ticks) {
        const Vector3d start_position = core_.snapshot().position_m;
        core_.advance_wall_ticks(wall_ticks);
        resolve_static_contacts(start_position);
        evaluate_policy();
    }

    [[nodiscard]] std::vector<DomainEvent> drain_events() {
        auto events = core_.drain_events();
        events.insert(events.end(), policy_events_.begin(), policy_events_.end());
        policy_events_.clear();
        std::stable_sort(events.begin(), events.end(), [](const DomainEvent& a, const DomainEvent& b) {
            return a.tick < b.tick;
        });
        return events;
    }

    // World geometry registration remains engine independent. Unreal's test
    // environment registers matching presentation bodies through the adapter;
    // the runtime owns whether the probe can actually pass through them.
    void add_static_sphere_body(StaticSphereBody body) {
        if (body.body_id.empty()) {
            throw std::invalid_argument("physical body id must not be empty");
        }
        if (!(body.radius_m > 0.0) || !std::isfinite(body.radius_m)) {
            throw std::invalid_argument("physical body radius must be finite and positive");
        }
        if (!finite_vector(body.center_m)) {
            throw std::invalid_argument("physical body center must be finite");
        }
        const auto duplicate = std::find_if(
            static_bodies_.begin(), static_bodies_.end(),
            [&body](const StaticSphereBody& existing) {
                return existing.body_id == body.body_id;
            });
        if (duplicate != static_bodies_.end()) {
            throw std::invalid_argument("physical body id already registered: " + body.body_id);
        }
        static_bodies_.push_back(std::move(body));
    }

    void clear_static_bodies() noexcept { static_bodies_.clear(); }

    [[nodiscard]] const std::vector<StaticSphereBody>& static_bodies() const noexcept {
        return static_bodies_;
    }

    void set_velocity_mps(Vector3d velocity) { core_.set_velocity_mps(velocity); }
    void adjust_attitude_degrees(EulerAttitudeDegrees delta) {
        core_.adjust_attitude_degrees(delta);
    }
    void adjust_local_velocity_mps(Vector3d local_delta_velocity) {
        core_.adjust_local_velocity_mps(local_delta_velocity);
    }

    void start_scan(const std::string& target_id, double duration_s) {
        const auto& state = core_.snapshot();
        if (state.power_allocated_sensors_w < kGeneration1MinimumSensorPowerW) {
            throw std::runtime_error(
                "sensors below minimum operating power: need >= " +
                whole_watts(kGeneration1MinimumSensorPowerW) + " W");
        }
        core_.start_scan(target_id, duration_s);
    }

    void cancel_scan() { core_.cancel_scan(); }

    void allocate_power(PowerSubsystem subsystem, double watts) {
        core_.allocate_power(subsystem, watts);
        if (subsystem == PowerSubsystem::Sensors &&
            watts < kGeneration1MinimumSensorPowerW &&
            core_.snapshot().is_scanning) {
            core_.cancel_scan();
        }
    }

    void set_subsystem_operational(PowerSubsystem subsystem, bool operational) {
        core_.set_subsystem_operational(subsystem, operational);
    }
    void set_energy_generation_w(double watts) { core_.set_energy_generation_w(watts); }
    void set_passive_cooling_w_per_k(double watts_per_k) { core_.set_passive_cooling_w_per_k(watts_per_k); }
    void set_max_operating_temperature_k(double kelvin) { core_.set_max_operating_temperature_k(kelvin); }
    [[nodiscard]] double total_power_allocated_w() const noexcept { return core_.total_power_allocated_w(); }
    void add_stored_material_kg(double kilograms) { core_.add_stored_material_kg(kilograms); }

    void install_policy(SoftwarePolicy policy) {
        validate_policy(policy);
        active_policy_ = std::move(policy);
        has_policy_ = true;
        policy_events_.push_back({
            tick(),
            DomainEventType::PolicyChanged,
            "policy installed: " + active_policy_.id
        });
    }

    void clear_policy() {
        if (!has_policy_) {
            return;
        }
        const std::string previous_id = active_policy_.id;
        active_policy_ = SoftwarePolicy{};
        has_policy_ = false;
        policy_events_.push_back({tick(), DomainEventType::PolicyChanged, "policy cleared: " + previous_id});
    }

    [[nodiscard]] SoftwarePolicyStatus policy_status() const noexcept {
        SoftwarePolicyStatus status;
        status.installed = has_policy_;
        status.enabled = has_policy_ && active_policy_.enabled;
        status.executor_available = policy_executor_available();
        status.policy_id = has_policy_ ? active_policy_.id : std::string{};
        status.rule_count = has_policy_ ? active_policy_.rules.size() : 0;
        status.minimum_computation_power_w = kGeneration1MinimumPolicyComputationPowerW;
        return status;
    }

    [[nodiscard]] const SoftwarePolicy* active_policy() const noexcept {
        return has_policy_ ? &active_policy_ : nullptr;
    }

    // Selects the nearest registered body within max_selection_range_m, or
    // clears the selection when none is registered or in range. Mirrors
    // find_nearest_selectable_target's own no-guessing contract.
    void select_nearest_target(double max_selection_range_m) {
        const auto nearest = find_nearest_selectable_target(
            core_.snapshot().position_m, static_bodies_, max_selection_range_m);
        selected_target_id_ = nearest.has_value() ? nearest->body_id : std::string{};
    }

    // Explicitly selects a target by id. An empty or currently-unregistered
    // id clears the selection rather than keeping a stale one (fail-closed,
    // same contract as select_target_telemetry).
    void select_target(const std::string& body_id) {
        const auto telemetry = select_target_telemetry(
            body_id, static_bodies_, core_.snapshot().position_m, core_.snapshot().velocity_mps);
        selected_target_id_ = telemetry.has_value() ? telemetry->body_id : std::string{};
    }

    void clear_target_selection() noexcept { selected_target_id_.clear(); }

    // Recomputed from the live registry/pose on every call rather than
    // cached, so range/closing speed always reflect the current tick and a
    // since-deregistered selection reports has_selection=false without
    // requiring a mutating call to notice.
    [[nodiscard]] TargetSelectionStatus selected_target_status() const noexcept {
        const auto& state = core_.snapshot();
        const auto telemetry = select_target_telemetry(
            selected_target_id_, static_bodies_, state.position_m, state.velocity_mps);
        TargetSelectionStatus status;
        if (telemetry.has_value()) {
            status.has_selection = true;
            status.body_id = telemetry->body_id;
            status.surface_range_m = telemetry->surface_range_m;
            status.closing_speed_mps = telemetry->closing_speed_mps;
        }
        return status;
    }

private:
    // The probe is represented by the authoritative compound envelope
    // (ProbeCompoundCollisionEnvelope): several local-space sphere samples
    // rather than one oversized bounding sphere. A candidate therefore
    // carries the contacted body alongside the winning sample's geometry.
    struct ContactCandidate {
        const StaticSphereBody* body{nullptr};
        CompoundContactCandidate sample_hit{};
    };

    [[nodiscard]] static bool finite_vector(Vector3d value) noexcept {
        return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
    }

    [[nodiscard]] static double dot(Vector3d a, Vector3d b) noexcept {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    // Sweeps every local-space sample in the authoritative compound envelope
    // (rotated by the probe's current attitude) against one body and keeps
    // the earliest genuine hit. This is the direct replacement for the
    // retired single-sphere sweep: a long probe with wing samples can now be
    // touched by its nose or a wing tip instead of an oversized center
    // sphere reporting contact in visibly empty space around the hull.
    [[nodiscard]] bool sweep_probe_against_body(
        Vector3d start,
        Vector3d end,
        EulerAttitudeDegrees attitude,
        const StaticSphereBody& body,
        ContactCandidate& candidate) const noexcept {
        const CompoundContactCandidate hit = sweep_compound_probe_against_body(
            start, end, attitude, core_.snapshot().compound_collision_envelope, body);
        if (!hit.hit) {
            return false;
        }

        const double inward_normal_speed =
            std::max(0.0, -dot(core_.snapshot().velocity_mps, hit.normal));

        // If we begin touching/overlapping but are already moving away, do not
        // manufacture another contact event or pin the probe to the surface.
        if (hit.fraction == 0.0 && inward_normal_speed <= 1e-9) {
            return false;
        }

        candidate.body = &body;
        candidate.sample_hit = hit;
        return true;
    }

    void resolve_static_contacts(Vector3d start_position) {
        if (static_bodies_.empty()) {
            return;
        }

        const auto& state = core_.snapshot();
        const Vector3d integrated_end = state.position_m;
        const EulerAttitudeDegrees attitude = state.attitude_degrees;

        ContactCandidate earliest;
        earliest.sample_hit.fraction = std::numeric_limits<double>::infinity();

        for (const StaticSphereBody& body : static_bodies_) {
            ContactCandidate candidate;
            if (sweep_probe_against_body(start_position, integrated_end, attitude, body, candidate) &&
                candidate.sample_hit.fraction < earliest.sample_hit.fraction) {
                earliest = candidate;
            }
        }

        if (earliest.body == nullptr) {
            return;
        }

        const Vector3d incoming_velocity = state.velocity_mps;
        const ProbeCollisionSphereSample& sample =
            state.compound_collision_envelope.samples[earliest.sample_hit.sample_index];
        const CompoundContactResolution resolution = resolve_compound_contact(
            earliest.sample_hit, attitude, sample, *earliest.body, incoming_velocity);

        core_.resolve_contact(
            earliest.body->body_id,
            resolution.resolved_probe_root,
            resolution.surface_point,
            earliest.sample_hit.normal,
            incoming_velocity,
            resolution.normal_speed_mps,
            resolution.resolved_velocity);
    }

    [[nodiscard]] bool policy_executor_available() const noexcept {
        const auto& state = core_.snapshot();
        return state.computation_operational &&
               state.power_allocated_computation_w >= kGeneration1MinimumPolicyComputationPowerW;
    }

    static void validate_policy(const SoftwarePolicy& policy) {
        if (policy.id.empty()) {
            throw std::invalid_argument("policy id must not be empty");
        }
        if (policy.rules.empty()) {
            throw std::invalid_argument("policy must contain at least one rule");
        }
        if (policy.rules.size() > kGeneration1MaxPolicyRules) {
            throw std::invalid_argument("Generation-1 policy exceeds two-rule computation limit");
        }

        for (const auto& rule : policy.rules) {
            if (rule.id.empty()) {
                throw std::invalid_argument("policy rule id must not be empty");
            }
            switch (rule.condition) {
                case PolicyConditionKind::EnergyFractionBelow:
                case PolicyConditionKind::EnergyFractionAbove:
                    if (rule.threshold < 0.0 || rule.threshold > 1.0) {
                        throw std::invalid_argument("energy-fraction policy threshold must be within [0, 1]");
                    }
                    break;
                case PolicyConditionKind::TemperatureAboveKelvin:
                case PolicyConditionKind::TemperatureBelowKelvin:
                    if (!(rule.threshold > 0.0)) {
                        throw std::invalid_argument("temperature policy threshold must be positive kelvin");
                    }
                    break;
            }
            if (rule.action_watts < 0.0) {
                throw std::invalid_argument("policy power action must be non-negative");
            }
        }
    }

    [[nodiscard]] static bool condition_matches(const SoftwarePolicyRule& rule, const ProbeStateSnapshot& state) noexcept {
        switch (rule.condition) {
            case PolicyConditionKind::EnergyFractionBelow: {
                const double fraction = state.energy_capacity_j > 0.0
                    ? state.stored_energy_j / state.energy_capacity_j
                    : 0.0;
                return fraction < rule.threshold;
            }
            case PolicyConditionKind::EnergyFractionAbove: {
                const double fraction = state.energy_capacity_j > 0.0
                    ? state.stored_energy_j / state.energy_capacity_j
                    : 0.0;
                return fraction > rule.threshold;
            }
            case PolicyConditionKind::TemperatureAboveKelvin:
                return state.temperature_k > rule.threshold;
            case PolicyConditionKind::TemperatureBelowKelvin:
                return state.temperature_k < rule.threshold;
        }
        return false;
    }

    [[nodiscard]] static double current_allocation(PowerSubsystem subsystem, const ProbeStateSnapshot& state) noexcept {
        switch (subsystem) {
            case PowerSubsystem::Sensors:
                return state.power_allocated_sensors_w;
            case PowerSubsystem::Propulsion:
                return state.power_allocated_propulsion_w;
            case PowerSubsystem::Computation:
                return state.power_allocated_computation_w;
            case PowerSubsystem::Thermal:
                return state.power_allocated_thermal_w;
        }
        return 0.0;
    }

    [[nodiscard]] static std::string whole_watts(double watts) {
        return std::to_string(static_cast<long long>(std::llround(watts)));
    }

    [[nodiscard]] static std::string subsystem_label(PowerSubsystem subsystem) {
        switch (subsystem) {
            case PowerSubsystem::Sensors:
                return "Sensors";
            case PowerSubsystem::Propulsion:
                return "Propulsion";
            case PowerSubsystem::Computation:
                return "Computation";
            case PowerSubsystem::Thermal:
                return "Thermal";
        }
        return "Unknown";
    }

    [[nodiscard]] static std::string condition_summary(const SoftwarePolicyRule& rule) {
        switch (rule.condition) {
            case PolicyConditionKind::EnergyFractionBelow:
                return "energy reserve below " +
                       std::to_string(static_cast<long long>(std::llround(rule.threshold * 100.0))) + "%";
            case PolicyConditionKind::EnergyFractionAbove:
                return "energy reserve above " +
                       std::to_string(static_cast<long long>(std::llround(rule.threshold * 100.0))) + "%";
            case PolicyConditionKind::TemperatureAboveKelvin:
                return "temperature above " + whole_watts(rule.threshold) + " K";
            case PolicyConditionKind::TemperatureBelowKelvin:
                return "temperature below " + whole_watts(rule.threshold) + " K";
        }
        return "condition matched";
    }

    void evaluate_policy() {
        if (!has_policy_ || !active_policy_.enabled || !policy_executor_available()) {
            return;
        }

        for (const auto& rule : active_policy_.rules) {
            const auto state_before_action = core_.snapshot();
            if (!condition_matches(rule, state_before_action)) {
                continue;
            }

            const double allocation_before = current_allocation(rule.subsystem, state_before_action);
            if (std::fabs(allocation_before - rule.action_watts) < 1e-9) {
                continue;
            }

            try {
                allocate_power(rule.subsystem, rule.action_watts);
                policy_events_.push_back({
                    tick(),
                    DomainEventType::PolicyRuleTriggered,
                    "AUTOMATION: " + subsystem_label(rule.subsystem) + " " +
                        whole_watts(allocation_before) + " W -> " +
                        whole_watts(rule.action_watts) + " W // " +
                        condition_summary(rule)
                });
            } catch (const std::exception& error) {
                policy_events_.push_back({
                    tick(),
                    DomainEventType::PolicyActionRejected,
                    "AUTOMATION REJECTED: " + subsystem_label(rule.subsystem) + " " +
                        whole_watts(allocation_before) + " W -> " +
                        whole_watts(rule.action_watts) + " W // " +
                        condition_summary(rule) + " // " + error.what()
                });
            }

            if (!policy_executor_available()) {
                break;
            }
        }
    }

    SimulationCore core_{};
    std::vector<StaticSphereBody> static_bodies_{};
    bool has_policy_{false};
    SoftwarePolicy active_policy_{};
    std::vector<DomainEvent> policy_events_{};
    std::string selected_target_id_{};
};

} // namespace everward::simulation
