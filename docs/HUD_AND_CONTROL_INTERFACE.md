# HUD and Control Interface

Everward's HUD is the probe's operating interface. It must make the machine understandable and controllable without permanently covering the cinematic view of space.

## Core rule

> Every controllable installed capability must be discoverable from the HUD, but not every control should be visible simultaneously.

The interface answers three questions quickly:

1. What state am I in?
2. What needs my attention?
3. What can I do right now?

## Information hierarchy

### Always visible

Only high-value state needed during ordinary operation:

- probe identity and generation;
- energy reserve;
- current power use versus capacity;
- thermal state;
- storage pressure;
- velocity/motion state;
- active high-value operations such as scanning;
- critical alerts.

### Contextual

Controls and telemetry for the selected installed system. The system surface should expand only when the player asks for it or when a task makes it relevant.

### On demand

Engineering detail, diagnostics, logs, component health, power routing, detailed sensor configuration, and other dense information belong in deeper views rather than the permanent HUD.

### Programming workspace

Scripts, policies, routines, priorities, conditions, automation state, and behavior editing use a deliberate expanded workspace. Programming is part of controlling the probe, not a separate game mode with a disconnected command model.

### Emergency escalation

Serious conditions such as thermal lockout, energy depletion, collision danger, or component failure may promote themselves into the visible HUD even when their subsystem panel is closed.

## Capability-driven interface

The HUD must be generated from the current probe's actual installed hardware and software capabilities. There is no universal ability list shared by every generation.

A probe with optical sensors and propulsion exposes those systems. A descendant that adds infrared sensing, a laser, a better drill, fabrication equipment, or a new manipulator exposes the corresponding new telemetry and controls. A parent without that hardware does not receive those commands.

The intended dependency is:

```text
installed hardware/software
    -> capabilities
    -> authoritative commands + telemetry
    -> manual control surface
    -> automation/script API
    -> contextual HUD presentation
```

## Shared authoritative command boundary

Manual control and automation must converge on the same authoritative command layer. Clicking a control and issuing the equivalent script instruction must not create two different mechanical implementations.

The Phase-2 boundary is `UProbeSimulationAdapter`, backed by the engine-independent `ProbeRuntime` and `SimulationCore`:

```text
manual input / HUD
        |
        v
UProbeSimulationAdapter command method
        |
        v
ProbeRuntime / SimulationCore authoritative command
        |
        v
state + domain events
        |
        v
adapter telemetry / HUD
```

Software policy evaluation lives in the engine-independent runtime. Matching policy actions call the same public `SimulationCore::allocate_power()` command used by manual control; they do not write separate automation-only power state.

Every submitted manual command produces an observable command result containing:

- sequence number;
- command identifier;
- accepted/rejected state;
- human-readable reason/detail.

This makes command failure explainable instead of silently ignored. Thermal lockout, energy depletion, failed hardware, invalid scan lifecycle state, and power-budget violations can therefore be surfaced to the player and later to automation diagnostics through the same model.

## Teaching the machine

Long term, manual operation should help players learn and create automation. A sequence performed manually may become the basis for a reusable routine: target, approach, stabilize, deploy tool, act, retract, depart. The player's progression therefore moves naturally from operating individual mechanisms toward designing behavior.

The first implemented step is deliberately primitive. Generation 1 can install one two-rule policy and must physically power its computation subsystem for the policy executor to run. This gives the player direct evidence that programming is part of the machine, not a detached magic menu.

## Generation-1 presentation

The first probe should expose a deliberately modest, somewhat clunky interface consistent with its limited hardware and computation. Its HUD should be understandable but not magically sophisticated. Later computation and hardware generations may support richer analysis, stronger automation, more concurrent state, and more advanced control surfaces.

Generation-1 software-policy constraints are documented in `GEN1_SOFTWARE_POLICY.md`.

## Current Phase-2 implementation

The current production implementation provides:

- compact authoritative telemetry;
- emergency energy/thermal alerts;
- active scan progress promotion;
- a collapsed-by-default systems panel;
- installed capability discovery;
- contextual capability state, power, manual-control availability, and automation availability;
- shared observable command methods for velocity, scan start, scan cancel, and power allocation;
- command acceptance/rejection feedback in the HUD;
- one primitive policy slot with a maximum of two simple rules;
- a 25 W Generation-1 computation requirement for policy execution;
- policy installation/clear controls and policy/executor status under Computation;
- a reproducible source-built Phase-2 environment with a visible bootstrap scan target and spatial movement references;
- mouse orbit/zoom for presentation inspection;
- authoritative yaw/pitch/roll attitude state and observable attitude-trim commands;
- probe-relative forward/lateral/vertical velocity trims projected through authoritative attitude;
- keyboard navigation and manual controls for the temporary engineering shell.

Temporary engineering-shell controls:

- `Tab`: open/close systems;
- `[` / `]`: select installed system;
- `Page Up` / `Page Down`: adjust selected subsystem power;
- mouse X / Y: orbit camera yaw / pitch;
- mouse wheel: zoom camera;
- Sensors: `Enter` scans the visible `phase2-test-target-001` target and `Backspace` cancels it;
- Propulsion:
  - `W` / `S`: local forward / reverse velocity trim;
  - `D` / `A`: local right / left velocity trim;
  - `E` / `Q`: local up / down velocity trim;
  - `J` / `L`: yaw left / right;
  - `I` / `K`: pitch up / down;
  - `U` / `O`: roll left / right;
  - `Space`: zero world velocity while preserving attitude;
  - Up / Down remain local forward / reverse aliases;
- Computation: `Enter` installs the temporary `gen1_basic_survival` policy and `Backspace` clears it.

The bootstrap scan target exists only because Phase 3 world-object targeting does not exist yet. It must be replaced by real selected-target state when that system arrives rather than becoming permanent gameplay content.

The Basic Survival policy uses an intentionally aggressive 60% energy threshold so its behavior is immediately visible during Phase-2 integration testing. That value is test scaffolding, not final balance.

This remains a foundation, not the final visual styling, camera design, rigid-body flight model, or control mapping. The first local Unreal Engine 5.8 run is recorded in `PHASE2_FIRST_RUN_FINDINGS_2026-08-24.md`. The next local pass should verify that attitude-driven movement materially improves spacecraft embodiment while preserving the intentionally primitive Generation-1 response and useful full-stop behavior.
