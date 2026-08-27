# Phase 2 — Physical Body / Contact Product Reality Test

Status: implementation branch; local Unreal Engine 5.8 Product Reality verification required.

## Purpose

This is the first physical-body slice in Everward. EV-0001 must stop behaving like a camera pawn that can ghost through matter.

The implementation deliberately separates three concerns:

1. **authoritative contact truth** lives in the engine-independent simulation runtime;
2. **Unreal presentation/query geometry** mirrors the same body/envelope dimensions;
3. **damage is not invented yet**. This slice records the information Slice 4 will need for physically grounded impact consequences.

## Current physical model

### Probe

EV-0001 currently uses a temporary spherical collision envelope with a **0.75 m radius** around its authoritative center of mass.

That envelope is separate from the temporary decorative/orientation meshes. The later Prime Probe A blockout will replace it with a more representative compound body while keeping the same contact telemetry contract.

### Bootstrap physical body

The existing Phase-2 scan target is now also a reproducible solid sphere:

- ID: `phase2-test-target-001`
- center: `[50, 0, 0] m`
- radius: `2.0 m`

The Unreal sphere and the engine-independent simulation body use the same shared constants.

## Contact behavior

Motion uses a **swept sphere** test from the probe's previous position to its integrated next position. This prevents high-speed tunneling through the body.

On contact the runtime records:

- contacted body ID;
- contact point;
- outward surface normal;
- relative velocity at contact;
- inward normal speed;
- simulation tick of contact.

Resolution places the probe just outside the combined body/envelope radius and removes only the velocity component moving into the surface. Tangential velocity is preserved, permitting a primitive slide rather than forcing every glancing contact to a full stop.

The HUD promotes the latest contact for four simulated seconds. A temporary `CONTACT WARNING` cue appears when normal contact speed is at least **5 m/s**. This is only an operator warning for Product Reality testing; it is **not** a damage or severity model. Slice 4 will derive impact consequences from physical inputs such as mass and impact energy.

## Local UE 5.8 test script

Run the exact CI-green build in one session.

1. Confirm the target label reads `PHASE-2 PHYSICAL BODY // SCAN-001`.
2. Approach the target nearly head-on at low speed.
3. Confirm EV-0001 stops at the surface instead of entering or passing through the sphere.
4. Confirm the HUD shows `CONTACT`, body ID, normal speed, contact point, and surface normal.
5. Continue commanding forward motion into the body and confirm the probe does not tunnel through it.
6. Command motion away from the surface and confirm EV-0001 departs normally rather than sticking.
7. Approach again on a diagonal/glancing trajectory.
8. Confirm inward motion is blocked while some tangential motion remains, producing a rough slide rather than an arbitrary full stop.
9. Build a faster approach with normal speed above 5 m/s and confirm the HUD changes to `CONTACT WARNING`.
10. Confirm that warning does **not** damage a subsystem yet; damage belongs to the next slice.
11. Orbit the camera, use `R` righting, brake with `Space`, and verify those controls still behave normally near the body.
12. Confirm Sensors can still scan the same physical body and that scan completion remains distinct from contact.

## Product Reality acceptance questions

- Can EV-0001 physically reach the target without passing through it?
- Does contact happen at a believable distance from the visible sphere?
- Is head-on stopping stable rather than jittery or explosive?
- Does a glancing approach retain tangential motion?
- Can the player immediately move away after contact?
- Does the HUD make the contact point/normal/speed understandable?
- Does the temporary 0.75 m envelope feel roughly consistent with the current engineering-shell visual?
- Does the 5 m/s warning help communicate caution without pretending a damage system already exists?

## Explicitly not in this slice

- arbitrary hit points;
- component damage;
- impact-energy severity classes;
- restitution/bounce tuning;
- rotating/moving collision bodies;
- mesh/convex collision shapes;
- terrain/planet surface collision;
- gravity.

Those build on this contact record rather than bypassing it.
