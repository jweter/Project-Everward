# Graphics Scalability and Full-Game Accessibility Standard

Everward must remain **fully playable across its entire supported graphics range**. Graphics quality may change dramatically between low-end and high-end hardware, but the game itself must not.

> **Low-end hardware gets the whole game. High-end hardware gets the spectacle.**

Graphics settings are a presentation layer. They must never become a gameplay layer.

## Full-game parity requirement

A player using the lowest supported graphics preset must retain access to the same gameplay systems, progression, simulation, saves, mechanics, information, objectives, and interactions as a player using the highest preset.

The following must remain functionally equivalent at every quality level:

- movement, propulsion, navigation, and controlled descent;
- mining, resource collection, storage, processing, and recycling;
- manipulators, tools, and tractor-field mechanics;
- scanning, targeting, and environmental awareness;
- damage, repair, replacement, and the `Fix_It` system;
- probe evolution, child probes, lineage, and persistence;
- AI and deterministic simulation behavior;
- save/load compatibility and progression;
- HUD information, alerts, targeting cues, and controls;
- hazards, physics, and gameplay difficulty;
- all interactable objects required to play or progress.

A graphics preset must never hide, disable, simplify, or materially alter a gameplay mechanic merely because its highest-fidelity presentation is expensive.

## Graceful visual degradation

When an effect is too expensive for lower hardware, Everward should substitute a cheaper representation rather than remove the information it communicates.

Examples:

- a cinematic volumetric thruster plume may become a simpler particle or emissive effect;
- a mining beam may lose volumetrics, reflections, and dense impact particles while retaining a readable beam and impact cue;
- a complex translucent tractor-field visualization may become a lightweight line, arc, icon, or simplified field indicator;
- expensive environmental fog, particles, and lighting may be reduced while navigation, hazard, targeting, and interaction cues remain fully readable.

At every preset the player must still be able to understand **what is happening, what is being targeted, what a system is doing, whether it succeeded, and whether the player needs to react**.

Readability and gameplay communication outrank visual fidelity.

## Required quality presets

Everward should provide at least these standardized presets:

| Preset | Purpose |
| --- | --- |
| **Emergency / Minimum** | Maximum compatibility and minimum memory/GPU load; a guaranteed-boot fallback where supported hardware is severely constrained. |
| **Low** | Full enjoyable gameplay on modest systems; the primary current laptop-development target. |
| **Medium** | Balanced fidelity and performance for mainstream hardware. |
| **High** | High-quality presentation for capable gaming systems. |
| **Ultra / Cinematic** | Maximum intended visual fidelity and the long-term showcase target. |
| **Custom** | Starts from a preset and allows independent tuning of individual graphics categories. |

Presets should be data-driven and maintainable rather than implemented as unrelated hard-coded branches.

## What presets may change

Presets may scale presentation and rendering costs such as:

- render resolution and screen percentage;
- upscaling strategy;
- texture resolution and streaming budget;
- shadow resolution and distance;
- global illumination and reflections;
- post-processing;
- anti-aliasing;
- effects density and particle counts;
- volumetrics;
- foliage and decorative environment density;
- view distance and presentation LODs;
- non-authoritative animation presentation detail;
- decals and environmental decoration;
- frame-rate targets and VSync policy.

They may **not** alter authoritative simulation frequency, physics rules, deterministic state, resource quantities, gameplay detection ranges, damage calculations, AI logic, mining yield, tractor behavior, progression rules, save semantics, spawn logic, or other gameplay-affecting values.

Performance optimization must never quietly become difficulty adjustment.

## Low-Spec Play Mode

During development, **Low** is the primary profile for Jeremy's current laptop testing. The target is not merely to make Unreal barely launch; it is to provide an actually playable version of Everward at reduced fidelity.

Initial low-spec development targets include:

- approximately 1280×720 windowed rendering where practical;
- approximately 30 FPS development cap;
- reduced render scale where needed;
- aggressively reduced shadows, global illumination, reflections, volumetrics, post-processing, and decorative effects;
- bounded texture-streaming memory appropriate for integrated graphics/shared memory;
- reduced presentation complexity while preserving HUD and gameplay readability;
- constrained build parallelism on low-memory machines;
- avoidance of unnecessary rebuilds when exact-head binaries are current;
- use of the smallest Unreal/editor/runtime environment necessary for the current test.

Jeremy should still be able to fly the probe, explore, scan, mine, manipulate objects, use tractor mechanics, inspect systems, exercise `Fix_It`, test damage/recovery, use the HUD, save/load, and generally experience Everward as a game.

**Emergency / Minimum** exists beneath Low as a more aggressive fallback when memory pressure makes Low impractical.

## Memory and hardware scalability

Graphics presets must manage memory pressure as deliberately as GPU load. Track and tune:

- texture streaming pools;
- render-target and buffer memory;
- retained LODs;
- effect budgets;
- scene complexity;
- peak RAM during startup, world load, and transitions;
- editor/build memory spikes during local iteration.

Low and Minimum should be explicitly tested on constrained shared-memory/integrated-GPU environments rather than being assumed to work because individual settings are labeled "Low".

## Preset-equivalence testing

Everward should include automated or developer-run tests proving that graphics quality does not change authoritative gameplay.

A deterministic scenario should be capable of running under multiple presentation profiles and producing the same authoritative gameplay result. For example:

```text
Low:   mine target -> 25.0 kg extracted -> 25.0 kg stored
Ultra: mine target -> 25.0 kg extracted -> 25.0 kg stored
```

Visual output may differ radically. Simulation output must not.

Preset-equivalence coverage should expand across tractor interactions, movement, damage, repair, save/load, resource routing, probe state, targeting, and other deterministic systems.

A graphics setting that changes authoritative gameplay is a regression unless that difference was explicitly designed as a separate gameplay option.

## HUD and accessibility requirement

Critical gameplay information must remain legible at every preset. Lower settings may simplify effects, but they may not remove essential targeting indicators, interaction prompts, warnings, navigation information, resource state, system state, or critical feedback.

Lower presets may sometimes require stronger simplified indicators because expensive environmental effects no longer provide the same visual context.

## Save compatibility

A save created on Ultra must load normally on Minimum, and a save created on Minimum must load normally on Ultra.

Graphics configuration belongs to presentation/user configuration and must not become authoritative universe state. Changing graphics settings must never alter the player's universe.

## Development and release principle

Everward's cinematic presentation remains a major product goal. Supporting modest hardware does not mean redesigning the game around low fidelity. The architecture should permit the same underlying world and simulation to be represented at several levels of visual sophistication.

- **Minimum:** still unmistakably Everward and fully playable.
- **Low:** still enjoyable and suitable for full gameplay.
- **Medium:** balanced intended experience.
- **High:** rich presentation for capable hardware.
- **Ultra / Cinematic:** full visual ambition.

The hardware determines **how beautifully the universe is rendered**, not **how much of the universe the player is allowed to experience**.

## Related implementation and Product Reality work

- Issue #228: low-spec laptop development/playtest mode.
- Issue #216: iPhone Mobile Product Reality simulation/state review harness.

The mobile harness can validate deterministic/state evidence but cannot substitute for Unreal-only rendering, visual alignment, collision/game feel, camera behavior, or performance acceptance.