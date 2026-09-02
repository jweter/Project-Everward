# Phase 2 — Target Selection and Range Telemetry Product Reality Test

Status: implemented in PR; local Unreal Engine 5.8 Product Reality verification required.

## Purpose

Wire **Slice 7**'s first foundation sub-slice (`target_selection.hpp`,
already merged, engine-independent nearest-target selection and
range/closing-speed telemetry) into `ProbeRuntime`'s authoritative state,
the Unreal adapter, and a minimal HUD/input surface. Until this pass, that
math existed but was not reachable from `ProbeRuntime`, the adapter, or any
Unreal input/HUD path, so there was no player-visible result. This does not
attempt selection UI beyond "nearest in range," approach/reach/grasp
mechanics, or anything else later in Slice 7's
`detect -> select -> approach -> scan -> reach -> grasp -> move -> release`
loop — those remain explicitly out of scope for this sub-slice.

This does not assume the still-Product-Reality-pending contact/manipulator
collision behavior is correct and does not change it: selection only reads
probe position/velocity and the existing registered-body list, the same
data the swept contact solver already consumes, and mutates no
authoritative state, so it qualifies for the parallel-safe lane.

## Authoritative mechanics

`ProbeRuntime` (`software_policy.hpp`) now owns selection state alongside
the same `static_bodies_` registry the swept contact solver reads:

- `select_nearest_target(max_selection_range_m)` selects the nearest
  registered body within range, or clears the selection when none qualify;
- `select_target(body_id)` selects an explicit id, or clears the selection
  when it is empty or not currently registered (fail-closed, matching this
  codebase's other registered-body lookups);
- `clear_target_selection()` clears it unconditionally;
- `selected_target_status()` recomputes range/closing speed from the live
  registry and pose on every call, so a since-deregistered selection reports
  `has_selection = false` without requiring a separate mutating call to
  notice.

`DamageAwareProbeRuntime` forwards all four rather than duplicating the
registry or the math. `everward_software_policy_tests` and
`everward_impact_damage_tests` cover nearest/explicit/invalid selection,
range/closing-speed correctness, and the deregistration fail-closed case.

## Unreal surface added

`UProbeSimulationAdapter` (new `ProbeTargetSelectionBridge.cpp`, mirroring
the existing `ProbeMiningBridge.cpp` split):

- `GetSelectedTargetStatus()` — `bHasSelection`, `TargetId`,
  `SurfaceRangeMeters`, `ClosingSpeedMetersPerSecond`;
- `CommandSelectNearestTarget(MaxSelectionRangeMeters)`;
- `CommandSelectTarget(TargetId)`;
- `CommandClearTargetSelection()`.

Input: `T` calls `CommandSelectNearestTarget` with the player controller's
`TargetSelectionRangeMeters` (default 500 m). `CommandSelectTarget` /
`CommandClearTargetSelection` are exposed on the adapter for later Slice 7
sub-slices (e.g. a multi-target cycle) but are not yet bound to input.

HUD: the always-visible telemetry panel gains a `TARGET` row between
`VELOCITY` and `SIM`, reading either
`TARGET  <id> // <range> M // CLOSING <rate> M/S` or a muted
`TARGET  NONE SELECTED // [T] SELECT NEAREST` prompt. The `F1` controls
reference's "MANIPULATOR + MINING" column documents `T`.

## Local UE 5.8 test script

1. Launch the exact CI-green build.
2. Confirm the telemetry panel's new `TARGET` row reads
   `NONE SELECTED // [T] SELECT NEAREST` at spawn, and that it does not
   overlap or push the `SIM` row or the `PORT`/`STBD` arm rows off the
   panel.
3. Fly toward the registered physical scan target (the same body the
   scan/mining flow already uses) until it is within 500 m, then press `T`.
   Confirm the row switches to `TARGET  phase2-test-target-001 // <range> M
   // CLOSING <rate> M/S` and a command-accepted banner appears.
4. Continue approaching. Confirm the reported range decreases and the
   closing-speed reading stays plausible (positive while closing, near zero
   at a matched approach speed) without needing to stop.
5. Reverse away from the target. Confirm the range increases and closing
   speed goes negative (or the reading otherwise clearly reads as
   "opening" rather than "closing").
6. Press `T` again while far enough that no body is within 500 m. Confirm a
   "COMMAND REJECTED" banner referencing "no physical target within 500 m"
   appears and the row reverts to the muted "NONE SELECTED" prompt rather
   than keeping a stale reading.
7. Open the `F1` controls reference and confirm the "MANIPULATOR + MINING"
   column lists `T — SELECT NEAREST PHYSICAL TARGET`.
8. Confirm existing flight, scan, manipulator, mining, and contact/damage
   behavior is unaffected by any of the above.

## Acceptance questions

- Is it immediately obvious from the HUD alone that pressing `T` selects a
  nearby object and shows live range/closing information for it?
- Does "no target in range" read as a clear rejection rather than a silent
  no-op or a fabricated reading?
- Does the new row fit the existing telemetry panel without crowding or
  overlapping the rows below it?

## Explicitly not complete in this slice

- No visual selection indicator (e.g. a world-space marker or reticle) on
  the selected target — only the HUD text row. (A material-retint indicator
  on the target's own mesh was added afterward; see
  `PHASE2_TARGET_VISUAL_INDICATOR_TEST.md`.)
- No cycling between multiple in-range targets; only nearest-in-range
  selection and explicit-id selection (not yet bound to input) exist.
- No approach/reach/grasp mechanics — those remain later Slice 7 sub-slices
  per `PHASE2_VERTICAL_SLICE_PLAN.md`.
- No change to contact, damage, or manipulator collision behavior, which
  remain Product Reality pending from prior slices independent of this one.
