# Phase 2 — Manipulator Grasp Product Reality Test

## Scope

`PHASE2_VERTICAL_SLICE_PLAN.md`'s Slice 7 minimum interactions run
`detect -> select -> approach -> scan -> reach -> grasp -> move -> release`.
`detect -> select` and `align a manipulator` ("reach") already had foundation
work (`PHASE2_TARGET_SELECTION_TEST.md`, `PHASE2_TARGET_CYCLING_TEST.md`,
`PHASE2_TARGET_VISUAL_INDICATOR_TEST.md`,
`PHASE2_MANIPULATOR_REACH_TELEMETRY_TEST.md`), all of it read-only telemetry.
This pass adds the first state-mutating Slice 7 interaction past that:
**grasp or dock with a simple object**. It does not add `move` (a grasped
object does not yet follow the probe or the arm) or `release`-as-a-distinct-
concept beyond simply letting go — those, plus everything downstream of a
held object, remain later, not-yet-attempted work.

The production path is:

`manipulator_reach_status() (existing, unchanged, from manipulator_reach.hpp)
-> attempt_grasp_selected_target() (new, engine-independent, in
manipulator_grasp.hpp) -> ManipulatorRig::begin_grasp()/release_grasp() (new,
in manipulator.hpp) -> UProbeSimulationAdapter::CommandGraspSelectedTarget()/
CommandReleaseGraspedTarget() (new) -> AEverwardHUD's existing manipulator
page (arm line now reports HOLDING <id>) -> EverwardPlayerController's F key
(new)`

The proximity gate is exactly `manipulator_reach_status()`'s existing
`in_reach` result — the same computation the REACH row on the manipulator
page already renders. `attempt_grasp_selected_target()` does not invent a
second notion of "close enough": a grasp attempt succeeds if and only if the
REACH row would currently read "IN REACH". This qualifies for the
parallel-safe lane the same way reach telemetry did: it composes only
already-authoritative state (reach, arm deployment, target selection)
through the existing simulation/adapter boundary, does not assume any still-
pending contact/collision/damage behavior is correct, and introduces exactly
one new authoritative field (`ManipulatorArmState::grasped_target_body_id`)
guarded by mechanical invariants a real gripper would have regardless of
Unreal-side collision correctness (deployed-only, one object at a time,
cannot stow while holding something).

## Behavior

- With the manipulator page (`M`) open, pressing `F` attempts to grasp the
  currently selected physical target (`T`) with whichever arm the page
  currently has selected (`N` to switch), or releases it if that arm is
  already holding something.
- A grasp attempt succeeds only when that arm reports "IN REACH" on the
  REACH row; otherwise the command is rejected (visible in the command
  banner) and no state changes.
- Once grasped, that arm's status line on both the always-visible telemetry
  panel and the manipulator page appends `// HOLDING <target id>`.
- Releasing (`F` again while holding) always succeeds regardless of current
  range — an operator letting go does not require re-proving proximity.
- Attempting to stow an arm that is still holding a target is rejected with
  an explicit reason, the same way stowing with a tool still attached
  already is; the target must be released first.
- No move/reposition mechanics exist yet: a grasped object's own position is
  unchanged by this pass. It also does not currently prevent stowing the
  *other* arm, changing target selection, or clearing the selection while
  holding something — none of those interactions are defined by this
  minimum slice.

## CI-verifiable acceptance

- `src/simulation/tests/manipulator_tests.cpp` covers `ManipulatorRig`'s own
  mechanical invariants directly: grasp requires a fully deployed arm and a
  non-empty target id, grasping while already holding something throws,
  releasing while not holding anything throws, stowing while holding
  something is rejected (and succeeds once released), and the two arms'
  grasp state is independent.
- `src/simulation/tests/manipulator_grasp_tests.cpp`
  (`everward_manipulator_grasp_tests` in `src/simulation/CMakeLists.txt`)
  covers the proximity gate itself: no target selected, arm not deployed,
  out of reach, and a stale/deregistered selection all fail closed without
  mutating rig state; an in-reach attempt succeeds and matches the exact
  geometry `manipulator_reach_tests.cpp` already uses for its own in-reach
  case; the gate stays scoped to the queried arm; and the
  `DamageAwareProbeRuntime` runtime-convenience overload matches the
  lower-level free function.
- `tools/test_phase2_manipulator_grasp_surface.py` confirms
  `manipulator.hpp` owns the grasp state and its mechanical invariants,
  `manipulator_grasp.hpp` reuses `manipulator_reach_status()` rather than a
  second proximity formula, the adapter forwards to
  `attempt_grasp_selected_target()`/`release_grasp()` without re-deriving the
  gate, the HUD surfaces `bTargetGrasped`/`GraspedTargetId`, and the player
  controller binds `F` to the HUD-selected arm via
  `GetSelectedManipulatorArmIndex()`.

No Unreal Editor/UBT build was available in this sandbox to compile-verify
`ProbeSimulationAdapter.h`/`.cpp`, `EverwardHUD.cpp`, or
`EverwardPlayerController.h`/`.cpp`. The adapter change follows the exact
accessor/command pattern `GetManipulatorReachStatus()` and
`CommandAttachManipulatorTool()`/`CommandDetachManipulatorTool()` already use
and compile elsewhere in that file; the HUD change extends the existing
`ManipulatorArmLine()` helper the same way the pre-existing `TOOL` suffix
does; the input binding follows the exact `EKeys::`/`BindKey` pattern the
rest of `EverwardPlayerController.cpp` already uses, and the HUD-selected-arm
resolution mirrors the existing pattern in
`EverwardPlayerControllerInteractionTick.cpp`'s manipulator highlight logic.
The next local Unreal pass should specifically confirm the project still
compiles under UBT before relying on this further, the same caveat prior
parallel-safe passes in this repository have recorded when local UBT was
unavailable.

## Local Unreal Product Reality acceptance

1. Launch the exact CI-green build and enter PIE.
2. Press `M` to open the manipulator page, `T` to select the registered
   physical target, and deploy an arm. Confirm the REACH row reads "OUT OF
   REACH" from a distance.
3. Press `F`. Confirm the command is rejected (visible in the command
   banner) and the arm's status line does not show `HOLDING`.
4. Fly the probe until the REACH row reads "IN REACH". Press `F` again and
   confirm the arm's status line on both the manipulator page and the
   always-visible telemetry panel now reads `// HOLDING <target id>`, and the
   command banner confirms the grasp.
5. Attempt to stow that arm (`1`/`2` matching the held arm). Confirm the
   command is rejected with a reason naming the grasped target.
6. Press `F` again to release. Confirm `HOLDING` disappears from both status
   lines, and that stowing the arm now succeeds.
7. Press `N` to switch the manipulator page to the other arm and confirm `F`
   now acts on that arm instead (grasp/release state is per-arm and
   independent).
8. Confirm this pass has not changed deploy/stow, joint articulation, tool
   attach/detach, target selection/cycling, the visual selection highlight,
   reach telemetry, mining, scan, contact, or damage behavior.
9. Record any discrepancy (a grasp that succeeds while the REACH row still
   reads "OUT OF REACH" or vice versa, a stale `HOLDING` label, incorrect arm
   attribution after `N`, or a build/compile failure) as Product Reality
   evidence.

## Explicitly not complete in this pass

- No `move` mechanics: a grasped object's position is unaffected by probe or
  arm motion.
- No world-space visual indicator that an object is grasped (no tether line,
  attachment glow, or similar) beyond the existing HUD text.
- No automatic approach-and-align assist.
- Slice 7 is not complete: `move`/`release`-with-consequence still do not
  exist, and the completion gate for the whole slice remains the local
  Product Reality pass in `PROJECT_STATUS.md`.

## Status

Implemented in the parallel-safe lane; Product Reality pending. Does not by
itself advance Slice 7's completion gate.
