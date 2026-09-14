# Phase 2 Input Remapping and Accessibility Architecture

Issue: #165

## Purpose

Define the next control-system architecture without claiming Unreal Product Reality. The current Phase-2 controls are sufficient for deterministic development, but key bindings must stop proliferating as hard-coded gameplay checks before the vertical slice grows further.

This document authorizes no visual acceptance and changes no simulation authority. It defines a migration path so input presentation can evolve independently while the deterministic simulation remains the source of game-state truth.

## Architectural boundary

```text
physical device input
    -> Unreal Enhanced Input mapping contexts
    -> semantic player actions
    -> player-controller/adaptor commands
    -> authoritative simulation command/state transition
    -> HUD/status presentation
```

Rules:

1. Device keys/buttons are presentation/input bindings, not simulation commands.
2. Simulation code receives semantic commands only; it must not depend on a keyboard key, controller button, display label, or remapping UI.
3. Keyboard/mouse and controller paths resolve to the same semantic action where their intent is equivalent.
4. Rebinding may change which physical control invokes an action but must never change action semantics, validation, resource cost, difficulty rules, deterministic state, or save authority.
5. An unavailable action remains unavailable after remapping. Input architecture must not bypass subsystem/integrity/resource prerequisites.

## Phase-2 semantic action catalog

The first migration should inventory existing player-controller commands and group them by intent rather than key name. Expected categories are:

- flight/translation/rotation;
- target selection and target cycling;
- scanner interaction;
- manipulator deploy/stow, joint selection, joint motion, grasp/release;
- mining/resource interaction;
- repair/recovery commands where implemented;
- HUD/page/controls-reference navigation;
- save/load development actions where they remain player-facing;
- pause/camera or other Unreal-only presentation actions.

The implementation PR must derive the exact action list from live controller code before mutation. This document does not invent missing mechanics.

## Enhanced Input migration

### Mapping contexts

Use Unreal Enhanced Input mapping contexts with explicit priority boundaries:

- `IMC_EverwardGameplay`: normal flight and interaction actions;
- `IMC_EverwardManipulator`: manipulator-specific actions when that interaction mode is active;
- `IMC_EverwardUI`: HUD/menu/remapping navigation;
- development-only mappings remain separate from production gameplay bindings where possible.

Names are architectural examples. The implementation may use repository-conforming Unreal asset/class names, but the separation of gameplay, manipulator, UI, and development-only concerns should remain.

### Input actions

Each action should represent semantic intent, for example `MoveForward`, `CycleTargetNext`, `ManipulatorGrasp`, rather than `PressW` or `PressG`. Analog-capable actions should preserve analog values through the Unreal adapter without making the simulation frame-rate dependent.

### Adapter boundary

The Unreal player controller translates Enhanced Input callbacks into the same deterministic command interfaces used today. Do not move game rules into input callbacks. When a command is rejected by authoritative state, surface the reason to the presentation layer.

## Rebinding contract

A remapping UI should operate on player-mappable Enhanced Input bindings and store only user input preferences, separate from authoritative game-save state.

Required behavior:

- show current binding and semantic action name;
- permit keyboard/mouse and controller alternatives where supported;
- detect conflicts before commit;
- offer explicit replace/cancel behavior for a conflict;
- preserve a guaranteed path to UI navigation/confirm/cancel;
- provide restore-defaults per action group and globally;
- fail closed on malformed or obsolete preference data by falling back to known defaults;
- do not silently delete a binding required to reach the remapping UI.

Binding persistence should be versioned independently from `docs/SAVE_FORMAT.md` gameplay persistence. A control-layout migration must not become a save-schema migration.

## Controller path

Controller support should be designed alongside remapping rather than added as a second control system later.

- Equivalent keyboard and controller inputs invoke the same semantic actions.
- Stick/dead-zone/scalar processing belongs in input presentation/configuration, not simulation truth.
- Controller prompts are derived from the active binding, never hard-coded strings.
- If a semantic action has no valid controller binding, the UI must show that limitation rather than implying support.

## Accessibility foundation

Phase 2 should establish architecture for accessibility without promising a complete accessibility feature set in the first implementation.

Initial settings should be separable into:

- HUD/text scale;
- high-contrast/readability presentation options where supported;
- reduced or disabled nonessential motion/animation in HUD presentation;
- input sensitivity/dead-zone settings for analog controls;
- hold-versus-toggle behavior only where the underlying action semantics safely permit it;
- remappable controls;
- controller and keyboard prompt switching based on active input method.

Color must not be the sole carrier of PASS/WARNING/FAIL, subsystem state, target state, or action availability.

## In-context discoverability

The F1 controls reference remains useful but is not the sole discovery mechanism.

For context-sensitive actions, presentation should be able to expose:

- semantic action;
- currently bound control;
- whether the action is currently available;
- concise prerequisite/rejection reason when unavailable.

Examples include a manipulator action while the arm is stowed, mining without a valid target, or repair without required resources. The authoritative system supplies availability/rejection state; the HUD formats it.

## Determinism and testing

The migration is acceptable only if input architecture changes do not alter deterministic mechanics.

Automated coverage should include, where practical:

1. multiple physical bindings dispatch the same semantic command;
2. rebinding changes physical input only, not the resulting command payload;
3. conflict detection prevents ambiguous active bindings;
4. invalid preference data falls back safely;
5. action rejection still comes from authoritative game state;
6. controller/keyboard prompt selection reflects active bindings;
7. deterministic simulation tests remain unchanged and GREEN.

Unreal-level automation can verify mapping/configuration mechanics when available. It does not replace human control-feel or HUD-readability acceptance.

## Product Reality boundary

Automated tests can verify mapping identity, dispatch semantics, persistence of preferences, deterministic command equivalence, and fail-closed configuration handling.

Human/Unreal Product Reality remains required for:

- control feel;
- sensible default bindings;
- controller ergonomics;
- discoverability in the real HUD;
- text/readability at target resolutions;
- prompt placement and information density;
- whether remapping/accessibility screens are understandable to a first-time player.

No automated PASS clears those experience gates.

## Migration sequence

1. Inventory live hard-coded controller inputs and map each to an existing semantic command/state transition.
2. Add Enhanced Input actions and contexts without changing simulation interfaces.
3. Route existing default keyboard behavior through the new semantic input layer and prove behavioral equivalence.
4. Add controller bindings against the same semantic actions.
5. Add versioned player-mappable preference persistence and deterministic conflict/fallback tests.
6. Add remapping UI and binding-aware prompts.
7. Add initial accessibility settings whose implementation does not change game mechanics.
8. Run the existing HUD/control Product Reality script plus dedicated remapping/controller/readability acceptance.

Each implementation step should remain independently reviewable and exact-head verified. Do not combine a broad control rewrite, UI redesign, gameplay mechanic change, and accessibility pass into one PR.

## Exit criteria for architecture phase

This architecture phase is complete when repository implementation work can proceed without deciding basic ownership boundaries again:

- Enhanced Input owns physical bindings;
- Unreal presentation owns prompts/remapping UI;
- an adapter maps semantic actions to existing authoritative commands;
- simulation remains device-agnostic and deterministic;
- control preferences are separate from authoritative gameplay saves;
- automated tests cover equivalence/fail-closed configuration;
- Unreal Product Reality remains explicit for feel/readability/discoverability.
