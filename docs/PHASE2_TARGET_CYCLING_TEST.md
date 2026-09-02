# Phase 2 Target Cycling Product Reality Test

## Scope

This check covers the Slice 7 target-cycling integration built on top of the existing physical target-selection foundation. The deterministic ordering primitive remains engine-independent; Unreal only requests the next target and renders the authoritative selected-target telemetry.

The production path is:

`registered physical bodies -> find_next_selectable_target -> authoritative runtime selection -> UProbeSimulationAdapter -> T input -> TARGET HUD telemetry`

## Expected behavior

- With no current selection, pressing `T` selects the nearest eligible registered physical body within `TargetSelectionRangeMeters`.
- Repeated presses advance through eligible bodies nearest-to-farthest and wrap from the farthest back to the nearest.
- Equal-range ties remain deterministic by body registration order.
- If the selected target becomes stale or moves outside the allowed selection set, the next cycle restarts at the nearest eligible target.
- If no registered target is eligible, selection clears and the command is rejected rather than preserving stale state.
- The existing TARGET HUD row continues to display the selected body id, surface range, and closing speed from authoritative simulation telemetry.
- Target cycling does not alter contact resolution, mining authority, manipulator collision, or automatic approach behavior.

## CI-verifiable acceptance

1. `everward_target_cycle_tests` verifies deterministic ordering, ties, stale/out-of-range recovery, wraparound, and fail-closed behavior in the engine-independent primitive.
2. `everward_target_cycle_runtime_tests` verifies that cycling updates the real `DamageAwareProbeRuntime` selection state and clears it when no body is eligible.
3. `tools/test_phase2_target_selection_surface.py` verifies the operation crosses the simulation/Unreal boundary through `CommandCycleTarget` without re-deriving ordering or range math in Unreal.

## Local Unreal Product Reality acceptance

The Phase-2 scene now registers three physical bodies at different ranges from spawn: the original `SCAN-001` mining/resource body (`phase2-test-target-001`, ~50 m) plus two plain reference targets (`phase2-test-target-002` at ~103 m, `phase2-test-target-003` at ~174 m), each with their own mesh, label, and selection-highlight material. This closes the specific gap this document previously called out: a multi-target nearest-to-farthest wraparound can now actually be demonstrated in this build rather than waiting on Slice 8's separate dedicated zero-g test environment. The two reference targets are not mineable and carry no scan/resource behavior -- they exist solely to give cycling, range telemetry, and the visual selection indicator more than one eligible body.

For this build:

1. Launch the Phase-2 Unreal scene and enter PIE.
2. Press `T`. Confirm the TARGET row selects the nearest registered body (`SCAN-001`) and reports plausible surface range and closing speed, and that its mesh visibly retints to the selected highlight color.
3. Move relative to the target and confirm range/closing-speed telemetry updates rather than remaining cached.
4. Press `T` again and confirm selection advances to the next-nearest reference target (`REF-002`): its mesh retints and `SCAN-001` reverts to its normal look at the same moment.
5. Press `T` again and confirm selection advances to the farthest reference target (`REF-003`), then press `T` a fourth time and confirm it wraps back around to `SCAN-001` rather than stopping or repeating a target out of order.
6. Confirm scan, mining, manipulator, contact, and full-stop controls still behave as before, and that only the selected target's mesh is ever highlighted at once.
7. Record any discrepancy as Product Reality evidence; a CI-green result does not by itself mark Slice 7 complete.

## Status

Implemented in the parallel-safe lane; Product Reality pending. This advances target selection toward the documented Slice 7 loop but does not claim approach, reach, grasp, move, or release complete.
