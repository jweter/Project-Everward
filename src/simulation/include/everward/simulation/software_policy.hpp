#pragma once

#include "everward/simulation/core.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
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

// Authoritative One-Probe runtime wrapper that couples the mechanical
// SimulationCore to the first primitive software-policy evaluator. The core
// remains the sole owner of physical state. Policies observe a snapshot and
// submit the exact same public SimulationCore commands used by manual control;
// they never mutate ProbeStateSnapshot directly.
class ProbeRuntime {
public:
    static constexpr std::size_t kGeneration1MaxPolicyRules = 2;
    static constexpr double kGeneration1MinimumPolicyComputationPowerW = 25.0;

    ProbeRuntime() = default;
    explicit ProbeRuntime(SimulationCore core) : core_(std::move(core)) {}

    [[nodiscard]] static ProbeRuntime make_canonical_ev0001() {
        return ProbeRuntime(SimulationCore::make_canonical_ev0001());
    }

    [[nodiscard]] const ProbeStateSnapshot& snapshot() const noexcept { return core_.snapshot(); }
    [[nodiscard]] std::int64_t tick() const noexcept { return core_.tick(); }

    void advance_wall_ticks(std::int64_t wall_ticks) {
        core_.advance_wall_ticks(wall_ticks);
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

    void set_velocity_mps(Vector3d velocity) { core_.set_velocity_mps(velocity); }
    void adjust_attitude_degrees(EulerAttitudeDegrees delta) {
        core_.adjust_attitude_degrees(delta);
    }
    void adjust_local_velocity_mps(Vector3d local_delta_velocity) {
        core_.adjust_local_velocity_mps(local_delta_velocity);
    }
    void start_scan(const std::string& target_id, double duration_s) { core_.start_scan(target_id, duration_s); }
    void cancel_scan() { core_.cancel_scan(); }
    void allocate_power(PowerSubsystem subsystem, double watts) { core_.allocate_power(subsystem, watts); }
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

    void evaluate_policy() {
        if (!has_policy_ || !active_policy_.enabled || !policy_executor_available()) {
            return;
        }

        // Generation 1 is intentionally primitive: rules are evaluated in
        // authored order after each fixed simulation advance. Each matching
        // rule performs one simple command. A policy can even command its own
        // computation allocation to zero, disabling itself until the player
        // manually restores enough compute power. This is deliberate early-
        // machine clunkiness rather than hidden convenience logic.
        for (const auto& rule : active_policy_.rules) {
            const auto state_before_action = core_.snapshot();
            if (!condition_matches(rule, state_before_action)) {
                continue;
            }

            if (std::fabs(current_allocation(rule.subsystem, state_before_action) - rule.action_watts) < 1e-9) {
                continue;
            }

            try {
                // Critical architecture rule: automation uses the same public
                // authoritative command as a manual power-allocation action.
                core_.allocate_power(rule.subsystem, rule.action_watts);
                policy_events_.push_back({
                    tick(),
                    DomainEventType::PolicyRuleTriggered,
                    "policy " + active_policy_.id + " rule " + rule.id + " executed"
                });
            } catch (const std::exception& error) {
                policy_events_.push_back({
                    tick(),
                    DomainEventType::PolicyActionRejected,
                    "policy " + active_policy_.id + " rule " + rule.id + " rejected: " + error.what()
                });
            }

            // If a rule disables computation, stop immediately. The remaining
            // rules cannot execute without the physical compute budget.
            if (!policy_executor_available()) {
                break;
            }
        }
    }

    SimulationCore core_{};
    bool has_policy_{false};
    SoftwarePolicy active_policy_{};
    std::vector<DomainEvent> policy_events_{};
};

} // namespace everward::simulation
