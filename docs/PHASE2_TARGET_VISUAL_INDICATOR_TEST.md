# Phase 2 — Target Visual Selection Indicator Product Reality Test

## Scope

`PHASE2_TARGET_SELECTION_TEST.md` explicitly left "no visual selection
indicator (e.g. a world-space marker or reticle) on the selected target"
out of scope, and `PHASE2_VERTICAL_SLICE_PLAN.md`'s Slice 7 status has since
named it, alongside approach/reach/grasp mechanics, as the remaining gap
before the `detect -> select` half of the slice's loop reads as complete.
This pass closes the visual-indicator half of that gap, presentation-only:
it does not touch approach, reach, or grasp.

The production path is:

`ProbeRuntime::selected_target_status() (already authoritative, unchanged)
-> UProbeSimulationAdapter::GetSelectedTargetStatus() (already exposed,
unchanged) -> AEverwardPhase2TestEnvironment::RefreshTargetSelectionHighlight()
(new) -> the registered target's own mesh material`

No new simulation state, adapter method, or input binding was added. The
registered physical body already renders as a distinct mesh
(`AEverwardPhase2TestEnvironment::ScanTargetMesh`); this pass retints that
mesh's existing dynamic material in place instead of inventing a second,
Unreal-owned notion of "selected" or a separate reticle actor. This
qualifies for the parallel-safe lane the same way the underlying selection
telemetry did: it reads already-authoritative state through the existing
adapter boundary, mutates no authoritative state, and does not assume any
still-pending contact/collision/damage behavior is correct.

## Behavior

- While `GetSelectedTargetStatus().bHasSelection` is true and its `TargetId`
  matches the registered physical body's id, that body's mesh retints from
  its default regolith-rock color/roughness to a bright cyan, more metallic
  highlight.
- The mesh reverts to its default material the moment the selection clears
  or moves to a different id (fail-closed the same way the HUD `TARGET` row
  already does — no stale highlight survives a deregistration or
  out-of-range clear).
- The material is only rewritten when the selected/not-selected outcome
  actually changes, not on every tick, so this adds no meaningful per-frame
  cost.
- The existing `TARGET` HUD row, `F1` controls reference, and all mining,
  contact, and manipulator behavior are unchanged.

## CI-verifiable acceptance

`tools/test_phase2_target_selection_surface.py`'s
`test_environment_highlights_selected_target_from_authoritative_status`
confirms `AEverwardPhase2TestEnvironment` reads
`GetSelectedTargetStatus()`/`bHasSelection`/`TargetId` and drives the
existing `ScanTargetDynamicMaterial` from it, and that no
`everward/simulation` header was pulled into Unreal-side code to do so.

No Unreal Editor/UBT build was available in this sandbox to compile-verify
this change. It follows the exact `UMaterialInstanceDynamic`/
`SetVectorParameterValue` pattern `ApplyEnvironmentMaterialScaffold` already
uses and compiles elsewhere in this file; the next local Unreal pass should
confirm the project still compiles under UBT before relying on this
further, the same caveat prior parallel-safe passes in this repository have
recorded when local UBT was unavailable.

## Local Unreal Product Reality acceptance

1. Launch the exact CI-green build and enter PIE.
2. Confirm the registered physical target renders in its normal
   regolith-rock color with no selection made.
3. Press `T` to select it. Confirm the mesh itself visibly retints to a
   brighter cyan/metallic highlight at the same moment the `TARGET` HUD row
   switches from "NONE SELECTED" to the selected id/range/closing-speed
   reading — the two should read as one consistent selection event, not two
   independent ones.
4. Retreat beyond the selection range and press `T` again (or otherwise
   cause the selection to clear). Confirm the mesh reverts to its default
   color at the same moment the HUD row reverts to "NONE SELECTED", with no
   stale highlight left behind.
5. Confirm the highlight does not affect the separate mining-status label
   text/color above the body, and that scan, mining, manipulator, contact,
   and full-stop controls are otherwise unaffected.
6. Record any discrepancy (wrong color, desync from the HUD row, stale
   highlight, or a build/compile failure) as Product Reality evidence.

## Explicitly not complete in this pass

- No reticle, outline shader, or screen-space marker — the indicator is a
  material retint on the target's own mesh.
- No indicator for a hypothetical second/third target — the current Phase-2
  scene still has only one registered physical body; a multi-target scene
  (Slice 8) is required before a per-target indicator can be visually
  distinguished from "the only body in the scene changed color."
- No approach, reach, or grasp mechanics — those remain later Slice 7
  sub-slices per `PHASE2_VERTICAL_SLICE_PLAN.md`.

## Status

Implemented in the parallel-safe lane; Product Reality pending.
