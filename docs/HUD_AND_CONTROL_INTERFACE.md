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

The Phase-2 boundary is `UProbeSimulationAdapter`:

```text
manual input / HUD
        |
        v
UProbeSimulationAdapter command method
        |
        v
SimulationCore authoritative command
        |
        v
state + domain events
        |
        v
adapter telemetry / HUD
```

Future software policies and scripts must call those same adapter command methods or an equivalent lower-level shared command service. They must not duplicate propulsion, scanning, power, manipulation, tool, or other mechanics.

Every submitted command produces an observable command result containing:

- sequence number;
- command identifier;
- accepted/rejected state;
- human-readable reason/detail.

This makes command failure explainable instead of silently ignored. Thermal lockout, energy depletion, failed hardware, invalid scan lifecycle state, and power-budget violations can therefore be surfaced to the player and later to automation diagnostics through the same model.

## Teaching the machine

Long term, manual operation should help players learn and create automation. A sequence performed manually may become the basis for a reusable routine: target, approach, stabilize, deploy tool, act, retract, depart. The player's progression therefore moves naturally from operating individual mechanisms toward designing behavior.

## Generation-1 presentation

The first probe should expose a deliberately modest, somewhat clunky interface consistent with its limited hardware and computation. Its HUD should be understandable but not magically sophisticated. Later computation and hardware generations may support richer analysis, stronger automation, more concurrent state, and more advanced control surfaces.

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
- keyboard navigation and manual controls for the temporary engineering shell.

Temporary engineering-shell controls:

- `Tab`: open/close systems;
- `[` / `]`: select installed system;
- `Page Up` / `Page Down`: adjust selected subsystem power;
- Sensors: `Enter` starts a Phase-2 bootstrap scan and `Backspace` cancels it;
- Propulsion: `Up` / `Down` changes authoritative X velocity and `Space` commands zero velocity.

The bootstrap scan target exists only because Phase 3 world-object targeting does not exist yet. It must be replaced by real selected-target state when that system arrives rather than becoming permanent gameplay content.

This remains a foundation, not final visual styling or final control mapping. The next high-value Phase-2 interaction is the first software-policy/automation primitive using this exact command boundary, followed by local Unreal playtesting and refinement of the Generation-1 embodiment/control feel.
