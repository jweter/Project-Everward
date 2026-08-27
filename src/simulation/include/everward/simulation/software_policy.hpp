#pragma once

#include "everward/simulation/core.hpp"

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

private:
    struct ContactCandidate {
        const StaticSphereBody* body{nullptr};
        double fraction{1.0};
        Vector3d probe_center_at_contact{};
        Vector3d normal{};
    };

    [[nodiscard]] static bool finite_vector(Vector3d value) noexcept {
        return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
    }

    [[nodiscard]] static Vector3d add(Vector3d a, Vector3d b) noexcept {
        return {a.x + b.x, a.y + b.y, a.z + b.z};
    }

    [[nodiscard]] static Vector3d subtract(Vector3d a, Vector3d b) noexcept {
        return {a.x - b.x, a.y - b.y, a.z - b.z};
    }

    [[nodiscard]] static Vector3d scale(Vector3d value, double scalar) noexcept {
        return {value.x * scalar, value.y * scalar, value.z * scalar};
    }

    [[nodiscard]] static double dot(Vector3d a, Vector3d b) noexcept {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    [[nodiscard]] static double length(Vector3d value) noexcept {
        return std::sqrt(dot(value, value));
    }

    [[nodiscard]] static Vector3d normalized_or_x(Vector3d value) noexcept {
        const double magnitude = length(value);
        if (magnitude <= 1e-12) {
            return {1.0, 0.0, 0.0};
        }
        return scale(value, 1.0 / magnitude);
    }

    [[nodiscard]] bool sweep_probe_against_body(
        Vector3d start,
        Vector3d end,
        const StaticSphereBody& body,
        ContactCandidate& candidate) const noexcept {
        const Vector3d delta = subtract(end, start);
        const Vector3d from_center = subtract(start, body.center_m);
        const double combined_radius = body.radius_m + core_.snapshot().collision_envelope_radius_m;
        const double a = dot(delta, delta);
        const double c = dot(from_center, from_center) - combined_radius * combined_radius;

        double fraction = std::numeric_limits<double>::infinity();
        if (c <= 0.0) {
            fraction = 0.0;
        } else if (a > 1e-18) {
            const double b = 2.0 * dot(from_center, delta);
            const double discriminant = b * b - 4.0 * a * c;
            if (discriminant >= 0.0) {
                const double root = (-b - std::sqrt(discriminant)) / (2.0 * a);
                if (root >= 0.0 && root <= 1.0) {
                    fraction = root;
                }
            }
        }

        if (!std::isfinite(fraction)) {
            return false;
        }

        const Vector3d center_at_contact = add(start, scale(delta, fraction));
        const Vector3d normal = normalized_or_x(subtract(center_at_contact, body.center_m));
        const double inward_normal_speed = std::max(0.0, -dot(core_.snapshot().velocity_mps, normal));

        // If we begin touching/overlapping but are already moving away, do not
        // manufacture another contact event or pin the probe to the surface.
        if (fraction == 0.0 && inward_normal_speed <= 1e-9) {
            return false;
        }

        candidate.body = &body;
        candidate.fraction = fraction;
        candidate.probe_center_at_contact = center_at_contact;
        candidate.normal = normal;
        return true;
    }

    void resolve_static_contacts(Vector3d start_position) {
        if (static_bodies_.empty()) {
            return;
        }

        const Vector3d integrated_end = core_.snapshot().position_m;
        ContactCandidate earliest;
        earliest.fraction = std::numeric_limits<double>::infinity();

        for (const StaticSphereBody& body : static_bodies_) {
            ContactCandidate candidate;
            if (sweep_probe_against_body(start_position, integrated_end, body, candidate) &&
                candidate.fraction < earliest.fraction) {
                earliest = candidate;
            }
        }

        if (earliest.body == nullptr) {
            return;
        }

        const Vector3d incoming_velocity = core_.snapshot().velocity_mps;
        const double normal_component = dot(incoming_velocity, earliest.normal);
        const double normal_speed = std::max(0.0, -normal_component);
        Vector3d resolved_velocity = incoming_velocity;
        if (normal_component < 0.0) {
            resolved_velocity = subtract(incoming_velocity, scale(earliest.normal, normal_component));
        }

        const double combined_radius =
            earliest.body->radius_m + core_.snapshot().collision_envelope_radius_m;
        const Vector3d resolved_center = add(
            earliest.body->center_m,
            scale(earliest.normal, combined_radius + 1e-6));
        const Vector3d surface_point = add(
            earliest.body->center_m,
            scale(earliest.normal, earliest.body->radius_m));

        core_.resolve_contact(
            earliest.body->body_id,
            resolved_center,
            surface_point,
            earliest.normal,
            incoming_velocity,
            normal_speed,
            resolved_velocity);
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
};

} // namespace everward::simulation
