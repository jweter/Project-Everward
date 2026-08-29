# Phase 2 — Manipulator Joint Articulation Product Reality Test

Status: implemented via the parallel-safe lane; local Unreal Engine 5.8 Product Reality verification required.

**Update (post-#127):** the "visible arm geometry" gap this document calls out below was closed by the Prime A embodiment pass (#127), which is out of scope for this document's own test script. See `PHASE2_SIMPLE_PRIME_A_EMBODIMENT_PASS.md` items 4–7 and `docs/PROJECT_STATUS.md`'s Slice 6 section for the current, accurate status — this file otherwise remains an accurate historical record of the joint-articulation input/HUD sub-slice itself.

## Purpose

Continue **Slice 6 — articulated manipulator arms** past the deploy/stow/tool foundation (`PHASE2_MANIPULATOR_ARM_FOUNDATION_TEST.md`) by adding the joint articulation input and dedicated manipulator HUD page that foundation explicitly deferred.

This sub-slice adds an input/HUD surface over mechanics that already exist and are already authoritative: `ManipulatorRig::command_joint_target_degrees` and `UProbeSimulationAdapter::CommandSetManipulatorJointTargetDegrees` were part of the foundation slice but had no bound control. No engine-independent simulation behavior changes; joint clamping and slew-rate limiting remain entirely inside `manipulator.hpp`. Visible arm geometry/animation remains out of scope here, matching `PHASE2_VERTICAL_SLICE_PLAN.md`'s note that it is the one part of Slice 6 requiring production art rather than adapter/HUD/input work.

## What this adds

`UProbeSimulationAdapter`'s `FEverwardManipulatorArmState` now also reports each joint's **commanded** target (`CommandedShoulderDegrees`/`CommandedElbowDegrees`/`CommandedWristDegrees`), alongside the current (still-slewing) angle it already reported. This lets input nudge the last commanded target instead of chasing the in-motion current angle.

`AEverwardHUD` gains a dedicated manipulator page, independent of the Tab-toggled systems panel (two arms can be deployed and mid-pose at once, which doesn't fit the systems panel's single-selected-capability model):

- `M` toggles the manipulator page open/closed;
- `N` cycles which arm (Port/Starboard) joint input targets;
- `4` / `5` / `6` select the Shoulder / Elbow / Wrist joint on the selected arm;
- `,` / `.` decrease/increase that joint's commanded target by 5° (configurable via `ManipulatorJointAdjustmentDegrees`), sent through the existing `CommandSetManipulatorJointTargetDegrees`.

The page shows both arms, marks the selected arm and joint with `>`, lists current -> commanded degrees per joint, and shows `DEPLOY ARM TO COMMAND JOINTS` for a stowed/stowing arm instead of joint rows. A joint command sent while the arm isn't fully deployed is rejected by the existing authoritative gate in `ManipulatorRig::command_joint_target_degrees` and surfaces through the existing `COMMAND REJECTED` banner — this sub-slice adds no new rejection path.

## Local UE 5.8 test script

1. Launch the exact CI-green build.
2. Press `M`. Confirm a `MANIPULATOR CONTROL` panel appears above the compact telemetry panel, showing both arms as `STOWED` with `DEPLOY ARM TO COMMAND JOINTS` under each.
3. Press `1` to deploy the Port arm. Once `DEPLOYED`, confirm the Port arm's panel entry now shows three joint rows (`SHOULDER`, `ELBOW`, `WRIST`) each reading `0.0 DEG -> 0.0 DEG`.
4. Confirm `SHOULDER` is marked selected (`>`) by default under the selected arm.
5. Press `.` several times. Confirm the commanded target on the `SHOULDER` line increases by 5° per press and the current-angle side visibly slews toward it over about a second (`kJointSlewDegreesPerSecond` = 45°/s), not snapping instantly.
6. Press `,` repeatedly past the shoulder's -90°..90° range. Confirm the commanded target stops at the clamped limit rather than going out of range or being rejected.
7. Press `5` then `.` a few times. Confirm ELBOW is now the one moving and SHOULDER holds its prior target.
8. Press `6` then `.` a few times. Confirm WRIST moves independently of the other two.
9. Press `N`. Confirm selection switches to the Starboard arm (still `STOWED`), and pressing `.` now does nothing observable (arm not deployed) while a `COMMAND REJECTED` banner explains why.
10. Deploy the Starboard arm, repeat steps 4–8 for it, and confirm adjusting Starboard's joints does not move the Port arm.
11. Press `M` again. Confirm the panel closes and the always-visible compact `PORT ARM` / `STBD ARM` status lines are unaffected.
12. Confirm existing flight, scan, power, deploy/stow, tool-attach, and contact/damage behavior is unaffected by any of the above.

## Acceptance questions

- Is it immediately obvious from the HUD alone that each arm has three independently controllable joints?
- Does the current -> commanded readout make the gradual slew read as real mechanical motion rather than an instant snap?
- Is the "deploy before commanding joints" gate understandable from the panel text and the rejection banner alone, without reading commit history?
- Do Port and Starboard truly move independently when both are deployed?

## Explicitly not complete in this slice

- visible arm geometry/skeletal mesh and animation (joints still have no visual representation to move);
- power draw and integrity-based effectiveness for manipulator motion;
- grasping/holding an object (Slice 7);
- collision between the arm and the probe body or environment.

Slice 6 is not complete until visible arm geometry/animation lands and local Unreal evidence is recorded for the whole slice.
