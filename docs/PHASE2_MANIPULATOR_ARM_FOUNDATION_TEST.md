# Phase 2 — Manipulator Arm Foundation Product Reality Test

Status: implemented in PR; local Unreal Engine 5.8 Product Reality verification required.

## Purpose

Begin **Slice 6 — articulated manipulator arms** on the Port/Starboard shoulder mounts the Prime Generation-1 blockout already exposes (`PHASE2_PRIME_GEN1_BODY_BLOCKOUT_TEST.md`).

This is a foundation sub-slice, not the complete slice: it lands the authoritative deploy/stow and tool-attach mechanics plus a minimal input surface. Joint articulation input, arm/tool visual presentation, and animation remain explicitly out of scope here (see "Explicitly not complete in this slice" below) and are the next sub-slice.

## Authoritative mechanics

`src/simulation/include/everward/simulation/manipulator.hpp` adds an engine-independent, deterministic `ManipulatorRig` covering exactly two arms (Port/Starboard):

- deploy/stow is gradual (2 s authoritative duration), not instantaneous, and each direction fires its `*Started`/`*Completed` event exactly once;
- reversing direction mid-transition (e.g. stow while still deploying) is supported;
- joint targets (shoulder/elbow/wrist) are only accepted once an arm is fully deployed, are clamped to a constrained range rather than rejected when out of range, and converge toward the commanded target at a bounded slew rate rather than snapping;
- a tool interface can be attached only on a fully deployed arm, and an arm cannot be stowed while a tool remains attached.

This mirrors the existing `SimulationCore`/`ImpactDamageModel` pattern: Unreal presents whatever this produces but never authors it. It does not yet participate in power draw or damage/integrity effectiveness; that composition is a documented follow-up, the same way `impact_damage.hpp` was added after `contact_physics` rather than in the same slice.

## Unreal surface added

`UProbeSimulationAdapter` now owns a `ManipulatorRig` alongside the existing `DamageAwareProbeRuntime`, advanced on the same fixed-step cadence:

- `GetManipulatorArmStates()` — authoritative Port/Starboard state (deployed/deploying/stowing, deployment fraction, joint angles, tool attached);
- `CommandDeployManipulatorArm` / `CommandStowManipulatorArm`;
- `CommandSetManipulatorJointTargetDegrees`;
- `CommandAttachManipulatorTool` / `CommandDetachManipulatorTool`.

Minimal input is bound directly (no HUD panel selection required, since there is no dedicated manipulator systems page yet):

- `1` toggles the Port arm between deploy/stow;
- `2` toggles the Starboard arm between deploy/stow;
- `3` toggles tool attach/detach on whichever arm is currently deployed (Port takes priority if both are).

The compact telemetry panel (top-left) now shows a `PORT ARM` / `STBD ARM` line each, reporting STOWED / DEPLOYING NN% / DEPLOYED / STOWING NN%, plus `// TOOL` when a tool is attached.

## Local UE 5.8 test script

1. Launch the exact CI-green build.
2. Confirm both arm status lines read `STOWED` at spawn.
3. Press `1`. Confirm the Port line transitions to `DEPLOYING NN%` and the percentage increases over roughly two seconds before settling on `DEPLOYED`.
4. Press `1` again while `DEPLOYED`. Confirm it begins `STOWING NN%` and returns to `STOWED`.
5. Press `1` to deploy again, then press `1` a second time partway through deployment. Confirm it reverses cleanly into `STOWING` rather than getting stuck or jumping states.
6. Deploy the Port arm fully, then press `3`. Confirm the line gains `// TOOL`.
7. With the tool attached, press `1` (stow). Confirm the command is rejected (a "COMMAND REJECTED" banner referencing the tool) and the arm does not begin stowing.
8. Press `3` again to detach the tool, then press `1`. Confirm the arm now stows normally.
9. Repeat steps 3–8 for the Starboard arm using `2` and confirm the two arms operate independently (deploying one does not affect the other).
10. Confirm existing flight, scan, power, and contact/damage behavior is unaffected by any of the above.

## Acceptance questions

- Is it immediately obvious from the HUD alone (without reading commit history) that two independent manipulator arms now exist and can be deployed?
- Does the gradual deploy/stow percentage read as a real mechanical transition rather than an instant flag flip?
- Is the tool-attach/stow interlock (detach before stow) understandable from the rejection message alone?
- Do the two arms clearly behave independently?

## Explicitly not complete in this slice

- visible arm geometry/skeletal mesh and animation (only the existing shoulder mount blockout meshes are present; the arms do not yet visually move);
- power draw and integrity-based effectiveness for manipulator motion;
- grasping/holding an object, which requires Slice 7's object-interaction foundation;
- collision between the arm and the probe body or environment.

Joint articulation input and a dedicated manipulator HUD page were the next sub-slice named above; see `PHASE2_MANIPULATOR_JOINT_ARTICULATION_TEST.md` for that follow-up (implemented, Product Reality pending) and its local test script.

Those build on this authoritative deploy/stow/tool foundation rather than requiring a later rewrite of it.
