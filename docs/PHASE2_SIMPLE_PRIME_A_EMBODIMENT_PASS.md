# Phase 2 Simple Prime A Embodiment Pass

## Why this slice exists

The latest local screenshots proved that the prior blockout was useful engineering scaffolding but did not yet read as the intended Everward probe. The user also observed that contact occurred in visibly empty space around the craft.

This pass moves the visible body toward the canonical early Probe A idea without pretending production art is finished:

- one coherent central tube;
- two lateral thermal/radiator wings;
- one obvious rear propulsion assembly;
- one compact forward science/sensor section;
- visible articulated mining/service arms connected to the existing authoritative manipulator state.

## Product goal

From ordinary third-person distance, EV-0001 should read as **one machine** rather than a collection of disconnected primitives.

The silhouette should immediately communicate:

`tube body + radiator wings + engine + science head + mining arm`

## Manipulator presentation

The existing engine-independent `ManipulatorRig` remains authoritative. Unreal only presents it.

The new shoulder/elbow/wrist hierarchy reads:

`shoulder pivot -> upper arm -> elbow pivot -> forearm -> wrist pivot -> tool head`

Deployment fraction and current joint angles come from `UProbeSimulationAdapter::GetManipulatorArmStates()` every tick. No visual component authors manipulator mechanics.

## Collision feedback from the local build

The reported "bigger box around the probe" is real Product Reality evidence, not an art problem.

The current contact solver models EV-0001 with a conservative 8 m-radius sphere. That was acceptable to prove swept contact and damage, but a sphere is a poor physical proxy for a long narrow probe. With the coherent tube silhouette, the mismatch is now easier to see.

This pass deliberately does **not** fake a fix by shrinking only the Unreal mirror or moving decorative geometry. Unreal and the engine-independent runtime must continue to agree on mechanical truth.

The next contact-shape correction should replace the single spherical probe envelope with an authoritative elongated/compound representation (capsule, swept segment + radius, or a small compound set) while preserving deterministic swept collision and impact telemetry.

## Local Product Reality checks

When this branch is built in Unreal 5.8, verify:

1. EV-0001 reads as one continuous tube-bodied probe from front, side, rear, and top.
2. Both radiator wings visibly connect to and belong to the central body.
3. Rear propulsion and forward sensor are obvious at a glance.
4. Manipulator geometry is visible under/alongside the body rather than represented only by HUD state.
5. Deploy/stow commands visibly move the corresponding arm.
6. Shoulder/elbow/wrist joint commands visibly articulate the arm and track the HUD values.
7. Tool attach/detach remains mechanically accepted/rejected exactly as before and has a visible tool-head presentation change.
8. Movement, scanning, power allocation, righting, contact, and damage are not regressed.
9. Record whether the 8 m spherical contact proxy still produces obviously early contact. It is expected to remain a known issue until the authoritative contact-shape follow-up lands.

## Acceptance

This slice can merge when portable CI is green because it is presentation over existing authoritative state and does not weaken the current collision gate. It remains Product Reality pending until a local Unreal run proves the visual hierarchy works as intended.

The collision-envelope mismatch is **not accepted as final**; it becomes the next physics correction after this embodiment pass.
