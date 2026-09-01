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

The current Phase-2 scene contains one registered physical resource body, so this build can verify that `T` enters the cycling path and selects that body, that the TARGET telemetry is authoritative, and that no regressions occur in existing selection/mining/contact behavior. A visible multi-target wraparound demonstration requires a scene with multiple registered physical bodies; Slice 8's dedicated zero-g test environment is already planned to provide multiple physical targets at different ranges.

For this build:

1. Launch the Phase-2 Unreal scene and enter PIE.
2. Press `T`. Confirm the TARGET row selects the registered physical body and reports plausible surface range and closing speed.
3. Move relative to the target and confirm range/closing-speed telemetry updates rather than remaining cached.
4. Press `T` repeatedly. With the current single registered target, confirm selection remains deterministic and the command succeeds without creating a parallel Unreal-owned selection.
5. Confirm scan, mining, manipulator, contact, and full-stop controls still behave as before.
6. Record any discrepancy as Product Reality evidence; a CI-green result does not by itself mark Slice 7 complete.

## Status

Implemented in the parallel-safe lane; Product Reality pending. This advances target selection toward the documented Slice 7 loop but does not claim approach, reach, grasp, move, or release complete.
