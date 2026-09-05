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

## Eleventh sub-slice: releasing near another registered body

This pass's own "explicitly not complete" list (below) named releasing near
another registered body as the first still-open release-with-consequence
gap -- only the probe's own hull was checked. `manipulator_release.hpp`
gains `sphere_intersects_other_registered_body()`, the same sphere-overlap
test `sphere_intersects_compound_hull()` already runs against the hull's
five samples, run instead against every other currently registered
`StaticSphereBody` (skipping the held body itself by id, since it always
overlaps its own recorded position). `attempt_release_grasped_target()`
now rejects a release that would leave the held body overlapping the
probe's own hull **or** any other registered body; `release_grasp` itself
is still unconditional and unchanged. The Unreal-side rejection message
widened from "target would collide with probe hull" to "target would
collide with the probe hull or another object" to match; no other adapter
behavior changed.

No new grasp/reach/move mechanic is introduced, and this does not touch the
existing hull check or any behavior still awaiting Product Reality -- it
adds one more registered-body-shaped obstruction to the same fail-closed
gate already documented above.

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
  probe's own hull envelope, or any other currently registered physical
  body, is rejected: the arm keeps `HOLDING <id>`, and global feedback
  reports "arm cannot release: target would collide with the probe hull or
  another object".
- Releasing once the held body is clear of the hull and every other
  registered body succeeds exactly as before: the mesh stays at its
  current (moved) position, `HOLDING` clears, and `TARGET`/`REACH`
  telemetry update the same way `PHASE2_MANIPULATOR_MOVE_TEST.md` already
  documents.
- No change to deploy/stow, joint articulation, tool attach/detach, grasp's
  own proximity gate, target selection/cycling, contact, or damage behavior.
- Releasing over empty space clear of the hull and every other registered
  body, or into the mining/storage flow, still has no consequence beyond
  these two overlap checks -- that remains intentionally out of scope.

## CI-verifiable acceptance

- `src/simulation/tests/manipulator_release_tests.cpp`
  (`everward_manipulator_release_tests` in `src/simulation/CMakeLists.txt`)
  covers: nothing held fails closed; a body clear of the hull releases
  successfully; a body overlapping the hull fails closed without clearing
  the grasp; a body overlapping another registered body fails closed
  without clearing the grasp; a body clear of every other registered body
  (but far from the hull) releases successfully; the other-body overlap
  test itself correctly excludes the held body from comparison against
  itself; a since-deregistered held body fails closed; the gate stays
  scoped to the queried arm; the probe's own world pose (not just local
  origin) is accounted for; and the runtime overload matches the free
  function. All 20 `src/simulation` CTest suites (including this one) were
  run locally and pass.
- `tools/test_phase2_manipulator_release_surface.py`, extended with an
  assertion that the new other-registered-body gate is wired into
  `attempt_release_grasped_target`, was run locally via `python3 -m
  unittest discover -s tools -p "test_phase2*.py"`; all 134 Phase 2
  source-contract tests pass.

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
8. With another registered reference target present in the scene (e.g. the
   Slice 8 partial `phase2-test-target-002`/`-003` bodies), articulate the
   holding arm so the grasped target's position overlaps that other body
   and attempt release (`F`). Confirm it is rejected the same way as the
   hull case, with feedback reading "target would collide with the probe
   hull or another object", then move clear and confirm release succeeds.
9. Record any discrepancy (release accepted while visibly embedded in the
   hull or another registered body, release rejected while clearly clear of
   both, no joint pose within range ever clearing the hull, stale `HOLDING`
   state after a rejected release, or a build/compile failure) as Product
   Reality evidence.

## Explicitly not complete in this pass

- No "place" or "drop toward a target location" mechanic, and no automatic
  hand-off into the mining/storage flow on release.
- No velocity/momentum imparted to a released body; it simply remains at
  its last held position once release is accepted, matching
  `PHASE2_MANIPULATOR_MOVE_TEST.md`'s existing behavior.
- Slice 7's completion gate remains the local Product Reality pass recorded
  in `PROJECT_STATUS.md`; this closes the "releasing near another registered
  body" gap but does not by itself close the slice.

## Status

Implemented in the parallel-safe lane; Product Reality pending. Does not by
itself advance Slice 7's completion gate.
