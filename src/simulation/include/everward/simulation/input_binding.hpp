#pragma once

// Engine-independent semantic input binding/remapping foundation for
// docs/PHASE2_INPUT_REMAP_ACCESSIBILITY_ARCHITECTURE.md (issue #165).
//
// This module implements migration-sequence steps 1 and 5 of that document:
// an audited catalog of every semantic action the live Unreal controllers
// currently expose (step 1), and a versioned, player-mappable binding
// preference model with deterministic conflict detection and fail-closed
// fallback (step 5). It deliberately does not touch any Unreal source: the
// architecture doc's own boundary rule is that simulation/presentation-adjacent
// logic must not depend on a keyboard key, controller button, or remapping
// UI, so this stays engine-independent exactly like fix_it.hpp/
// target_cycle_runtime.hpp/surface_descent_guidance.hpp did before their own
// later Unreal wiring passes. Steps 2-4/6-8 (Enhanced Input assets, a live
// remapping UI, controller defaults, and the Product Reality acceptance
// pass) remain later, separately reviewable slices per the doc's own "do
// not combine a broad control rewrite... into one PR" rule.
//
// The semantic action catalog and default keyboard bindings below are a
// direct audit of every `EKeys::`/`BindKey` call in `unreal/Source/Everward`
// as of this pass (`EverwardPlayerController.cpp`,
// `EverwardPlayerControllerInteractionTick.cpp`, `EverwardProbePawn.cpp`,
// `PlaytestRecorderActor.cpp`), not an invented control scheme -- this
// closes the exact gap PR #267's own review found: the architecture's first
// migration-inventory attempt missed the two production input consumers
// outside the main player controller (`EverwardProbePawn.cpp`'s `R` and
// `PlaytestRecorderActor.cpp`'s `F12`), both of which are included here.
// Context assignment (Gameplay/Manipulator/UI/Development) reflects the
// doc's own named categories; the live bindings today are not yet
// context-scoped in Unreal (every key is checked unconditionally each
// tick), so this catalog states where each action *should* live once a
// later Unreal-side slice adopts Enhanced Input mapping contexts.
//
// `SpaceBar` intentionally maps to exactly one semantic action
// (`StopPropulsion`), not several: in the live controller, pressing SpaceBar
// triggers a full manual stop and, depending on which copilot/auto-approach
// mode happens to be engaged, also cancels José/controlled-descent/
// controlled-hover/mining-auto-approach as coordinated side effects of that
// same "the player asked to stop" input. Modeling that as one semantic
// action deliberately matches the doc's own binding/dispatch contract
// ("device keys are presentation/input bindings, not simulation
// commands... rebinding must never change action semantics") -- rebinding
// SpaceBar to a different key must move every one of those coordinated
// cancels together, which a single semantic action naturally guarantees and
// separate per-mode actions bound to the same key would not. The same
// reasoning applies to the tractor field: engage-on-press and
// disengage-on-release of `B` are two edges of one continuous "hold to
// couple" interaction, so they are one semantic action
// (`ToggleTractorFieldCoupling`), not two independently bindable ones.

#include "everward/simulation/json_value.hpp"

#include <algorithm>
#include <array>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace everward::simulation {

// A remapping/binding-preferences schema version, deliberately independent
// of docs/SAVE_FORMAT.md's kSaveFormatVersion: the doc requires "a control
// layout migration must not become a save-schema migration."
inline constexpr int kBindingPreferencesSchemaVersion = 1;

enum class MappingContext {
    Gameplay,
    Manipulator,
    UI,
    Development
};

enum class SemanticAction {
    // Flight/translation/rotation.
    IncreaseForwardVelocity,
    DecreaseForwardVelocity,
    IncreaseLateralVelocity,
    DecreaseLateralVelocity,
    IncreaseVerticalVelocity,
    DecreaseVerticalVelocity,
    YawProbeLeft,
    YawProbeRight,
    PitchProbeUp,
    PitchProbeDown,
    RollProbeLeft,
    RollProbeRight,
    StopPropulsion,
    BeginOrCancelCameraAlignedRighting,
    // Assisted flight.
    ToggleJoseTakeTheWheel,
    ToggleControlledDescent,
    ToggleControlledHover,
    // Target selection and tractor interaction.
    SelectNearestPhysicalTarget,
    ToggleTractorFieldCoupling,
    // Mining/resource interaction.
    CycleMiningTarget,
    ToggleAutoApproachMiningTarget,
    CommandMineBootstrapTarget,
    // Player-facing development actions (save/load remain player-facing per
    // the architecture doc's own category list).
    SaveGame,
    LoadGame,
    // Manipulator deploy/stow, joint selection, joint motion, grasp/release.
    TogglePortManipulatorArm,
    ToggleStarboardManipulatorArm,
    ToggleManipulatorTool,
    ToggleSelectedManipulatorTool,
    ToggleManipulatorPanel,
    CycleManipulatorArmSelection,
    SelectManipulatorJointShoulder,
    SelectManipulatorJointElbow,
    SelectManipulatorJointWrist,
    DecreaseManipulatorJointTarget,
    IncreaseManipulatorJointTarget,
    ToggleManipulatorGrasp,
    CollectGraspedSample,
    // HUD/page/controls-reference navigation.
    ToggleControlsReference,
    ToggleSystemsPanel,
    SelectNextCapability,
    SelectPreviousCapability,
    ExecutePrimarySystemAction,
    ExecuteSecondarySystemAction,
    IncreaseSelectedSystemPower,
    DecreaseSelectedSystemPower,
    // Development-only.
    RecordPlaytestIssueMarker,
};

inline constexpr std::array<SemanticAction, 46> kAllSemanticActions{{
    SemanticAction::IncreaseForwardVelocity,
    SemanticAction::DecreaseForwardVelocity,
    SemanticAction::IncreaseLateralVelocity,
    SemanticAction::DecreaseLateralVelocity,
    SemanticAction::IncreaseVerticalVelocity,
    SemanticAction::DecreaseVerticalVelocity,
    SemanticAction::YawProbeLeft,
    SemanticAction::YawProbeRight,
    SemanticAction::PitchProbeUp,
    SemanticAction::PitchProbeDown,
    SemanticAction::RollProbeLeft,
    SemanticAction::RollProbeRight,
    SemanticAction::StopPropulsion,
    SemanticAction::BeginOrCancelCameraAlignedRighting,
    SemanticAction::ToggleJoseTakeTheWheel,
    SemanticAction::ToggleControlledDescent,
    SemanticAction::ToggleControlledHover,
    SemanticAction::SelectNearestPhysicalTarget,
    SemanticAction::ToggleTractorFieldCoupling,
    SemanticAction::CycleMiningTarget,
    SemanticAction::ToggleAutoApproachMiningTarget,
    SemanticAction::CommandMineBootstrapTarget,
    SemanticAction::SaveGame,
    SemanticAction::LoadGame,
    SemanticAction::TogglePortManipulatorArm,
    SemanticAction::ToggleStarboardManipulatorArm,
    SemanticAction::ToggleManipulatorTool,
    SemanticAction::ToggleSelectedManipulatorTool,
    SemanticAction::ToggleManipulatorPanel,
    SemanticAction::CycleManipulatorArmSelection,
    SemanticAction::SelectManipulatorJointShoulder,
    SemanticAction::SelectManipulatorJointElbow,
    SemanticAction::SelectManipulatorJointWrist,
    SemanticAction::DecreaseManipulatorJointTarget,
    SemanticAction::IncreaseManipulatorJointTarget,
    SemanticAction::ToggleManipulatorGrasp,
    SemanticAction::CollectGraspedSample,
    SemanticAction::ToggleControlsReference,
    SemanticAction::ToggleSystemsPanel,
    SemanticAction::SelectNextCapability,
    SemanticAction::SelectPreviousCapability,
    SemanticAction::ExecutePrimarySystemAction,
    SemanticAction::ExecuteSecondarySystemAction,
    SemanticAction::IncreaseSelectedSystemPower,
    SemanticAction::DecreaseSelectedSystemPower,
    SemanticAction::RecordPlaytestIssueMarker,
}};

enum class DeviceKind {
    Keyboard,
    MouseButton,
    GamepadButton,
    GamepadAxis
};

struct PhysicalBinding {
    DeviceKind device{DeviceKind::Keyboard};
    std::string identifier;

    [[nodiscard]] bool operator==(const PhysicalBinding& other) const noexcept {
        return device == other.device && identifier == other.identifier;
    }
    [[nodiscard]] bool operator!=(const PhysicalBinding& other) const noexcept {
        return !(*this == other);
    }
};

[[nodiscard]] inline PhysicalBinding key(std::string identifier) {
    return PhysicalBinding{DeviceKind::Keyboard, std::move(identifier)};
}

// Static catalog metadata for one semantic action: which context it belongs
// to, a short player-legible description (the doc's in-context
// discoverability requirement needs "semantic action" text independent of
// whatever key currently happens to be bound), and whether a remapping UI
// must refuse to leave the action with zero bindings because it is part of
// the guaranteed path to reach/operate that UI itself.
struct SemanticActionDescriptor {
    SemanticAction action{};
    MappingContext context{MappingContext::Gameplay};
    std::string description;
    bool essential_navigation{false};
};

[[nodiscard]] inline const std::vector<SemanticActionDescriptor>& semantic_action_catalog() {
    static const std::vector<SemanticActionDescriptor> catalog = [] {
        std::vector<SemanticActionDescriptor> entries;
        auto add = [&entries](SemanticAction action, MappingContext context, std::string description,
                               bool essential_navigation = false) {
            entries.push_back({action, context, std::move(description), essential_navigation});
        };

        add(SemanticAction::IncreaseForwardVelocity, MappingContext::Gameplay, "Increase forward velocity");
        add(SemanticAction::DecreaseForwardVelocity, MappingContext::Gameplay, "Decrease forward velocity");
        add(SemanticAction::IncreaseLateralVelocity, MappingContext::Gameplay, "Increase lateral velocity");
        add(SemanticAction::DecreaseLateralVelocity, MappingContext::Gameplay, "Decrease lateral velocity");
        add(SemanticAction::IncreaseVerticalVelocity, MappingContext::Gameplay, "Increase vertical velocity");
        add(SemanticAction::DecreaseVerticalVelocity, MappingContext::Gameplay, "Decrease vertical velocity");
        add(SemanticAction::YawProbeLeft, MappingContext::Gameplay, "Yaw probe left");
        add(SemanticAction::YawProbeRight, MappingContext::Gameplay, "Yaw probe right");
        add(SemanticAction::PitchProbeUp, MappingContext::Gameplay, "Pitch probe up");
        add(SemanticAction::PitchProbeDown, MappingContext::Gameplay, "Pitch probe down");
        add(SemanticAction::RollProbeLeft, MappingContext::Gameplay, "Roll probe left");
        add(SemanticAction::RollProbeRight, MappingContext::Gameplay, "Roll probe right");
        add(SemanticAction::StopPropulsion, MappingContext::Gameplay,
            "Full stop / cancel active autopilot, descent, hover, or mining approach");
        add(SemanticAction::BeginOrCancelCameraAlignedRighting, MappingContext::Gameplay,
            "Begin or cancel camera-aligned righting");
        add(SemanticAction::ToggleJoseTakeTheWheel, MappingContext::Gameplay,
            "Engage or cancel Jose Take the Wheel autopilot");
        add(SemanticAction::ToggleControlledDescent, MappingContext::Gameplay,
            "Engage or cancel controlled descent");
        add(SemanticAction::ToggleControlledHover, MappingContext::Gameplay, "Engage or cancel controlled hover");
        add(SemanticAction::SelectNearestPhysicalTarget, MappingContext::Gameplay,
            "Cycle/select the nearest physical target");
        add(SemanticAction::ToggleTractorFieldCoupling, MappingContext::Gameplay,
            "Hold to couple the tractor field to the selected target");
        add(SemanticAction::CycleMiningTarget, MappingContext::Gameplay, "Cycle available mining targets");
        add(SemanticAction::ToggleAutoApproachMiningTarget, MappingContext::Gameplay,
            "Auto-approach the selected surveyed mining target");
        add(SemanticAction::CommandMineBootstrapTarget, MappingContext::Gameplay, "Attempt resource extraction");
        add(SemanticAction::SaveGame, MappingContext::Gameplay, "Save the current game");
        add(SemanticAction::LoadGame, MappingContext::Gameplay, "Load the most recent save");

        add(SemanticAction::TogglePortManipulatorArm, MappingContext::Manipulator, "Deploy/stow the port arm");
        add(SemanticAction::ToggleStarboardManipulatorArm, MappingContext::Manipulator,
            "Deploy/stow the starboard arm");
        add(SemanticAction::ToggleManipulatorTool, MappingContext::Manipulator,
            "Attach/detach the manipulator tool");
        add(SemanticAction::ToggleSelectedManipulatorTool, MappingContext::Manipulator,
            "Attach/detach the mining tool on the selected arm");
        add(SemanticAction::ToggleManipulatorPanel, MappingContext::Manipulator, "Toggle the manipulator HUD panel");
        add(SemanticAction::CycleManipulatorArmSelection, MappingContext::Manipulator,
            "Cycle which manipulator arm is selected");
        add(SemanticAction::SelectManipulatorJointShoulder, MappingContext::Manipulator, "Select the shoulder joint");
        add(SemanticAction::SelectManipulatorJointElbow, MappingContext::Manipulator, "Select the elbow joint");
        add(SemanticAction::SelectManipulatorJointWrist, MappingContext::Manipulator, "Select the wrist joint");
        add(SemanticAction::DecreaseManipulatorJointTarget, MappingContext::Manipulator,
            "Decrease the selected joint's target angle");
        add(SemanticAction::IncreaseManipulatorJointTarget, MappingContext::Manipulator,
            "Increase the selected joint's target angle");
        add(SemanticAction::ToggleManipulatorGrasp, MappingContext::Manipulator, "Grasp or release the target");
        add(SemanticAction::CollectGraspedSample, MappingContext::Manipulator, "Collect the grasped sample");

        add(SemanticAction::ToggleControlsReference, MappingContext::UI, "Toggle the controls reference");
        add(SemanticAction::ToggleSystemsPanel, MappingContext::UI, "Toggle the systems panel",
            /*essential_navigation=*/true);
        add(SemanticAction::SelectNextCapability, MappingContext::UI, "Select the next capability");
        add(SemanticAction::SelectPreviousCapability, MappingContext::UI, "Select the previous capability");
        add(SemanticAction::ExecutePrimarySystemAction, MappingContext::UI, "Execute the primary system action",
            /*essential_navigation=*/true);
        add(SemanticAction::ExecuteSecondarySystemAction, MappingContext::UI, "Execute the secondary system action",
            /*essential_navigation=*/true);
        add(SemanticAction::IncreaseSelectedSystemPower, MappingContext::UI, "Increase selected system power");
        add(SemanticAction::DecreaseSelectedSystemPower, MappingContext::UI, "Decrease selected system power");

        add(SemanticAction::RecordPlaytestIssueMarker, MappingContext::Development,
            "Record a manual playtest issue marker");
        return entries;
    }();
    return catalog;
}

[[nodiscard]] inline const SemanticActionDescriptor& describe(SemanticAction action) {
    for (const auto& entry : semantic_action_catalog()) {
        if (entry.action == action) {
            return entry;
        }
    }
    throw std::invalid_argument("no catalog entry for this semantic action");
}

[[nodiscard]] inline std::string to_string(SemanticAction action) {
    return describe(action).description.empty() ? std::string("SemanticAction") : describe(action).description;
}

// Stable, versionable wire identifier for a semantic action -- independent
// of enum declaration order, so inserting a new action never renumbers an
// already-persisted preference file.
[[nodiscard]] inline const std::string& action_id(SemanticAction action) {
    static const std::vector<std::pair<SemanticAction, std::string>> table = {
        {SemanticAction::IncreaseForwardVelocity, "IncreaseForwardVelocity"},
        {SemanticAction::DecreaseForwardVelocity, "DecreaseForwardVelocity"},
        {SemanticAction::IncreaseLateralVelocity, "IncreaseLateralVelocity"},
        {SemanticAction::DecreaseLateralVelocity, "DecreaseLateralVelocity"},
        {SemanticAction::IncreaseVerticalVelocity, "IncreaseVerticalVelocity"},
        {SemanticAction::DecreaseVerticalVelocity, "DecreaseVerticalVelocity"},
        {SemanticAction::YawProbeLeft, "YawProbeLeft"},
        {SemanticAction::YawProbeRight, "YawProbeRight"},
        {SemanticAction::PitchProbeUp, "PitchProbeUp"},
        {SemanticAction::PitchProbeDown, "PitchProbeDown"},
        {SemanticAction::RollProbeLeft, "RollProbeLeft"},
        {SemanticAction::RollProbeRight, "RollProbeRight"},
        {SemanticAction::StopPropulsion, "StopPropulsion"},
        {SemanticAction::BeginOrCancelCameraAlignedRighting, "BeginOrCancelCameraAlignedRighting"},
        {SemanticAction::ToggleJoseTakeTheWheel, "ToggleJoseTakeTheWheel"},
        {SemanticAction::ToggleControlledDescent, "ToggleControlledDescent"},
        {SemanticAction::ToggleControlledHover, "ToggleControlledHover"},
        {SemanticAction::SelectNearestPhysicalTarget, "SelectNearestPhysicalTarget"},
        {SemanticAction::ToggleTractorFieldCoupling, "ToggleTractorFieldCoupling"},
        {SemanticAction::CycleMiningTarget, "CycleMiningTarget"},
        {SemanticAction::ToggleAutoApproachMiningTarget, "ToggleAutoApproachMiningTarget"},
        {SemanticAction::CommandMineBootstrapTarget, "CommandMineBootstrapTarget"},
        {SemanticAction::SaveGame, "SaveGame"},
        {SemanticAction::LoadGame, "LoadGame"},
        {SemanticAction::TogglePortManipulatorArm, "TogglePortManipulatorArm"},
        {SemanticAction::ToggleStarboardManipulatorArm, "ToggleStarboardManipulatorArm"},
        {SemanticAction::ToggleManipulatorTool, "ToggleManipulatorTool"},
        {SemanticAction::ToggleSelectedManipulatorTool, "ToggleSelectedManipulatorTool"},
        {SemanticAction::ToggleManipulatorPanel, "ToggleManipulatorPanel"},
        {SemanticAction::CycleManipulatorArmSelection, "CycleManipulatorArmSelection"},
        {SemanticAction::SelectManipulatorJointShoulder, "SelectManipulatorJointShoulder"},
        {SemanticAction::SelectManipulatorJointElbow, "SelectManipulatorJointElbow"},
        {SemanticAction::SelectManipulatorJointWrist, "SelectManipulatorJointWrist"},
        {SemanticAction::DecreaseManipulatorJointTarget, "DecreaseManipulatorJointTarget"},
        {SemanticAction::IncreaseManipulatorJointTarget, "IncreaseManipulatorJointTarget"},
        {SemanticAction::ToggleManipulatorGrasp, "ToggleManipulatorGrasp"},
        {SemanticAction::CollectGraspedSample, "CollectGraspedSample"},
        {SemanticAction::ToggleControlsReference, "ToggleControlsReference"},
        {SemanticAction::ToggleSystemsPanel, "ToggleSystemsPanel"},
        {SemanticAction::SelectNextCapability, "SelectNextCapability"},
        {SemanticAction::SelectPreviousCapability, "SelectPreviousCapability"},
        {SemanticAction::ExecutePrimarySystemAction, "ExecutePrimarySystemAction"},
        {SemanticAction::ExecuteSecondarySystemAction, "ExecuteSecondarySystemAction"},
        {SemanticAction::IncreaseSelectedSystemPower, "IncreaseSelectedSystemPower"},
        {SemanticAction::DecreaseSelectedSystemPower, "DecreaseSelectedSystemPower"},
        {SemanticAction::RecordPlaytestIssueMarker, "RecordPlaytestIssueMarker"},
    };
    for (const auto& [candidate, id] : table) {
        if (candidate == action) {
            return id;
        }
    }
    throw std::invalid_argument("no wire identifier for this semantic action");
}

[[nodiscard]] inline SemanticAction action_from_id(const std::string& id) {
    for (SemanticAction action : kAllSemanticActions) {
        if (action_id(action) == id) {
            return action;
        }
    }
    throw std::runtime_error("unknown semantic action id in binding preferences: " + id);
}

[[nodiscard]] inline std::string device_kind_id(DeviceKind device) {
    switch (device) {
        case DeviceKind::Keyboard: return "Keyboard";
        case DeviceKind::MouseButton: return "MouseButton";
        case DeviceKind::GamepadButton: return "GamepadButton";
        case DeviceKind::GamepadAxis: return "GamepadAxis";
    }
    throw std::invalid_argument("unknown device kind");
}

[[nodiscard]] inline DeviceKind device_kind_from_id(const std::string& id) {
    if (id == "Keyboard") return DeviceKind::Keyboard;
    if (id == "MouseButton") return DeviceKind::MouseButton;
    if (id == "GamepadButton") return DeviceKind::GamepadButton;
    if (id == "GamepadAxis") return DeviceKind::GamepadAxis;
    throw std::runtime_error("unknown device kind in binding preferences: " + id);
}

// The maximum number of simultaneous physical bindings a single semantic
// action may hold (for example a keyboard primary plus a controller
// alternative). Bounded rather than unbounded so a remapping UI has a fixed,
// presentable slot layout, matching the doc's "keyboard/mouse and
// controller alternatives" contract without implying unlimited stacking.
inline constexpr std::size_t kMaxBindingSlotsPerAction = 2;

enum class RebindResult {
    Applied,
    Conflict,
    InvalidSlot
};

enum class ClearResult {
    Applied,
    RefusedEssentialNavigation,
    InvalidSlot
};

struct RebindOutcome {
    RebindResult result{RebindResult::Applied};
    // Populated only when result == Conflict: the other action already
    // bound to the requested physical input within the same context.
    std::optional<SemanticAction> conflicting_action{};
};

// A versioned, player-mappable set of physical bindings per semantic
// action, separate from docs/SAVE_FORMAT.md gameplay persistence per the
// architecture doc's explicit requirement. Every action always has at least
// one binding under the default set; a caller may reduce it to zero only
// through clear_binding(), which fails closed for essential_navigation
// actions.
class BindingPreferences {
public:
    // A fresh preference set with no overrides: every action resolves
    // through its default binding until explicitly rebound.
    BindingPreferences() {
        for (SemanticAction action : kAllSemanticActions) {
            bindings_.push_back({action, default_binding_preferences_table(action)});
        }
    }

    [[nodiscard]] const std::vector<PhysicalBinding>& bindings_for(SemanticAction action) const {
        return entry_for(action).second;
    }

    // Given the context that is currently active and a physical input that
    // just occurred, resolves to the one semantic action it invokes, or
    // std::nullopt if nothing in that context is bound to it. Only actions
    // belonging to `context` are searched, so the same physical input can be
    // reused safely across different contexts (for example a UI-context
    // binding reusing a key a Gameplay-context action also uses) without
    // creating a real dispatch ambiguity, since only one context resolves a
    // given input at a time.
    [[nodiscard]] std::optional<SemanticAction> resolve(MappingContext context,
                                                          const PhysicalBinding& input) const {
        for (const auto& [action, physical_bindings] : bindings_) {
            if (describe(action).context != context) {
                continue;
            }
            for (const auto& binding : physical_bindings) {
                if (binding == input) {
                    return action;
                }
            }
        }
        return std::nullopt;
    }

    // Every pair of distinct actions within the same context that currently
    // share an identical physical binding. Deterministic conflict detection
    // per the doc's rebinding contract; expected to be empty for the
    // shipped default set (proven by a test below) and used by rebind() to
    // refuse creating a new one.
    [[nodiscard]] std::vector<std::pair<SemanticAction, SemanticAction>> find_conflicts(
        MappingContext context) const {
        std::vector<std::pair<SemanticAction, SemanticAction>> conflicts;
        for (std::size_t i = 0; i < bindings_.size(); ++i) {
            const auto& [action_a, bindings_a] = bindings_[i];
            if (describe(action_a).context != context) continue;
            for (std::size_t j = i + 1; j < bindings_.size(); ++j) {
                const auto& [action_b, bindings_b] = bindings_[j];
                if (describe(action_b).context != context) continue;
                const bool shares_binding = std::any_of(
                    bindings_a.begin(), bindings_a.end(), [&bindings_b](const PhysicalBinding& candidate) {
                        return std::find(bindings_b.begin(), bindings_b.end(), candidate) != bindings_b.end();
                    });
                if (shares_binding) {
                    conflicts.emplace_back(action_a, action_b);
                }
            }
        }
        return conflicts;
    }

    // Assigns `input` to `action`'s binding slot `slot_index` (0-based, must
    // be < kMaxBindingSlotsPerAction). Refuses (Conflict) without mutating
    // anything if a different action already in the same context is bound
    // to `input` -- a rebind must never silently create an ambiguous
    // dispatch. Rebinding an action to a physical input it already itself
    // holds in a different slot is idempotent-applied (not a conflict).
    [[nodiscard]] RebindOutcome rebind(SemanticAction action, std::size_t slot_index, PhysicalBinding input) {
        if (slot_index >= kMaxBindingSlotsPerAction) {
            return {RebindResult::InvalidSlot, std::nullopt};
        }
        const MappingContext context = describe(action).context;
        for (const auto& [other_action, physical_bindings] : bindings_) {
            if (other_action == action) continue;
            if (describe(other_action).context != context) continue;
            if (std::find(physical_bindings.begin(), physical_bindings.end(), input) != physical_bindings.end()) {
                return {RebindResult::Conflict, other_action};
            }
        }
        auto& physical_bindings = mutable_entry_for(action).second;
        if (physical_bindings.size() <= slot_index) {
            physical_bindings.resize(slot_index + 1, PhysicalBinding{});
        }
        physical_bindings[slot_index] = std::move(input);
        return {RebindResult::Applied, std::nullopt};
    }

    // Removes the binding at `slot_index`, refusing (RefusedEssentialNavigation)
    // if doing so would leave an essential_navigation action with zero
    // remaining bindings -- the doc's "do not silently delete a binding
    // required to reach the remapping UI" rule.
    [[nodiscard]] ClearResult clear_binding(SemanticAction action, std::size_t slot_index) {
        auto& physical_bindings = mutable_entry_for(action).second;
        if (slot_index >= physical_bindings.size()) {
            return ClearResult::InvalidSlot;
        }
        const std::size_t remaining_after =
            std::count_if(physical_bindings.begin(), physical_bindings.end(),
                           [](const PhysicalBinding& binding) { return !binding.identifier.empty(); }) -
            (physical_bindings[slot_index].identifier.empty() ? 0 : 1);
        if (remaining_after == 0 && describe(action).essential_navigation) {
            return ClearResult::RefusedEssentialNavigation;
        }
        physical_bindings.erase(physical_bindings.begin() + static_cast<std::ptrdiff_t>(slot_index));
        return ClearResult::Applied;
    }

    void restore_defaults(SemanticAction action) {
        mutable_entry_for(action).second = default_binding_preferences_table(action);
    }

    void restore_all_defaults() {
        for (SemanticAction action : kAllSemanticActions) {
            restore_defaults(action);
        }
    }

    [[nodiscard]] bool operator==(const BindingPreferences& other) const {
        if (bindings_.size() != other.bindings_.size()) return false;
        for (SemanticAction action : kAllSemanticActions) {
            if (bindings_for(action) != other.bindings_for(action)) return false;
        }
        return true;
    }

    [[nodiscard]] JsonValue to_json() const {
        JsonValue root = JsonValue::make_object();
        root.set("schema_version", JsonValue(static_cast<std::int64_t>(kBindingPreferencesSchemaVersion)));
        JsonValue actions = JsonValue::make_array();
        for (const auto& [action, physical_bindings] : bindings_) {
            JsonValue entry = JsonValue::make_object();
            entry.set("action", JsonValue(action_id(action)));
            JsonValue slots = JsonValue::make_array();
            for (const auto& binding : physical_bindings) {
                // A slot left empty by clear_binding()/rebind()'s internal
                // resize is a placeholder, not a real preference; never
                // round-trip it as one (from_json() rejects an empty
                // identifier as malformed).
                if (binding.identifier.empty()) continue;
                JsonValue slot = JsonValue::make_object();
                slot.set("device", JsonValue(device_kind_id(binding.device)));
                slot.set("identifier", JsonValue(binding.identifier));
                slots.push_back(std::move(slot));
            }
            entry.set("bindings", std::move(slots));
            actions.push_back(std::move(entry));
        }
        root.set("actions", std::move(actions));
        return root;
    }

    // Throws on any structurally invalid or unsupported-version input.
    // Callers that need the doc's "fail closed... by falling back to known
    // defaults" behavior should use load_or_default() instead, which is the
    // boundary that actually absorbs a malformed/obsolete preference file.
    [[nodiscard]] static BindingPreferences from_json(const JsonValue& value) {
        const std::int64_t schema_version = value.require("schema_version").as_int64();
        if (schema_version != kBindingPreferencesSchemaVersion) {
            throw std::runtime_error("unsupported binding preferences schema_version " +
                                      std::to_string(schema_version));
        }
        BindingPreferences preferences;
        std::vector<bool> seen(kAllSemanticActions.size(), false);
        for (const JsonValue& entry : value.require("actions").as_array()) {
            const SemanticAction action = action_from_id(entry.require("action").as_string());
            std::vector<PhysicalBinding> physical_bindings;
            for (const JsonValue& slot : entry.require("bindings").as_array()) {
                const DeviceKind device = device_kind_from_id(slot.require("device").as_string());
                const std::string identifier = slot.require("identifier").as_string();
                if (identifier.empty()) {
                    throw std::runtime_error("empty physical binding identifier in binding preferences");
                }
                physical_bindings.push_back(PhysicalBinding{device, identifier});
            }
            if (physical_bindings.size() > kMaxBindingSlotsPerAction) {
                throw std::runtime_error("too many bindings for one semantic action in binding preferences");
            }
            if (physical_bindings.empty() && describe(action).essential_navigation) {
                throw std::runtime_error("essential navigation action has no binding in binding preferences: " +
                                          action_id(action));
            }
            preferences.mutable_entry_for(action).second = std::move(physical_bindings);
            for (std::size_t i = 0; i < kAllSemanticActions.size(); ++i) {
                if (kAllSemanticActions[i] == action) {
                    seen[i] = true;
                    break;
                }
            }
        }
        // A file that does not describe every action the current schema
        // version knows about is treated as obsolete/malformed rather than
        // silently leaving the missing action(s) unbound: any catalog
        // addition must come with a schema_version bump, matching the doc's
        // "fail closed on malformed or obsolete preference data" rule.
        for (std::size_t i = 0; i < kAllSemanticActions.size(); ++i) {
            if (!seen[i]) {
                throw std::runtime_error("binding preferences missing action: " +
                                          action_id(kAllSemanticActions[i]));
            }
        }
        return preferences;
    }

    [[nodiscard]] std::string dump() const { return to_json().dump(); }

    // The doc's fail-closed contract for preference *data* (distinct from
    // gameplay save integrity, which must throw rather than substitute a
    // default): malformed, truncated, or future/obsolete-schema preference
    // JSON never blocks play or crashes the session -- it silently reverts
    // to the known-good default binding set.
    [[nodiscard]] static BindingPreferences load_or_default(const std::string& json_text) {
        try {
            return from_json(JsonValue::parse(json_text));
        } catch (const std::exception&) {
            // The default constructor already yields the full default
            // binding set (see above); this is the doc's "fail closed by
            // falling back to known defaults" boundary for preference data.
            return BindingPreferences{};
        }
    }

private:
    using Entry = std::pair<SemanticAction, std::vector<PhysicalBinding>>;
    std::vector<Entry> bindings_;

    [[nodiscard]] const Entry& entry_for(SemanticAction action) const {
        for (const auto& entry : bindings_) {
            if (entry.first == action) return entry;
        }
        throw std::invalid_argument("unknown semantic action");
    }

    [[nodiscard]] Entry& mutable_entry_for(SemanticAction action) {
        for (auto& entry : bindings_) {
            if (entry.first == action) return entry;
        }
        throw std::invalid_argument("unknown semantic action");
    }

    [[nodiscard]] static const std::vector<PhysicalBinding>& default_binding_preferences_table(
        SemanticAction action);
};

// The audited default keyboard bindings, one-to-one with the live
// `EKeys::`/`BindKey` calls in `unreal/Source/Everward` (see this header's
// top-of-file audit note). `IncreaseForwardVelocity`/`DecreaseForwardVelocity`
// are the one case with two real default bindings today (`W`/`Up` and
// `S`/`Down` respectively); every other action has exactly one.
[[nodiscard]] inline const std::vector<PhysicalBinding>& BindingPreferences::default_binding_preferences_table(
    SemanticAction action) {
    static const std::vector<std::pair<SemanticAction, std::vector<PhysicalBinding>>> table = {
        {SemanticAction::IncreaseForwardVelocity, {key("W"), key("Up")}},
        {SemanticAction::DecreaseForwardVelocity, {key("S"), key("Down")}},
        {SemanticAction::IncreaseLateralVelocity, {key("D")}},
        {SemanticAction::DecreaseLateralVelocity, {key("A")}},
        {SemanticAction::IncreaseVerticalVelocity, {key("E")}},
        {SemanticAction::DecreaseVerticalVelocity, {key("Q")}},
        {SemanticAction::YawProbeLeft, {key("J")}},
        {SemanticAction::YawProbeRight, {key("L")}},
        {SemanticAction::PitchProbeUp, {key("I")}},
        {SemanticAction::PitchProbeDown, {key("K")}},
        {SemanticAction::RollProbeLeft, {key("U")}},
        {SemanticAction::RollProbeRight, {key("O")}},
        {SemanticAction::StopPropulsion, {key("SpaceBar")}},
        {SemanticAction::BeginOrCancelCameraAlignedRighting, {key("R")}},
        {SemanticAction::ToggleJoseTakeTheWheel, {key("Y")}},
        {SemanticAction::ToggleControlledDescent, {key("C")}},
        {SemanticAction::ToggleControlledHover, {key("V")}},
        {SemanticAction::SelectNearestPhysicalTarget, {key("T")}},
        {SemanticAction::ToggleTractorFieldCoupling, {key("B")}},
        {SemanticAction::CycleMiningTarget, {key("H")}},
        {SemanticAction::ToggleAutoApproachMiningTarget, {key("P")}},
        {SemanticAction::CommandMineBootstrapTarget, {key("G")}},
        {SemanticAction::SaveGame, {key("F5")}},
        {SemanticAction::LoadGame, {key("F6")}},
        {SemanticAction::TogglePortManipulatorArm, {key("One")}},
        {SemanticAction::ToggleStarboardManipulatorArm, {key("Two")}},
        {SemanticAction::ToggleManipulatorTool, {key("Three")}},
        {SemanticAction::ToggleSelectedManipulatorTool, {key("Seven")}},
        {SemanticAction::ToggleManipulatorPanel, {key("M")}},
        {SemanticAction::CycleManipulatorArmSelection, {key("N")}},
        {SemanticAction::SelectManipulatorJointShoulder, {key("Four")}},
        {SemanticAction::SelectManipulatorJointElbow, {key("Five")}},
        {SemanticAction::SelectManipulatorJointWrist, {key("Six")}},
        {SemanticAction::DecreaseManipulatorJointTarget, {key("Comma")}},
        {SemanticAction::IncreaseManipulatorJointTarget, {key("Period")}},
        {SemanticAction::ToggleManipulatorGrasp, {key("F")}},
        {SemanticAction::CollectGraspedSample, {key("X")}},
        {SemanticAction::ToggleControlsReference, {key("F1")}},
        {SemanticAction::ToggleSystemsPanel, {key("Tab")}},
        {SemanticAction::SelectNextCapability, {key("RightBracket")}},
        {SemanticAction::SelectPreviousCapability, {key("LeftBracket")}},
        {SemanticAction::ExecutePrimarySystemAction, {key("Enter")}},
        {SemanticAction::ExecuteSecondarySystemAction, {key("BackSpace")}},
        {SemanticAction::IncreaseSelectedSystemPower, {key("PageUp")}},
        {SemanticAction::DecreaseSelectedSystemPower, {key("PageDown")}},
        {SemanticAction::RecordPlaytestIssueMarker, {key("F12")}},
    };
    for (const auto& [candidate, physical_bindings] : table) {
        if (candidate == action) {
            return physical_bindings;
        }
    }
    throw std::invalid_argument("no default binding table entry for this semantic action");
}

[[nodiscard]] inline BindingPreferences default_binding_preferences() {
    BindingPreferences preferences;
    preferences.restore_all_defaults();
    return preferences;
}

} // namespace everward::simulation
