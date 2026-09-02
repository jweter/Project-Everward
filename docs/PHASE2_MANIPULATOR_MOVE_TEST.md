# Phase 2 — Manipulator Move Product Reality Test

## Scope

`PHASE2_VERTICAL_SLICE_PLAN.md`'s Slice 7 minimum interactions run
`detect -> select -> approach -> scan -> reach -> grasp -> move -> release`.
`grasp` (`PHASE2_MANIPULATOR_GRASP_TEST.md`) already introduced the one new
piece of authoritative state (`ManipulatorArmState::grasped_target_body_id`)
but explicitly did not move anything: a grasped body's registered position
stayed exactly where it was before the grasp. This pass wires the
already-landed read-only `manipulator_move.hpp` math (wrist-world-position
telemetry for a currently grasped target) into three places so a held body
actually follows the arm:

1. `everward::simulation::ProbeRuntime::update_static_sphere_body_position()`
   (new, in `software_policy.hpp`, forwarded by
   `DamageAwareProbeRuntime`) — the single authoritative mutation point for a
   registered body's `center_m`. Every other reader of the registered-body
   list (contact, target selection, reach) already reads `center_m` directly,
   so this is the only place a "current position" needs to be written for
   all of them to agree.
2. `UProbeSimulationAdapter::TickComponent()` — after
   `Manipulators->advance(FixedStepSeconds)` each fixed step, for each arm
   `grasped_target_position()` is queried; a non-empty result is written back
   through `Core->update_static_sphere_body_position()`. An empty result
   (arm holds nothing) leaves that tick's registered bodies untouched.
3. `AEverwardPhase2TestEnvironment::RefreshScanTargetPosition()` (new) —
   reads the registered body's current position back through the new
   `UProbeSimulationAdapter::GetStaticBodyPositionMeters()` accessor every
   tick and mirrors it onto the existing `ScanTargetMesh`/`ScanTargetLabel`
   components, exactly the way `RefreshTargetSelectionHighlight()` already
   mirrors authoritative selection state onto the same mesh's material.

No new grasp/reach/release rule is introduced, and no second position
formula is invented anywhere in this chain — every step reuses
`manipulator_move.hpp`'s existing `grasped_target_position()` (unchanged) or
reads straight from `Core->static_bodies()`. This qualifies for the
parallel-safe lane the same way reach and grasp did: it composes only
already-authoritative state through the existing simulation/adapter
boundary, does not assume any still-pending contact/collision/damage
behavior is correct, and the presentation step is a reversible mirror of
already-authoritative position data rather than a new physics/animation
system.

## Behavior

- While an arm holds a target (`HOLDING <id>` on its status line), that
  target's registered body position updates every fixed simulation step to
  the holding arm's current wrist world position — translating and rotating
  with the probe exactly as `manipulator_move_tests.cpp`'s
  `test_grasped_target_follows_probe_translation` already proves for the
  underlying math.
- The visible `SCAN-001` mesh and its label now move to match: flying the
  probe (or articulating the holding arm's joints) while holding the target
  visibly drags the mesh along with the wrist instead of leaving it at its
  original registered position.
- Releasing (`F`) leaves the body at wherever it last was while held — there
  is no snap-back to the original registration point, matching plain
  physical intuition (letting go leaves an object where it currently is).
- Target-selection telemetry (`TARGET` row: surface range, closing speed)
  and reach telemetry (`REACH` row) both automatically reflect the body's
  now-current position on every read, since they already read `center_m`
  from the same registered-body list this pass mutates; neither was changed
  by this pass.
- No new grasp/release rule, no new collision consequence, and no automatic
  "place" or "drop toward a location" targeting: the body's position is
  exactly the wrist's position, full stop, while held.

## CI-verifiable acceptance

- `src/simulation/tests/manipulator_move_tests.cpp`
  (`everward_manipulator_move_tests` in `src/simulation/CMakeLists.txt`)
  now also covers: `update_static_sphere_body_position()` moves the matching
  registered body's `center_m` and nothing else about its identity; calling
  it with an unregistered id is a no-op rather than throwing or fabricating
  a registration; and an end-to-end scenario that grasps a body, computes
  its new wrist-following position via the existing
  `grasped_target_position()`, writes it through
  `update_static_sphere_body_position()`, and confirms
  `selected_target_status()`/`static_bodies()` both report the moved
  position rather than the original registration point.
- `tools/test_phase2_manipulator_move_surface.py` confirms
  `ProbeRuntime`/`DamageAwareProbeRuntime` expose the single mutation point
  and it fails closed on an unregistered id, `TickComponent` calls
  `grasped_target_position()` and feeds a non-empty result into
  `Core->update_static_sphere_body_position()` after
  `Manipulators->advance()` (ordering asserted), the adapter exposes
  `GetStaticBodyPositionMeters()` reading straight from
  `Core->static_bodies()`, and the test environment's
  `RefreshScanTargetPosition()` mirrors that position onto `ScanTargetMesh`
  without introducing a second Unreal-owned motion system.

No Unreal Editor/UBT build was available in this sandbox to compile-verify
`ProbeSimulationAdapter.h`/`.cpp` or `EverwardPhase2TestEnvironment.h`/`.cpp`.
The adapter change follows the exact accessor pattern
`GetManipulatorReachStatus()`/`GetSelectedTargetStatus()` already use and
compile elsewhere in those files; the environment change follows the exact
`ResolvePlayerAdapter()`/component-mutation pattern
`RefreshTargetSelectionHighlight()` already uses and compiles. The next
local Unreal pass should specifically confirm the project still compiles
under UBT before relying on this further, the same caveat prior
parallel-safe passes in this repository have recorded when local UBT was
unavailable.

## Local Unreal Product Reality acceptance

1. Launch the exact CI-green build and enter PIE.
2. Deploy an arm, select the registered physical target (`T`), approach
   until REACH reads "IN REACH", and grasp it (`F`). Confirm `HOLDING`
   appears as in `PHASE2_MANIPULATOR_GRASP_TEST.md`.
3. Translate the probe a short distance and confirm the `SCAN-001` mesh and
   its label visibly move together with the probe/arm rather than staying
   at their original world position.
4. With the arm still holding the target, open the manipulator page (`M`)
   and nudge that arm's joints (`,`/`.`). Confirm the mesh visibly tracks
   the wrist's motion as the arm articulates, not only the probe's own
   translation.
5. Confirm the `TARGET` row's surface range and the `REACH` row both keep
   reading correctly (in particular, staying "IN REACH") as the held body
   moves with the arm, since both now derive from the same authoritative
   position this pass updates.
6. Release (`F`). Confirm the mesh stays at its current (moved) position
   rather than snapping back to its original spawn location.
7. Confirm this pass has not changed deploy/stow, joint articulation, tool
   attach/detach, grasp's own proximity gate, target selection/cycling,
   contact, or damage behavior.
8. Record any discrepancy (mesh not following the wrist, mesh following with
   a visible lag/offset, `TARGET`/`REACH` rows disagreeing with the visible
   mesh position, a stale position after release, or a build/compile
   failure) as Product Reality evidence.

## Explicitly not complete in this pass

- No "place" or "drop toward a target location" mechanic: the body simply
  equals the wrist position while held.
- No re-collision of the moved body against the probe hull or other bodies
  while carried (the existing arm/hull and arm/environment collision guards
  are unchanged and unrelated to this read-side telemetry consumer).
- No consequence for releasing over empty space, another body, or the
  original scan/mining flow beyond the body remaining wherever it was
  released.
- Slice 7 is not complete: `release`-with-consequence still does not exist,
  and the completion gate for the whole slice remains the local Product
  Reality pass in `PROJECT_STATUS.md`.

## Status

Implemented in the parallel-safe lane; Product Reality pending. Does not by
itself advance Slice 7's completion gate.
