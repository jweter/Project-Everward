# Phase 2 Kickoff Scaffold

This document records the architecture contract for **Phase 2 — One Probe** established by ADR-0012 and reconciles it with the implementation now present after PR #100.

It elaborates `ARCHITECTURE.md`, `SIMULATION_PHILOSOPHY.md`, `SAVE_FORMAT.md`, ADR-0002, and ADR-0012. `PROJECT_STATUS.md` remains the operational continuation record; this file defines the Phase 2 ownership boundary and current interaction surface.

## Current implementation status

**The first local Unreal Engine 5.8 playtest is complete; the One Probe slice is in its evidence-driven embodiment/control pass.**

Implemented:

- production Unreal Engine 5.8 project under `unreal/`;
- engine-independent C++20 authoritative simulation under `src/simulation/`;
- canonical EV-0001 runtime state;
- deterministic fixed-step movement and domain events;
- authoritative position-driven Unreal presentation transform;
- scan start/progress/complete/cancel lifecycle;
- subsystem power allocation and capacity validation;
- stored-energy consumption plus canonical passive generation;
- thermal load, passive cooling, overheat lockout, energy-depletion lockout, and recovery where supported;
- explicit subsystem operational-state hooks;
- capability-driven telemetry/HUD shell;
- shared observable commands for movement, scanning, power, and policy interaction;
- primitive Generation-1 software policy runtime with compute-power gating;
- adjacent-generation evolution generator foundation;
- deterministic runtime-generated Phase-2 test environment, spatial references, visible bootstrap target, and camera orbit/zoom;
- authoritative yaw/pitch/roll attitude plus probe-relative three-axis movement controls, with Unreal rendering the resulting state;
- CMake/CTest and static Unreal source-contract coverage in GitHub Actions.

Still intentionally incomplete:

- real scan-result/discovery content;
- real world-object targeting;
- final propulsion/attitude model;
- rich component health/wear/failure and repair mechanics;
- production save/load implementation;
- final software-programming workspace;
- production Prime Probe mesh/material integration;
- stronger-hardware production validation capture.

These omissions do not block the first local Phase-2 integration run. See `PHASE2_FIRST_RUN_PLAYTEST.md`.

## 1. Repository and ownership boundary

### Unreal project

The production Unreal project lives under `unreal/`. Historical Phase 1 benchmark projects under `prototypes/` remain evidence, not production gameplay code.

### Simulation truth

`src/simulation/` owns mechanical truth and must remain buildable/testable without Unreal dependencies.

Unreal owns:

- presentation;
- local input translation;
- camera;
- HUD rendering;
- temporary integration-environment actors;
- adapter calls into authoritative runtime.

Unreal does **not** independently decide trajectories, scan lifecycle outcomes, energy, heat, subsystem availability, policy effects, or successor capability logic.

### Adapter boundary

`UProbeSimulationAdapter` is the designated Unreal-side boundary into the authoritative One Probe runtime. Presentation classes must not include or call simulation-core implementation directly.

The dependency remains:

```text
Unreal input / HUD / presentation
             |
             v
UProbeSimulationAdapter
             |
             v
ProbeRuntime / SimulationCore
             |
             v
authoritative state + domain events
```

## 2. State channel — simulation to presentation

The current Phase 2 snapshot/read model includes enough state for the first integrated playtest:

| Field group | Current state |
|---|---|
| Identity | probe ID and generation |
| Position / velocity | authoritative metres and m/s; local movement commands are projected through current attitude; Unreal converts position to centimetres once at boundary |
| Attitude | authoritative yaw/pitch/roll degrees; Unreal renders the snapshot as FRotator but does not own orientation truth |
| Mass | total mass present; component breakdown later |
| Energy | stored/capacity/generation plus subsystem allocations and total budget |
| Thermal | current temperature, ambient/passive cooling, operating limit, overheat state |
| Storage | used/capacity scaffold present; inventory mechanics later |
| Sensors | capability/operational state, active target, scan remaining time |
| Computation | operational state, allocated power, primitive policy executor availability |
| Propulsion | operational/capability state plus current velocity command path |
| Thermal control | operational state and allocation |
| Software | active primitive policy ID/rule count/executor state through the runtime read model |

Future component-level detail should extend this model rather than allowing Unreal presentation to create a second source of truth.

## 3. Command channel — presentation/automation to simulation

Every mutating mechanical interaction follows:

> **Command -> validation -> authoritative state transition -> domain event/readback**

Current command surfaces include:

- set absolute velocity (including full stop);
- adjust probe-relative velocity;
- adjust yaw/pitch/roll attitude;
- start scan;
- cancel scan;
- allocate subsystem power;
- install/clear the temporary Generation-1 policy through the runtime control surface.

Manual and automated actions must converge on the same authoritative mechanics.

For example, a manual power change and a software-policy power change both resolve through `SimulationCore::allocate_power()` rather than maintaining separate UI-owned and automation-owned values.

Command rejection must remain observable and explainable to the player.

## 4. Generation-1 software policy boundary

Generation 1 deliberately starts with crude automation:

- one active policy;
- at most two simple rules;
- scalar energy/temperature conditions;
- power-allocation actions;
- policy execution only while computation is operational and receives at least 25 W.

This is proof of architecture, not the final programming language.

The physical-computation rule is mandatory: software sophistication is constrained by installed compute hardware and available power. Later generations may gain richer state, planning depth, concurrency, simulation, and automation, but should do so through evolved capability rather than free UI privilege.

## 5. Capability-driven interface rule

There is no universal player ability list.

The intended dependency is:

```text
installed hardware/software
    -> capability descriptors
    -> telemetry + authoritative commands
    -> manual HUD controls
    -> automation/script surfaces
```

A descendant that gains new hardware may expose new commands and information unavailable to its parent. Unknown future capabilities must be addable without redesigning the whole HUD or simulation boundary.

## 6. Fixed-step simulation and units

The Unreal adapter advances authoritative runtime through a fixed-step accumulator. Variable render-frame timing does not directly author mechanical state.

Canonical simulation units remain engine-independent, including metres, seconds/ticks, kelvin, joules/watts, and kilograms.

Unreal presentation uses centimetres. Unit conversion occurs at the adapter/presentation boundary and must not leak into authoritative simulation state.

## 7. Phase-2 test environment

`AEverwardGameMode` creates a deterministic temporary integration environment at runtime so a clean checkout can produce the same first-run setup without an authored `.umap`.

The temporary environment includes:

- deterministic player start;
- visible target `phase2-test-target-001` about 50 m along +X;
- six fixed spatial-reference markers;
- temporary lighting;
- camera orbit/zoom support;
- temporary probe-relative three-axis velocity trims and yaw/pitch/roll attitude controls.

This environment is test scaffolding. It must not accumulate Phase 3 astronomy, resource truth, collision gameplay, or final production content.

## 8. Persistence

Canonical campaign saves remain simulation-owned, explicitly versioned data per ADR-0004 and `SAVE_FORMAT.md`. Unreal object serialization is not the canonical campaign-save architecture.

Production save/load remains pending and does not block the first integrated Phase-2 run.

## 9. Residual rendering risk

Phase 1 hardware evidence showed meaningful GPU risk on weaker integrated hardware. That remains tracked but does not justify lowering the cinematic scientific-realism product target.

Phase 2 should stay visually minimal enough to test embodiment and controls while production visual work begins from the canonical Prime Probe references in parallel.

## 10. Phase 2 gate and next evidence

The roadmap-required interaction set is now represented in the current slice:

- observe;
- scan;
- move;
- inspect systems;
- manage power;
- alter a basic software policy.

The first structured run is recorded in `PHASE2_FIRST_RUN_FINDINGS_2026-08-24.md`. The next step is a focused local Unreal Engine 5.8 retest of authoritative attitude, probe-relative translation, visual rotation, and full stop before moving to the persistent all-subsystem telemetry slice.

Passing an integration protocol proves a slice works end-to-end. The actual Phase 2 product gate remains:

> **Simply existing as the probe is compelling.**

That requires direct playtest iteration.

## See also

- `PROJECT_STATUS.md` — current continuation point;
- `PHASE2_FIRST_RUN_ENVIRONMENT.md` — temporary integration environment;
- `PHASE2_FIRST_RUN_PLAYTEST.md` — local evidence protocol;
- `HUD_AND_CONTROL_INTERFACE.md` — HUD/control architecture;
- `GEN1_SOFTWARE_POLICY.md` — primitive automation design;
- `GENERATION_ORIGIN_AND_EVOLUTION.md` — Generation-1 origin/open-ended evolution doctrine;
- `ADJACENT_GENERATION_EVOLUTION.md` — successor-option generator architecture;
- `ROADMAP.md` — phase ordering and gates;
- `ARCHITECTURE.md` — global logical layers/boundaries;
- `SAVE_FORMAT.md` — persistence contract.
