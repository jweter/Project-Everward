# Phase 2 Slice 8 — Dedicated Zero-G Space Environment Acceptance Contract

Status: dedicated environment implementation added; Product Reality pending

## Purpose

This contract turns the existing Slice 8 requirements in `PHASE2_VERTICAL_SLICE_PLAN.md` into a concrete, testable Product Reality gate without changing authoritative simulation mechanics.

The dedicated zero-g environment is separate from the current ground sandbox. Launch it through the normal Phase-2 game mode with the explicit URL option `?ZeroG=1`; without that option the existing Phase-2 environment remains the default. It must exercise existing probe flight, target selection, scanning, contact, manipulator, and telemetry behavior in a free six-degree-of-freedom context. This document does not authorize new movement physics, assisted approach, orbital mechanics, or planetary gravity.

## Required scene properties

The Slice 8 environment must provide all of the already-approved minimums:

- no flat-ground assumption;
- a distant star or directional light reference;
- an asteroid or small-moon reference body;
- multiple registered physical targets at different ranges;
- useful navigation/reference markers;
- free six-degree-of-freedom context;
- collision/contact with local registered bodies.

The existing `phase2-test-target-001/002/003` registered-body conventions may be reused where appropriate. Their simulation state remains authoritative through `UProbeSimulationAdapter`; Unreal presentation must not create a second mechanical truth.

## Portable verification before Product Reality

Portable CI must prove, where source-level verification is practical:

1. the dedicated environment actor/scene surface is separate from the existing ground test environment;
2. no code path disables or bypasses the authoritative simulation adapter;
3. all physical targets used for selection/contact are registered through the existing simulation-body registration path;
4. target IDs are stable and unique;
5. no new commercial or third-party assets are introduced without provenance;
6. existing simulation/foundation tests remain green.

Portable CI cannot prove visual scale, spatial readability, camera feel, six-DOF legibility, or whether the space scene is compelling. Those remain Product Reality.

## Local UE 5.8 Product Reality sequence

On the exact tested build:

1. launch the dedicated zero-g environment and confirm no ground-plane assumption is visible or required for normal operation;
2. confirm EV-0001 spawns with a readable distant light/star reference and at least one asteroid/small-moon-scale reference body;
3. translate forward/back/left/right/up/down and rotate through yaw/pitch/roll without relying on a ground horizon;
4. use the existing target-selection/cycling control and verify multiple targets at different ranges can be selected in deterministic nearest-to-farthest order;
5. verify the HUD range/closing-opening telemetry remains understandable without a ground reference;
6. scan at least one target and confirm scan state/results remain authoritative and legible;
7. approach a registered body slowly and verify collision/contact prevents ghosting;
8. perform a glancing contact and confirm tangential motion remains plausible;
9. deploy and articulate both manipulators and verify geometry/control remains legible in free space;
10. when a target is reachable, exercise grasp/move/release using the existing Slice 7 commands without introducing a scene-specific interaction path;
11. confirm the camera can frame the probe, selected target, and reference body without requiring terrain;
12. confirm no regression in power, subsystem-state, storage, mining, save/load, or global feedback surfaces used by the current Phase-2 build.

## Pass criteria

Slice 8 Product Reality is VERIFIED only when:

- the dedicated scene is visibly and operationally distinct from the ground sandbox;
- the probe can be understood and controlled in six degrees of freedom;
- multiple physical targets, selection, scan, collision/contact, and manipulator interaction work through existing authoritative mechanics;
- no scene-specific shortcut duplicates simulation truth;
- the exact build/commit and observations are recorded.

A portable-CI green result alone means **IMPLEMENTED / PRODUCT_REALITY_PENDING**, not Slice 8 complete.

## Failure handling

Any failure in collision/contact, target registration, or adapter authority is a correctness blocker and outranks later Slice 8 presentation polish. Visual/readability failures remain Product Reality debt and should generate the smallest evidence-driven follow-up slice.

Refs: `docs/CURRENT_EXECUTION_PLAN.md`, `docs/PHASE2_VERTICAL_SLICE_PLAN.md`, `docs/PROJECT_STATUS.md`, issues #159, #161, #162, #163, #166, #168.
