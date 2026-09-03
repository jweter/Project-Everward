# Phase 2 — Manipulator Release-With-Consequence Product Reality Test

## Scope

`PHASE2_VERTICAL_SLICE_PLAN.md`'s Slice 7 minimum interactions run
`detect -> select -> approach -> scan -> reach -> grasp -> move -> release`.
Every prior sub-slice document
(`PHASE2_MANIPULATOR_GRASP_TEST.md`, `PHASE2_MANIPULATOR_MOVE_TEST.md`)
explicitly named the same outstanding gap: `ManipulatorRig::release_grasp`
unconditionally lets go of the held body wherever it currently is, with no
re-collision against the probe's own hull. This pass closes exactly that one
gap:

- `manipulator_release.hpp` (new) adds `attempt_release_grasped_target()`,
  the same kind of gated wrapper `manipulator_grasp.hpp` already established
  over `begin_grasp` -- it fails closed (no mutation, the arm keeps holding
  the target) whenever the held body's current registered position/radius
  would overlap the probe's own five-sphere `ProbeCompoundCollisionEnvelope`
  (the same envelope `software_policy.hpp`'s swept contact and
  `manipulator_hull_contact.hpp`'s arm/hull guard already use). Only once
  clear does it call `ManipulatorRig::release_grasp`, which stays
  unconditional exactly as documented.
- `UProbeSimulationAdapter::CommandReleaseGraspedTarget` now routes through
  `attempt_release_grasped_target(*Manipulators, *Core, ArmId)` instead of
  calling `Manipulators->release_grasp()` directly, mirroring
  `CommandGraspSelectedTarget`'s existing pattern; a rejected release reports
  "target would collide with probe hull" through the same
  `RecordCommandResult`/`GetLastCommandResult()` global-feedback path every
  other rejected command already uses.

No new grasp/reach/move rule is introduced, and no second placement or
rotation convention is invented: the gate reads the held body's own
already-authoritative `center_m`/`radius_m` from the registered-body list
(kept current every tick by `manipulator_move.hpp`'s existing wiring) and
reuses `rotate_local_contact_offset` to place the hull's local samples in
world space, exactly as `manipulator_pose_intersects_environment` already
does for the arm itself.

## Important integration finding from local verification

The current single-target test scene's `SCAN-001` body is registered with
`AEverwardPhase2TestEnvironment::BootstrapBodyRadiusMeters` = **2.0 m**. A
deterministic sweep of the Port arm's full shoulder/elbow joint range
(`/tmp`-local, not checked in) shows:

- At the default just-deployed pose (all joint angles at 0, immediately
  after grasp), the held target already overlaps the forward hull sample by
  about **1.4 m** -- release fails closed immediately after grasp unless the
  arm is first articulated.
- A release-eligible pose does exist inside the documented joint range
  (shoulder near +90 degrees, elbow near 25 degrees clears the hull by
  roughly 1.2 m), so this is not a deadlock, but it does mean release is
  **not achievable from the default post-grasp pose** with this scene's
  oversized 2.0 m placeholder target.

This is expected given `BootstrapBodyRadiusMeters` was sized for the mining
test target, not for grasp/move/release ergonomics, and is exactly the kind
of consequence this sub-slice is meant to surface rather than hide. It is
not a defect in this pass by itself, but the local Product Reality pass
below must exercise deliberate joint articulation to reach a release-eligible
pose, and any case where no reachable pose clears the hull should be
recorded as a Product Reality finding (candidate follow-up: a smaller/
separate grasp-test target, or reach-envelope/joint-range reconciliation).

## Behavior

- Releasing (`F`) while the held body's current position would overlap the
  probe's own hull envelope is rejected: the arm keeps `HOLDING <id>`, and
  global feedback reports "arm cannot release: target would collide with
  probe hull".
- Releasing once the held body is clear of the hull succeeds exactly as
  before: the mesh stays at its current (moved) position, `HOLDING`
  clears, and `TARGET`/`REACH` telemetry update the same way
  `PHASE2_MANIPULATOR_MOVE_TEST.md` already documents.
- No change to deploy/stow, joint articulation, tool attach/detach, grasp's
  own proximity gate, target selection/cycling, contact, or damage behavior.
- Releasing over empty space clear of the hull, near another registered
  body, or into the mining/storage flow still has no consequence beyond this
  one hull check -- that remains intentionally out of scope.

## CI-verifiable acceptance

- `src/simulation/tests/manipulator_release_tests.cpp`
  (`everward_manipulator_release_tests` in `src/simulation/CMakeLists.txt`)
  covers: nothing held fails closed; a body clear of the hull releases
  successfully; a body overlapping the hull fails closed without clearing
  the grasp; a since-deregistered held body fails closed; the gate stays
  scoped to the queried arm; the probe's own world pose (not just local
  origin) is accounted for; and the runtime overload matches the free
  function. All 20 `src/simulation` CTest suites (including this new one)
  were run locally and pass.
- `tools/test_phase2_manipulator_release_surface.py` and the updated
  `tools/test_phase2_manipulator_grasp_surface.py` (whose adapter assertion
  moved from a direct `Manipulators->release_grasp(` call to the new gated
  wrapper) were run locally via `python3 -m unittest discover -s tools -p
  "test_phase2*.py"`; all 132 Phase 2 source-contract tests pass.

No Unreal Editor/UBT build was available in this sandbox to compile-verify
`ProbeSimulationAdapter.cpp`. The adapter change follows the exact
`CommandGraspSelectedTarget` pattern that already compiles in the same file,
changing only which free function is called and the rejection message text.
The next local Unreal pass should specifically confirm the project still
compiles under UBT before relying on this further, the same caveat prior
parallel-safe passes in this repository have recorded when local UBT was
unavailable.

## Local Unreal Product Reality acceptance

1. Launch the exact CI-green build and enter PIE.
2. Deploy an arm, select the registered physical target (`T`), approach
   until REACH reads "IN REACH", and grasp it (`F`). Confirm `HOLDING`
   appears as in `PHASE2_MANIPULATOR_GRASP_TEST.md`.
3. Immediately attempt to release (`F`) without articulating the arm.
   Given the integration finding above, expect this to be rejected with
   "target would collide with probe hull" and the arm to remain `HOLDING`.
   Record it as a defect only if release instead silently succeeds while
   the mesh is visibly embedded in the probe body.
4. Open the manipulator page (`M`) and articulate the holding arm's
   shoulder/elbow joints (`,`/`.`) outward and away from the hull.
5. Release (`F`) again once the arm is well clear of the hull. Confirm it
   now succeeds: `HOLDING` clears, the mesh stays at its released position,
   and global feedback reports the released-target message.
6. Re-grasp the same target, articulate the arm back toward the hull, and
   attempt release again close to (but not overlapping) the boundary.
   Confirm the accept/reject boundary is stable and matches moving the mesh
   visibly in/out of the hull rather than flickering or lagging.
7. Confirm this pass has not changed deploy/stow, joint articulation, tool
   attach/detach, grasp's own proximity gate, target selection/cycling,
   contact, or damage behavior.
8. Record any discrepancy (release accepted while visibly embedded in the
   hull, release rejected while clearly clear of the hull, no joint pose
   within range ever clearing the hull, stale `HOLDING` state after a
   rejected release, or a build/compile failure) as Product Reality
   evidence.

## Explicitly not complete in this pass

- No consequence for releasing near another registered body (only the
  probe's own hull is checked).
- No "place" or "drop toward a target location" mechanic, and no automatic
  hand-off into the mining/storage flow on release.
- No velocity/momentum imparted to a released body; it simply remains at
  its last held position once release is accepted, matching
  `PHASE2_MANIPULATOR_MOVE_TEST.md`'s existing behavior.
- Slice 7's completion gate remains the local Product Reality pass recorded
  in `PROJECT_STATUS.md`; this closes the last previously-named
  release-with-consequence gap but does not by itself close the slice.

## Status

Implemented in the parallel-safe lane; Product Reality pending. Does not by
itself advance Slice 7's completion gate.
