# Input Remapping and Accessibility Architecture

Status: **Architecture slice implemented; runtime/UI implementation pending.**

Tracks issue #165. This document defines the repository boundary for replacing hard-coded player bindings without claiming Unreal Product Reality.

## Goals

Everward must provide a single authoritative input-action layer that supports keyboard/mouse and controller, user remapping, readable prompts, and accessibility settings while preserving deterministic simulation and player agency.

Input configuration is presentation/control intent. It must not become simulation truth.

## Ownership boundary

- Unreal input adapters translate device events into named player intents.
- Player intents call the existing authoritative gameplay/controller commands.
- Simulation state remains authoritative for whether an action is legal and for its consequences.
- HUD/help/prompt surfaces read the same action/binding catalogue used by input registration; they must not maintain a second hand-written binding table.
- Save data may persist user binding/accessibility preferences, but never authoritative simulation outcomes.

This keeps remapping from bypassing command validation or creating a second gameplay implementation.

## Named action catalogue

Runtime implementation should introduce stable semantic actions rather than exposing physical keys to gameplay code. Initial groups mirror the existing F1 reference:

### Flight and camera
Named actions cover current flight, full-stop, orientation/righting, camera, and target-selection intents.

### Systems
Named actions cover the current system selection, power adjustment, scan, and other existing system commands.

### Manipulator and mining
Named actions cover arm deployment/stow, arm/joint/tool selection and adjustment, grasp/release, and mining.

The implementation must inventory the current controller bindings before replacing them. Existing behavior is the compatibility baseline; this architecture does not authorize new mechanics.

## Enhanced Input strategy

Use Unreal Enhanced Input as the maintained device-to-intent boundary.

1. Define one Input Action asset per semantic action.
2. Group default mappings into one or more Input Mapping Contexts by control domain where useful.
3. Register contexts through the player/controller lifecycle rather than polling hard-coded keys for gameplay decisions.
4. Route triggered actions into the same authoritative command methods used today.
5. Keep device-specific defaults in mapping data, not in simulation code.
6. Permit keyboard/mouse and controller mappings to coexist.

Migration should be incremental. A converted action must have deterministic or automation coverage showing that its semantic command path remains unchanged before the old hard-coded poll is removed.

## Remapping model

A user remap is a preference mapping:

`semantic action -> one or more physical inputs`.

Requirements:

- detect and visibly resolve conflicts instead of silently stealing a binding;
- provide Restore Defaults per action group and globally;
- allow more than one physical input for actions where Unreal supports it cleanly;
- preserve required escape/menu/navigation access;
- never let a remap alter action semantics, command validation, cooldowns, resource costs, physics, or simulation timing;
- version persisted mapping preferences so future action-catalogue changes can migrate or safely fall back to defaults.

Unknown or invalid persisted actions fail closed to maintained defaults rather than disabling gameplay controls.

## Controller path

Controller support uses the same semantic actions as keyboard/mouse. It must not introduce controller-only gameplay behavior.

The first runtime pass should cover every action needed for the current playable loop before controller support is called complete. Analog flight/camera inputs may preserve continuous values where the existing command path supports them; discrete actions remain discrete intents.

## Accessibility settings

The initial architecture supports presentation/control preferences that do not alter simulation authority:

- HUD/text scale within tested layout bounds;
- high-contrast/readability presentation options where supported;
- reduced motion/camera-effect preferences where presentation-only;
- input sensitivity and axis inversion;
- hold/toggle behavior only where command semantics remain equivalent;
- remappable controls.

Difficulty, resource economics, physics, damage, progression, and deterministic universe state are not accessibility presentation settings unless separately authorized by governing design.

## In-context prompts

Interaction prompts must resolve the current physical binding from the action catalogue at render time. A prompt should communicate:

1. the available semantic action;
2. its current binding;
3. when unavailable, the authoritative rejection/prerequisite or recovery action.

The F1 controls page must consume the same catalogue. This prevents remapping from making help text stale.

## Persistence

Persist only user preference data: mapping overrides and accessibility presentation settings. Keep it versioned separately from authoritative world/simulation state. Loading malformed or obsolete preference data must not corrupt or invalidate a gameplay save.

## Verification plan

Runtime implementation is not complete until repository-native deterministic checks cover:

- semantic action registration is unique and complete for the migrated scope;
- default bindings preserve the current command path;
- remap serialization round-trips;
- invalid/obsolete mappings fall back safely;
- binding conflicts are detected;
- F1/help/prompt surfaces resolve remapped bindings from the shared catalogue;
- keyboard and controller events reach equivalent semantic intents where applicable;
- no input layer directly mutates authoritative simulation state.

Portable CI can verify these contracts where engine-independent seams exist. Unreal compile/PIE, layout/readability, controller feel, and actual remapping UX remain Product Reality/environment-dependent gates.

## Product Reality boundary

This architecture does **not** clear the existing HUD/control Product Reality debt. The exact local Unreal test in `PHASE2_HUD_LEGIBILITY_AND_CONTROLS_TEST.md` remains authoritative for the implemented HUD. Future remapping UI/controller work additionally requires local Unreal validation for discoverability, readability, focus/navigation, and physical feel.

## Implementation sequence

1. Inventory current physical bindings and map each to a stable semantic action.
2. Add the shared action/binding catalogue and regression tests without changing behavior.
3. Introduce Enhanced Input contexts and migrate one control group at a time.
4. Make F1 and in-context prompts resolve bindings from the catalogue.
5. Add preference persistence and conflict-safe remapping UI.
6. Add controller defaults and navigation.
7. Add accessibility presentation settings.
8. Run repository-native preflight/CI, then the local Unreal Product Reality pass.

Each step should remain small, reversible, and independently reviewable.