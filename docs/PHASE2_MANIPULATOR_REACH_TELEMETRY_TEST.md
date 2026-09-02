# Phase 2 — Manipulator Reach Telemetry Product Reality Test

## Scope

`PHASE2_VERTICAL_SLICE_PLAN.md`'s Slice 7 minimum interactions run
`detect -> select -> approach -> scan -> reach -> grasp -> move -> release`.
Foundation work has already landed for `detect -> select` (target selection,
range/closing-speed telemetry, nearest→farthest cycling, and a visual
selection indicator; see `PHASE2_TARGET_SELECTION_TEST.md`,
`PHASE2_TARGET_CYCLING_TEST.md`, `PHASE2_TARGET_VISUAL_INDICATOR_TEST.md`)
and separately for the arms themselves (`PHASE2_MANIPULATOR_ARM_FOUNDATION_TEST.md`,
`PHASE2_MANIPULATOR_JOINT_ARTICULATION_TEST.md`). Nothing before this pass
connected the two: there was no way to tell whether a deployed arm was
actually close enough to the selected target to do anything with it.

This pass adds exactly the "align a manipulator" minimum interaction and
nothing past it: **read-only telemetry** reporting whether the currently
selected manipulator arm's wrist is within a fixed reach envelope of the
selected physical target's surface, and how far away it still is. It does
**not** add grasp, attach, dock, or any other state-mutating interaction —
those remain later, not-yet-attempted Slice 7 sub-slices.

The production path is:

`manipulator_arm_contact_samples() (existing, unchanged, from
manipulator_hull_contact.hpp) + rotate_local_contact_offset() (existing,
unchanged) -> manipulator_reach_status() (new, engine-independent, in
manipulator_reach.hpp) -> UProbeSimulationAdapter::GetManipulatorReachStatus()
(new) -> AEverwardHUD's existing manipulator page (new REACH row)`

No new simulation state is mutated. The wrist position reuses the exact
forward-kinematics helper the arm/environment collision guard already
computes for the same wrist pivot; the range-to-surface reuses
`target_selection.hpp`'s `surface_range_to_body()` against the same
registered `StaticSphereBody` list the HUD `TARGET` row already reads. This
qualifies for the parallel-safe lane the same way the underlying target
selection and manipulator telemetry did: it reads already-authoritative
state through the existing simulation/adapter boundary, mutates nothing, and
does not assume any still-pending contact/collision/damage behavior is
correct.

## Behavior

- The dedicated manipulator HUD page (`M`) gains a `REACH` row directly
  below whichever arm/joint rows it is already showing, for whichever arm
  the page currently has selected (`N` to cycle, matching the existing
  joint-articulation selection).
- The row is shown only once a physical target is actually selected (the
  `TARGET` row shows something other than "NONE SELECTED"). With no target
  selected there is nothing authoritative to report range against, so the
  row is omitted entirely rather than showing a fabricated distance.
- With a target selected:
  - if the selected arm is not fully deployed (stowed, mid-deploy, or
    mid-stow), the row reads a muted "ARM NOT DEPLOYED / NO VALID POSE"
    explanation rather than a number;
  - if the arm is fully deployed, the row reads either "IN REACH // *X* M TO
    SURFACE" or "OUT OF REACH // *X* M REMAINING", live-updating as the
    probe approaches/retreats or the arm's commanded pose slews.
- No new input binding was added or is required: the row simply extends the
  existing manipulator page and target-selection surfaces.

## CI-verifiable acceptance

- `src/simulation/tests/manipulator_reach_tests.cpp`
  (`everward_manipulator_reach_tests` in `src/simulation/CMakeLists.txt`)
  covers the pure math directly: in-reach, out-of-reach with the expected
  remaining distance, no-target-selected, arm-not-deployed, arm-mid-deploy,
  arm-mid-stow, a stale/deregistered selected-target id, the range tracking
  the probe's live world pose rather than a pose fixed at some earlier call,
  and the `DamageAwareProbeRuntime` runtime-convenience overload matching
  the lower-level free function.
- `tools/test_phase2_manipulator_reach_surface.py` confirms
  `manipulator_reach.hpp` is engine-independent and reuses
  `manipulator_arm_contact_samples()`, `rotate_local_contact_offset()`, and
  `surface_range_to_body()` rather than inventing new geometry; that the
  fail-closed conditions are present verbatim; that
  `UProbeSimulationAdapter::GetManipulatorReachStatus()` calls straight
  through to the engine-independent function without re-implementing the
  envelope comparison or forward kinematics itself; and that the HUD reads
  `bHasResult`/`bInReach`/the two distance fields without pulling in any
  `everward/simulation` header directly.

No Unreal Editor/UBT build was available in this sandbox to compile-verify
`ProbeSimulationAdapter.h`/`.cpp` or `EverwardHUD.cpp`. The adapter change
follows the exact accessor pattern `GetSelectedTargetStatus()` and
`GetManipulatorArmStates()` already use and compile elsewhere in that file
(read `Core`/`Manipulators`, recompute live, return a plain `USTRUCT`); the
HUD change follows the exact panel-row pattern the existing arm/joint rows
already use and compile elsewhere in `EverwardHUD.cpp`. The next local
Unreal pass should specifically confirm the project still compiles under
UBT before relying on this further, the same caveat prior parallel-safe
passes in this repository have recorded when local UBT was unavailable.

## Local Unreal Product Reality acceptance

1. Launch the exact CI-green build and enter PIE.
2. Press `M` to open the manipulator page. Confirm no `REACH` row appears
   while no physical target is selected.
3. Press `T` to select the registered physical target. Confirm a `REACH`
   row now appears for whichever arm the manipulator page currently has
   selected, reading "ARM NOT DEPLOYED / NO VALID POSE" while that arm is
   stowed.
4. Deploy that arm (`1` for Port, `2` for Starboard, matching whichever the
   page has selected). Once deployment completes, confirm the row switches
   to a live "IN REACH" or "OUT OF REACH // *X* M REMAINING" reading.
5. Fly the probe toward the registered target and confirm the remaining
   distance decreases smoothly and the row flips to "IN REACH" once close
   enough, without needing to reopen the manipulator page.
6. Retreat and confirm the row flips back to "OUT OF REACH" with an
   increasing remaining distance, and that clearing the target selection
   (retreat past selection range, then `T`) removes the row entirely rather
   than leaving a stale reading.
7. Press `N` to switch the manipulator page to the other arm and confirm the
   `REACH` row now reports that arm's own wrist position instead of the
   previously selected arm's.
8. Confirm this pass has not changed deploy/stow, joint articulation,
   target selection/cycling, the visual selection highlight, mining, scan,
   contact, or damage behavior.
9. Record any discrepancy (wrong reach/not-reach transition, a stale
   reading after the selection changes, incorrect arm attribution after
   `N`, or a build/compile failure) as Product Reality evidence.

## Explicitly not complete in this pass

- No grasp, attach, or dock mechanics, and no new authoritative state of any
  kind — this is read-only telemetry over already-authoritative pose,
  target-selection, and manipulator state.
- No world-space visual alignment aid (a reticle, a highlighted approach
  vector, or similar) — the indicator lives entirely on the existing
  manipulator HUD page.
- No automatic approach-and-align assist — the player must fly the probe and
  the arm into position manually; this only reports whether that has
  succeeded.
- Slice 7 is not complete: `move`/`release` and the `grasp or dock with a
  simple object` minimum interaction still do not exist.

## Status

Implemented in the parallel-safe lane; Product Reality pending. Does not by
itself advance Slice 7's completion gate.
