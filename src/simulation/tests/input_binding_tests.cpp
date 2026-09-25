#include "everward/simulation/input_binding.hpp"

#undef NDEBUG
#include <cassert>
#include <cstdio>

namespace {

using namespace everward::simulation;

void test_catalog_covers_every_semantic_action_exactly_once() {
    int seen = 0;
    for (SemanticAction action : kAllSemanticActions) {
        (void)describe(action);
        ++seen;
    }
    assert(seen == static_cast<int>(kAllSemanticActions.size()));
    assert(semantic_action_catalog().size() == kAllSemanticActions.size());
}

void test_every_action_has_a_stable_round_tripping_wire_id() {
    for (SemanticAction action : kAllSemanticActions) {
        const std::string& id = action_id(action);
        assert(!id.empty());
        assert(action_from_id(id) == action);
    }
}

void test_action_from_unknown_id_throws() {
    bool threw = false;
    try {
        (void)action_from_id("NotARealAction");
    } catch (const std::runtime_error&) {
        threw = true;
    }
    assert(threw);
}

void test_default_bindings_have_no_within_context_conflicts() {
    BindingPreferences defaults = default_binding_preferences();
    for (MappingContext context :
         {MappingContext::Gameplay, MappingContext::Manipulator, MappingContext::UI, MappingContext::Development}) {
        assert(defaults.find_conflicts(context).empty());
    }
}

void test_every_action_has_at_least_one_default_binding() {
    BindingPreferences defaults = default_binding_preferences();
    for (SemanticAction action : kAllSemanticActions) {
        assert(!defaults.bindings_for(action).empty());
    }
}

void test_multiple_physical_bindings_dispatch_the_same_semantic_command() {
    // Real live-controller default: both W and Up increase forward velocity.
    BindingPreferences defaults = default_binding_preferences();
    const auto resolved_w = defaults.resolve(MappingContext::Gameplay, key("W"));
    const auto resolved_up = defaults.resolve(MappingContext::Gameplay, key("Up"));
    assert(resolved_w.has_value() && *resolved_w == SemanticAction::IncreaseForwardVelocity);
    assert(resolved_up.has_value() && *resolved_up == SemanticAction::IncreaseForwardVelocity);
}

void test_resolve_is_scoped_to_the_active_context() {
    BindingPreferences defaults = default_binding_preferences();
    // F1 (ToggleControlsReference) is a UI-context action; it must not
    // resolve while Gameplay is the active context.
    assert(!defaults.resolve(MappingContext::Gameplay, key("F1")).has_value());
    const auto resolved = defaults.resolve(MappingContext::UI, key("F1"));
    assert(resolved.has_value() && *resolved == SemanticAction::ToggleControlsReference);
}

void test_resolve_returns_nullopt_for_an_unbound_input() {
    BindingPreferences defaults = default_binding_preferences();
    assert(!defaults.resolve(MappingContext::Gameplay, key("F9")).has_value());
}

void test_rebind_changes_physical_input_not_the_resulting_command() {
    BindingPreferences preferences = default_binding_preferences();
    const auto outcome = preferences.rebind(SemanticAction::YawProbeLeft, 0, key("Z"));
    assert(outcome.result == RebindResult::Applied);

    // The new physical input now resolves to the unchanged semantic action.
    const auto resolved_new = preferences.resolve(MappingContext::Gameplay, key("Z"));
    assert(resolved_new.has_value() && *resolved_new == SemanticAction::YawProbeLeft);

    // The old physical input no longer resolves to anything: it was the
    // action's only binding and slot 0 was overwritten, not appended.
    assert(!preferences.resolve(MappingContext::Gameplay, key("J")).has_value());
}

void test_rebind_detects_conflict_within_the_same_context_and_does_not_mutate() {
    BindingPreferences preferences = default_binding_preferences();
    // T is already SelectNearestPhysicalTarget's default Gameplay binding.
    const auto outcome = preferences.rebind(SemanticAction::YawProbeLeft, 0, key("T"));
    assert(outcome.result == RebindResult::Conflict);
    assert(outcome.conflicting_action.has_value() &&
           *outcome.conflicting_action == SemanticAction::SelectNearestPhysicalTarget);
    // No mutation: YawProbeLeft is still bound to its original J.
    assert(preferences.resolve(MappingContext::Gameplay, key("J")).has_value());
    assert(*preferences.resolve(MappingContext::Gameplay, key("J")) == SemanticAction::YawProbeLeft);
}

void test_rebind_across_different_contexts_is_not_a_conflict() {
    BindingPreferences preferences = default_binding_preferences();
    // ToggleSystemsPanel (UI) may legitimately reuse a Gameplay key (Y),
    // since only one context resolves a given input at a time.
    const auto outcome = preferences.rebind(SemanticAction::ToggleSystemsPanel, 0, key("Y"));
    assert(outcome.result == RebindResult::Applied);
}

void test_rebind_to_an_actions_own_existing_binding_in_another_slot_applies() {
    BindingPreferences preferences = default_binding_preferences();
    const auto outcome = preferences.rebind(SemanticAction::IncreaseForwardVelocity, 1, key("Up"));
    assert(outcome.result == RebindResult::Applied);
}

void test_rebind_rejects_an_out_of_range_slot() {
    BindingPreferences preferences = default_binding_preferences();
    const auto outcome = preferences.rebind(SemanticAction::YawProbeLeft, kMaxBindingSlotsPerAction, key("Z"));
    assert(outcome.result == RebindResult::InvalidSlot);
}

void test_clear_binding_refuses_to_empty_an_essential_navigation_action() {
    BindingPreferences preferences = default_binding_preferences();
    assert(describe(SemanticAction::ExecutePrimarySystemAction).essential_navigation);
    const auto result = preferences.clear_binding(SemanticAction::ExecutePrimarySystemAction, 0);
    assert(result == ClearResult::RefusedEssentialNavigation);
    assert(!preferences.bindings_for(SemanticAction::ExecutePrimarySystemAction).empty());
}

void test_clear_binding_permits_emptying_a_non_essential_action() {
    BindingPreferences preferences = default_binding_preferences();
    assert(!describe(SemanticAction::YawProbeLeft).essential_navigation);
    const auto result = preferences.clear_binding(SemanticAction::YawProbeLeft, 0);
    assert(result == ClearResult::Applied);
    assert(preferences.bindings_for(SemanticAction::YawProbeLeft).empty());
}

void test_restore_defaults_reverts_a_single_action() {
    BindingPreferences preferences = default_binding_preferences();
    (void)preferences.rebind(SemanticAction::YawProbeLeft, 0, key("Z"));
    preferences.restore_defaults(SemanticAction::YawProbeLeft);
    assert(preferences.bindings_for(SemanticAction::YawProbeLeft) ==
           default_binding_preferences().bindings_for(SemanticAction::YawProbeLeft));
}

void test_restore_all_defaults_reverts_every_action() {
    BindingPreferences preferences = default_binding_preferences();
    (void)preferences.rebind(SemanticAction::YawProbeLeft, 0, key("Z"));
    (void)preferences.clear_binding(SemanticAction::PitchProbeUp, 0);
    preferences.restore_all_defaults();
    assert(preferences == default_binding_preferences());
}

void test_json_round_trip_preserves_every_binding() {
    BindingPreferences preferences = default_binding_preferences();
    (void)preferences.rebind(SemanticAction::YawProbeLeft, 0, key("Z"));
    const std::string dumped = preferences.dump();
    const BindingPreferences reloaded = BindingPreferences::from_json(JsonValue::parse(dumped));
    assert(reloaded == preferences);
}

void test_from_json_rejects_unsupported_schema_version() {
    BindingPreferences preferences = default_binding_preferences();
    JsonValue json = preferences.to_json();
    json.replace("schema_version", JsonValue(static_cast<std::int64_t>(999)));
    bool threw = false;
    try {
        (void)BindingPreferences::from_json(json);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    assert(threw);
}

void test_from_json_rejects_an_action_missing_from_the_file() {
    BindingPreferences preferences = default_binding_preferences();
    JsonValue json = preferences.to_json();
    // Drop the last action entry to simulate a file from an older schema
    // that predates a newly added action.
    const JsonValue& original_actions = json.require("actions");
    JsonValue trimmed = JsonValue::make_array();
    const auto& array = original_actions.as_array();
    for (std::size_t i = 0; i + 1 < array.size(); ++i) {
        trimmed.push_back(array[i]);
    }
    json.replace("actions", std::move(trimmed));
    bool threw = false;
    try {
        (void)BindingPreferences::from_json(json);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    assert(threw);
}

void test_from_json_rejects_zero_bindings_for_an_essential_navigation_action() {
    BindingPreferences preferences = default_binding_preferences();
    JsonValue json = preferences.to_json();
    JsonValue actions = JsonValue::make_array();
    for (const JsonValue& entry : json.require("actions").as_array()) {
        if (entry.require("action").as_string() == action_id(SemanticAction::ExecutePrimarySystemAction)) {
            JsonValue emptied = JsonValue::make_object();
            emptied.set("action", JsonValue(action_id(SemanticAction::ExecutePrimarySystemAction)));
            emptied.set("bindings", JsonValue::make_array());
            actions.push_back(std::move(emptied));
        } else {
            actions.push_back(entry);
        }
    }
    json.replace("actions", std::move(actions));
    bool threw = false;
    try {
        (void)BindingPreferences::from_json(json);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    assert(threw);
}

void test_load_or_default_falls_back_on_malformed_json() {
    const BindingPreferences loaded = BindingPreferences::load_or_default("{ not valid json");
    assert(loaded == default_binding_preferences());
}

void test_load_or_default_falls_back_on_unsupported_schema_version() {
    BindingPreferences preferences = default_binding_preferences();
    (void)preferences.rebind(SemanticAction::YawProbeLeft, 0, key("Z"));
    JsonValue json = preferences.to_json();
    json.replace("schema_version", JsonValue(static_cast<std::int64_t>(999)));
    const BindingPreferences loaded = BindingPreferences::load_or_default(json.dump());
    // Falls back to defaults, not to the (unsupported-version) customized
    // preferences that were dumped.
    assert(loaded == default_binding_preferences());
}

void test_load_or_default_preserves_valid_customization() {
    BindingPreferences preferences = default_binding_preferences();
    (void)preferences.rebind(SemanticAction::YawProbeLeft, 0, key("Z"));
    const BindingPreferences loaded = BindingPreferences::load_or_default(preferences.dump());
    assert(loaded == preferences);
    assert(!(loaded == default_binding_preferences()));
}

void test_controller_alternative_binding_can_be_added_without_removing_the_keyboard_default() {
    // Controller default bindings are deliberately not shipped yet (a later
    // Unreal-side migration slice per the architecture doc), but the
    // binding model itself already supports a controller alternative
    // alongside the existing keyboard default.
    BindingPreferences preferences = default_binding_preferences();
    const auto outcome = preferences.rebind(SemanticAction::ToggleManipulatorGrasp, 1,
                                             PhysicalBinding{DeviceKind::GamepadButton, "FaceButton_Bottom"});
    assert(outcome.result == RebindResult::Applied);
    const auto& bindings = preferences.bindings_for(SemanticAction::ToggleManipulatorGrasp);
    assert(bindings.size() == 2);
    assert(bindings[0] == key("F"));
    assert(bindings[1].device == DeviceKind::GamepadButton);
}

} // namespace

int main() {
    test_catalog_covers_every_semantic_action_exactly_once();
    test_every_action_has_a_stable_round_tripping_wire_id();
    test_action_from_unknown_id_throws();
    test_default_bindings_have_no_within_context_conflicts();
    test_every_action_has_at_least_one_default_binding();
    test_multiple_physical_bindings_dispatch_the_same_semantic_command();
    test_resolve_is_scoped_to_the_active_context();
    test_resolve_returns_nullopt_for_an_unbound_input();
    test_rebind_changes_physical_input_not_the_resulting_command();
    test_rebind_detects_conflict_within_the_same_context_and_does_not_mutate();
    test_rebind_across_different_contexts_is_not_a_conflict();
    test_rebind_to_an_actions_own_existing_binding_in_another_slot_applies();
    test_rebind_rejects_an_out_of_range_slot();
    test_clear_binding_refuses_to_empty_an_essential_navigation_action();
    test_clear_binding_permits_emptying_a_non_essential_action();
    test_restore_defaults_reverts_a_single_action();
    test_restore_all_defaults_reverts_every_action();
    test_json_round_trip_preserves_every_binding();
    test_from_json_rejects_unsupported_schema_version();
    test_from_json_rejects_an_action_missing_from_the_file();
    test_from_json_rejects_zero_bindings_for_an_essential_navigation_action();
    test_load_or_default_falls_back_on_malformed_json();
    test_load_or_default_falls_back_on_unsupported_schema_version();
    test_load_or_default_preserves_valid_customization();
    test_controller_alternative_binding_can_be_added_without_removing_the_keyboard_default();
    std::puts("input_binding_tests: all tests passed");
    return 0;
}
