#pragma once

// Small, pure decisions behind three first-loop controls that the Aura run 20261108-084846 found broken
// (Everward issue #372). Kept here, beside the simulation they describe, so the deterministic test suite can prove them
// against the real runtime and rig; the Unreal controller and HUD call these instead of re-deriving them.

#include "everward/simulation/manipulator.hpp"
#include "everward/simulation/types.hpp"

#include <algorithm>
#include <string>
#include <string_view>

namespace everward::simulation {

// P1-1: a survey is earned by a genuine ScanCompleted domain event for exactly that target. A later cancelled scan, a
// scan of another target, or any other event never takes it away (the caller latches true; nothing here clears it).
[[nodiscard]] inline bool scan_event_completes_survey(const DomainEvent& event, std::string_view target_id) {
    if (event.type != DomainEventType::ScanCompleted || target_id.empty()) return false;
    const std::string suffix = ": " + std::string(target_id);
    const std::string& detail = event.detail;
    return detail.size() >= suffix.size() && detail.compare(detail.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// P1-2: one key toggles an arm. ManipulatorRig keeps is_deployed true until a stow COMPLETES (by design: joint commands
// are then refused while stowing), so "deployed" alone must not decide the direction. A press during a stow reverses
// it to a deploy; a press during a deploy reverses it to a stow; otherwise the press inverts the resting state.
enum class ArmToggleCommand { Deploy, Stow };

[[nodiscard]] constexpr ArmToggleCommand arm_toggle_command(bool is_deployed, bool is_deploying, bool is_stowing) noexcept {
    if (is_stowing) return ArmToggleCommand::Deploy;
    if (is_deploying) return ArmToggleCommand::Stow;
    return is_deployed ? ArmToggleCommand::Stow : ArmToggleCommand::Deploy;
}

[[nodiscard]] constexpr ArmToggleCommand arm_toggle_command(const ManipulatorArmState& arm) noexcept {
    return arm_toggle_command(arm.is_deployed, arm.is_deploying, arm.is_stowing);
}

// F6 (Aura report): the HUD may offer joint control only when the rig would accept a joint command -- fully deployed
// and not moving -- exactly the condition ManipulatorRig::command_joint_target_degrees enforces.
[[nodiscard]] constexpr bool arm_accepts_joint_commands(bool is_deployed, bool is_deploying, bool is_stowing) noexcept {
    return is_deployed && !is_deploying && !is_stowing;
}

// P2-1: a selection cursor over `count` entries moves by `delta` and always stays inside [0, count - 1] (0 when
// empty), whether or not the panel showing it is open.
[[nodiscard]] constexpr int bounded_selection(int index, int delta, int count) noexcept {
    if (count <= 0) return 0;
    const long long next = static_cast<long long>(index) + delta;
    return static_cast<int>(std::clamp<long long>(next, 0, count - 1));
}

} // namespace everward::simulation
