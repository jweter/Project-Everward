# Phase 2 First-Run Environment

This document defines the temporary, reproducible integration environment used to test **Phase 2 — One Probe** before Phase 3 introduces an authored star system and real world-object targeting.

The purpose is not visual polish. The purpose is to make the embodied probe testable in a repeatable way on any machine that opens the production Unreal project.

## Why this exists

A useful first run must let the player perceive that the authoritative probe is actually moving and interacting with something visible. A blank Unreal map can technically run the simulation while giving almost no perceptual evidence that movement, camera control, or scanning are working.

For that reason, `AEverwardGameMode` creates the temporary Phase-2 environment at runtime rather than depending on an editor-authored `.umap`.

This keeps the test setup:

- version-controlled as C++ source;
- reproducible from a clean checkout;
- independent of whichever editor map happened to be open;
- explicitly temporary and removable once Phase 3 owns real environment content.

## Deterministic spawn

`AEverwardGameMode::InitGame()` creates a dedicated `APlayerStart` at the world origin and uses it through `ChoosePlayerStart_Implementation()`.

The canonical EV-0001 presentation therefore begins from a known location for every Phase-2 integration run.

The simulation remains authoritative for probe motion after spawn. The player-start actor establishes presentation/bootstrap location only; it does not become simulation truth.

## Visible bootstrap target

`AEverwardPhase2TestEnvironment` creates one visible target at **50 m along +X** from the initial probe position.

Canonical temporary target identifier:

```text
phase2-test-target-001
```

The target is labeled in-world:

```text
PHASE-2 TARGET // SCAN-001
```

The Sensors `Enter` action submits this same identifier to the existing authoritative scan command.

This is not a real targeting system. It deliberately closes only the perceptual loop required for Phase 2:

```text
see target
    -> operate sensors
    -> start authoritative scan
    -> observe scan progress / completion
```

Phase 3 must replace this bootstrap target with real selected world-object state rather than extending the temporary identifier into production content.

## Spatial reference field

Six fixed reference markers surround the initial path between the probe and target. Their purpose is parallax and motion perception.

They are not resources, obstacles, asteroids, or gameplay objects. They make it easy to answer during a test:

- did the probe move?;
- in which direction?;
- does motion feel appropriately slow and mechanical?;
- does the third-person camera communicate scale and orientation?

The environment also owns a temporary point light so visibility does not depend on editor-map lighting.

## Camera controls

The first-run camera is intentionally simple:

- mouse X: orbit yaw;
- mouse Y: orbit pitch;
- mouse wheel: zoom in/out;
- zoom is clamped to a practical third-person inspection range.

Camera motion is presentation only. It never changes authoritative probe position, velocity, or simulation state.

## Temporary propulsion controls

With the systems panel open and **Propulsion** selected:

- `W` / `S`: +X / -X velocity trim;
- `D` / `A`: +Y / -Y velocity trim;
- `E` / `Q`: +Z / -Z velocity trim;
- `Space`: command zero velocity;
- Up / Down remain aliases for +X / -X from the earlier engineering shell.

Each trim changes commanded velocity by only **1 m/s**. This is intentionally crude. It gives the first probe useful three-axis translation without pretending Phase 2 already has a final thruster/attitude-flight model.

Every translation action still calls `UProbeSimulationAdapter::CommandSetVelocityMetersPerSecond()` and therefore preserves the existing authoritative command boundary.

## What this environment is not

It is not:

- the first star system;
- final navigation or flight controls;
- final camera behavior;
- real target selection;
- scan-result science content;
- collision gameplay;
- final lighting or art direction;
- the Prime Probe production mesh.

Those systems should not accrete accidentally into this temporary actor.

## Exit condition

This environment has done its job when a clean local Unreal Engine 5.8 run can reliably demonstrate:

1. EV-0001 spawns in a known location;
2. the player can orbit and zoom around the probe;
3. spatial references make authoritative movement visually obvious;
4. the player can translate along all three axes through the shared command path;
5. the visible bootstrap target corresponds to the target used by the authoritative scan command;
6. HUD, power, scan, and Generation-1 policy behavior can all be exercised in the same run.

The environment definition and the evidence protocol are intentionally separate. Execute `PHASE2_FIRST_RUN_PLAYTEST.md` for the actual local test and record the result using `playtests/phase2/first_run_observation.template.json`.
