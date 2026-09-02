#pragma once

#include <cmath>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

namespace everward::simulation {

// Slice 6 — articulated manipulator arms. This module is engine-independent
// and deterministic, matching the rest of src/simulation/: Unreal presents
// whatever authoritative state this produces but never authors it. It is
// deliberately standalone (no dependency on SimulationCore/ProbeRuntime)
// because manipulator articulation does not yet need to read or mutate
// probe motion, power, or damage truth; a later slice may compose this with
// DamageAwareProbeRuntime the same way impact_damage.hpp composes
// ProbeRuntime, once manipulator power draw and integrity-based
// effectiveness are in scope.
//
// The Prime Generation-1 body carries two shoulder mounts (see
// EverwardProbePawn's PortShoulder/StarboardShoulder), so this foundation
// models exactly two arms rather than an arbitrary N-arm rig.
enum class ManipulatorArmId {
    Port,
    Starboard
};

enum class ManipulatorJoint {
    Shoulder,
    Elbow,
    Wrist
};

enum class ManipulatorEventType {
    ArmDeployStarted,
    ArmDeployCompleted,
    ArmStowStarted,
    ArmStowCompleted,
    ToolAttached,
    ToolDetached,
    TargetGrasped,
    TargetReleased
};

struct ManipulatorEvent {
    ManipulatorArmId arm{ManipulatorArmId::Port};
    ManipulatorEventType type{ManipulatorEventType::ArmDeployStarted};
    std::string detail;
};

struct ManipulatorJointRangeDegrees {
    double min_degrees{};
    double max_degrees{};
};

struct ManipulatorArmAngles {
    double shoulder_degrees{0.0};
    double elbow_degrees{0.0};
    double wrist_degrees{0.0};
};

struct ManipulatorArmState {
    // Stowed is the resting/at-rest state a new probe (and a freshly stowed
    // arm) starts in. Deployment/stow are not instantaneous: fraction moves
    // toward 1.0 (deployed) or 0.0 (stowed) over ManipulatorRig's fixed rate
    // so telemetry and later animation stay meaningful across ticks rather
    // than snapping.
    bool is_deployed{false};
    bool is_deploying{false};
    bool is_stowing{false};
    double deployment_fraction{0.0};

    // Authoritative current joint pose and the commanded target it is
    // slewing toward. Joint commands are only accepted once the arm is
    // fully deployed (see ManipulatorRig::command_joint_target_degrees).
    ManipulatorArmAngles angles{};
    ManipulatorArmAngles commanded_angles{};

    bool tool_attached{false};

    // Slice 7 "grasp or dock with a simple object" minimum interaction.
    // Empty means not grasping; the gating decision of whether a grasp
    // attempt is close enough to succeed lives outside this module (see
    // manipulator_grasp.hpp), the same separation manipulator_reach.hpp
    // already established for reach telemetry. This module only enforces
    // the mechanical invariants a real gripper would: deployed-only, one
    // object at a time, and never stowing away with something still held.
    std::string grasped_target_body_id;
};

// Deterministic, engine-independent constrained-joint manipulator rig for
// exactly two arms (Port/Starboard). All motion is rate-limited and
// range-clamped: there is no unconstrained rag-doll or unlimited rotation,
// matching the canonical "shoulder -> upper arm -> elbow -> forearm -> wrist
// -> tool interface" chain in PHASE2_VERTICAL_SLICE_PLAN.md Slice 6.
class ManipulatorRig {
public:
    // First-pass Generation-1 mechanical calibration. A later hardware
    // evolution slice may make these per-descendant rather than fixed
    // constants, the same way component integrity later became a bridge
    // into staged Self Repair without replacing the underlying model.
    static constexpr double kDeployStowDurationS = 2.0;
    static constexpr double kJointSlewDegreesPerSecond = 45.0;

    // Returns true if the given fully-deployed pose is safe to occupy.
    // Injected rather than computed here so this module stays standalone
    // (see the header comment above): manipulator_hull_contact.hpp supplies
    // the real hull-aware implementation, wired in where the rig is actually
    // constructed. A default-constructed rig has no guard and behaves
    // exactly as before this pose-safety check existed.
    using SelfCollisionGuard =
        std::function<bool(ManipulatorArmId, double /*deployment_fraction*/, ManipulatorArmAngles)>;

    ManipulatorRig() = default;
    explicit ManipulatorRig(SelfCollisionGuard self_collision_guard)
        : self_collision_guard_(std::move(self_collision_guard)) {}

    [[nodiscard]] static constexpr ManipulatorJointRangeDegrees shoulder_range() noexcept {
        return {-90.0, 90.0};
    }
    [[nodiscard]] static constexpr ManipulatorJointRangeDegrees elbow_range() noexcept {
        return {0.0, 150.0};
    }
    [[nodiscard]] static constexpr ManipulatorJointRangeDegrees wrist_range() noexcept {
        return {-180.0, 180.0};
    }

    [[nodiscard]] const ManipulatorArmState& arm(ManipulatorArmId id) const noexcept {
        return state_ref(id);
    }

    [[nodiscard]] bool is_deployed(ManipulatorArmId id) const noexcept {
        return state_ref(id).is_deployed;
    }

    // Begins deploying the arm. Safe to call while the arm is mid-stow: it
    // reverses direction cleanly toward fully deployed. Throws if the arm is
    // already fully deployed (and not mid-stow), since that command carries
    // no meaningful effect.
    void begin_deploy(ManipulatorArmId id) {
        ManipulatorArmState& state = state_ref(id);
        if (state.deployment_fraction >= 1.0 && !state.is_stowing) {
            throw std::runtime_error("manipulator arm already deployed");
        }
        const bool was_deploying = state.is_deploying;
        state.is_deploying = true;
        state.is_stowing = false;
        if (!was_deploying) {
            events_.push_back({id, ManipulatorEventType::ArmDeployStarted, "arm deploy started"});
        }
    }

    // Begins stowing the arm. A tool must be detached first: retracting an
    // arm with an attached tool against the probe body is not a state this
    // model allows. Safe to call mid-deploy to reverse direction. Throws if
    // the arm is already fully stowed (and not mid-deploy).
    void begin_stow(ManipulatorArmId id) {
        ManipulatorArmState& state = state_ref(id);
        if (state.tool_attached) {
            throw std::runtime_error("detach tool before stowing manipulator arm");
        }
        if (!state.grasped_target_body_id.empty()) {
            throw std::runtime_error("release grasped target before stowing manipulator arm");
        }
        if (state.deployment_fraction <= 0.0 && !state.is_deploying) {
            throw std::runtime_error("manipulator arm already stowed");
        }
        const bool was_stowing = state.is_stowing;
        state.is_stowing = true;
        state.is_deploying = false;
        // Retract joints toward the stowed pose while the arm folds away so
        // it never ends up stowed with an outstretched joint.
        state.commanded_angles = ManipulatorArmAngles{};
        if (!was_stowing) {
            events_.push_back({id, ManipulatorEventType::ArmStowStarted, "arm stow started"});
        }
    }

    // Commands a joint toward an absolute target angle. Out-of-range targets
    // are clamped rather than rejected: real hardware simply stops at its
    // physical travel limit instead of refusing the whole command. Motion
    // toward the (possibly clamped) target is rate-limited in advance().
    // Throws if the arm is not fully deployed.
    void command_joint_target_degrees(ManipulatorArmId id, ManipulatorJoint joint, double degrees) {
        if (!std::isfinite(degrees)) {
            throw std::invalid_argument("joint target degrees must be finite");
        }
        ManipulatorArmState& state = state_ref(id);
        if (!state.is_deployed || state.is_stowing) {
            throw std::runtime_error("manipulator arm must be fully deployed to command a joint");
        }

        switch (joint) {
            case ManipulatorJoint::Shoulder:
                state.commanded_angles.shoulder_degrees = clamp_to_range(degrees, shoulder_range());
                break;
            case ManipulatorJoint::Elbow:
                state.commanded_angles.elbow_degrees = clamp_to_range(degrees, elbow_range());
                break;
            case ManipulatorJoint::Wrist:
                state.commanded_angles.wrist_degrees = clamp_to_range(degrees, wrist_range());
                break;
        }
    }

    // Attaches the tool interface. Requires the arm to be fully deployed;
    // throws if a tool is already attached.
    void attach_tool(ManipulatorArmId id) {
        ManipulatorArmState& state = state_ref(id);
        if (!state.is_deployed || state.is_stowing) {
            throw std::runtime_error("manipulator arm must be fully deployed to attach a tool");
        }
        if (state.tool_attached) {
            throw std::runtime_error("tool already attached");
        }
        state.tool_attached = true;
        events_.push_back({id, ManipulatorEventType::ToolAttached, "tool attached"});
    }

    void detach_tool(ManipulatorArmId id) {
        ManipulatorArmState& state = state_ref(id);
        if (!state.tool_attached) {
            throw std::runtime_error("no tool attached");
        }
        state.tool_attached = false;
        events_.push_back({id, ManipulatorEventType::ToolDetached, "tool detached"});
    }

    // Records that the arm now holds target_body_id. This is the mechanical
    // half only: whether the arm is actually close enough to grasp is a
    // proximity decision the caller must already have made (see
    // manipulator_grasp.hpp's attempt_grasp_selected_target, which is the
    // only intended caller in practice). Requires a fully deployed, not
    // mid-stow arm -- the same steady-state regime command_joint_target_degrees
    // and attach_tool already require -- and throws if the arm is already
    // grasping something or target_body_id is empty.
    void begin_grasp(ManipulatorArmId id, const std::string& target_body_id) {
        if (target_body_id.empty()) {
            throw std::invalid_argument("grasped target body id must not be empty");
        }
        ManipulatorArmState& state = state_ref(id);
        if (!state.is_deployed || state.is_stowing) {
            throw std::runtime_error("manipulator arm must be fully deployed to grasp a target");
        }
        if (!state.grasped_target_body_id.empty()) {
            throw std::runtime_error("manipulator arm already grasping a target");
        }
        state.grasped_target_body_id = target_body_id;
        events_.push_back({id, ManipulatorEventType::TargetGrasped, "target grasped: " + target_body_id});
    }

    // Releasing is always allowed while grasping regardless of current
    // reach/range -- an operator letting go does not require re-proving
    // proximity, matching detach_tool's unconditional release.
    void release_grasp(ManipulatorArmId id) {
        ManipulatorArmState& state = state_ref(id);
        if (state.grasped_target_body_id.empty()) {
            throw std::runtime_error("no target grasped");
        }
        const std::string released = state.grasped_target_body_id;
        state.grasped_target_body_id.clear();
        events_.push_back({id, ManipulatorEventType::TargetReleased, "target released: " + released});
    }

    // Deterministic fixed-step-friendly integration. Safe to call with the
    // same per-tick seconds value SimulationCore uses so manipulator motion
    // stays reproducible for the same recorded input, per
    // SIMULATION_PHILOSOPHY.md's determinism rule.
    void advance(double seconds) {
        if (!std::isfinite(seconds) || seconds < 0.0) {
            throw std::invalid_argument("seconds must be finite and non-negative");
        }
        advance_arm(ManipulatorArmId::Port, seconds);
        advance_arm(ManipulatorArmId::Starboard, seconds);
    }

    [[nodiscard]] std::vector<ManipulatorEvent> drain_events() {
        auto out = std::move(events_);
        events_.clear();
        return out;
    }

private:
    [[nodiscard]] static double clamp_to_range(double degrees, ManipulatorJointRangeDegrees range) noexcept {
        if (degrees < range.min_degrees) return range.min_degrees;
        if (degrees > range.max_degrees) return range.max_degrees;
        return degrees;
    }

    [[nodiscard]] static double slew_toward(double current, double target, double max_step) noexcept {
        const double delta = target - current;
        if (delta > max_step) return current + max_step;
        if (delta < -max_step) return current - max_step;
        return target;
    }

    void advance_arm(ManipulatorArmId id, double seconds) {
        ManipulatorArmState& state = state_ref(id);

        if (state.is_deploying && seconds > 0.0) {
            state.deployment_fraction += seconds / kDeployStowDurationS;
            if (state.deployment_fraction >= 1.0) {
                state.deployment_fraction = 1.0;
                state.is_deploying = false;
                state.is_deployed = true;
                events_.push_back({id, ManipulatorEventType::ArmDeployCompleted, "arm deploy completed"});
            }
        } else if (state.is_stowing && seconds > 0.0) {
            state.deployment_fraction -= seconds / kDeployStowDurationS;
            if (state.deployment_fraction <= 0.0) {
                state.deployment_fraction = 0.0;
                state.is_stowing = false;
                state.is_deployed = false;
                state.angles = ManipulatorArmAngles{};
                state.commanded_angles = ManipulatorArmAngles{};
                events_.push_back({id, ManipulatorEventType::ArmStowCompleted, "arm stow completed"});
            }
        }

        if (seconds <= 0.0) {
            return;
        }
        const double max_step = kJointSlewDegreesPerSecond * seconds;
        ManipulatorArmAngles candidate = state.angles;
        candidate.shoulder_degrees =
            slew_toward(state.angles.shoulder_degrees, state.commanded_angles.shoulder_degrees, max_step);
        candidate.elbow_degrees =
            slew_toward(state.angles.elbow_degrees, state.commanded_angles.elbow_degrees, max_step);
        candidate.wrist_degrees =
            slew_toward(state.angles.wrist_degrees, state.commanded_angles.wrist_degrees, max_step);

        // Only guard the steady-state "fully deployed, following an operator
        // command" regime that command_joint_target_degrees itself requires.
        // Deploy/stow's own fold motion (see the fraction block above) is
        // left unguarded: it already ends at the same zero-angle pose every
        // time, and guarding it too would risk deadlocking deploy/stow
        // against this coarse hull approximation instead of only refusing
        // genuinely new operator-commanded penetration.
        const bool steady_state = state.is_deployed && !state.is_stowing && !state.is_deploying;
        if (steady_state && self_collision_guard_ &&
            !self_collision_guard_(id, state.deployment_fraction, candidate)) {
            return;
        }
        state.angles = candidate;
    }

    [[nodiscard]] ManipulatorArmState& state_ref(ManipulatorArmId id) noexcept {
        return id == ManipulatorArmId::Port ? port_ : starboard_;
    }
    [[nodiscard]] const ManipulatorArmState& state_ref(ManipulatorArmId id) const noexcept {
        return id == ManipulatorArmId::Port ? port_ : starboard_;
    }

    ManipulatorArmState port_{};
    ManipulatorArmState starboard_{};
    std::vector<ManipulatorEvent> events_{};
    SelfCollisionGuard self_collision_guard_{};
};

} // namespace everward::simulation
