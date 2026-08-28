# Phase 2 — Physical Body / Contact Product Reality Test

Status: **Product Reality passed for the original contact behavior on 2026-08-27.** Prime Generation-1 blockout rescaling introduces an updated 8 m conservative envelope that should be rechecked as part of the Slice-5 body test.

## Purpose

This is the first physical-body slice in Everward. EV-0001 must stop behaving like a camera pawn that can ghost through matter.

The implementation deliberately separates three concerns:

1. **authoritative contact truth** lives in the engine-independent simulation runtime;
2. **Unreal presentation/query geometry** mirrors the same body/envelope dimensions;
3. impact/damage consumes this contact truth rather than letting Unreal invent damage.

## Current physical model

### Probe

The original contact slice used a temporary **0.75 m radius** sphere around the tiny engineering-shell placeholder. That behavior passed local Product Reality: the probe contacted the target, deflected/slid rather than ghosting through it, and no tunneling/sticking failure was reported.

Slice 5 replaces the tiny presentation with the approximately 15 m Prime Generation-1 body blockout. The authoritative and Unreal-mirrored collision envelope therefore move to a conservative **8 m radius bounding sphere** so contact remains meaningfully tied to the visible spacecraft scale.

The 8 m sphere is still transitional. A later compound collision envelope should follow the actual Prime geometry more closely while preserving this same contact telemetry contract.

### Bootstrap physical body

The existing Phase-2 scan target is a reproducible solid sphere:

- ID: `phase2-test-target-001`
- center: `[50, 0, 0] m`
- radius: `2.0 m`

The Unreal sphere and engine-independent simulation body use the same shared constants.

## Contact behavior

Motion uses a **swept sphere** test from the probe's previous position to its integrated next position. This prevents high-speed tunneling through the body.

On contact the runtime records:

- contacted body ID;
- contact point;
- outward surface normal;
- relative velocity at contact;
- inward normal speed;
- simulation tick of contact.

Resolution places the probe just outside the combined body/envelope radius and removes only the velocity component moving into the surface. Tangential velocity is preserved, permitting a primitive slide/deflection rather than forcing every glancing contact to a full stop.

Impact severity/component damage now builds on this telemetry in the authoritative simulation layer.

## Recorded Product Reality evidence — 2026-08-27

Local Unreal Engine 5.8 playtest report:

- scan works;
- yaw/pitch/roll and camera-aligned `R` righting work;
- physical target contact blocks passage through the body;
- head-on/glancing contact produces deflection, bounce, or sliding off the surface;
- requested collision sequence was completed with no tunneling/sticking failure reported.

This passes the original collision/contact prerequisite. Restitution/bounce feel remains a tuning concern rather than a blocker.

## Slice-5 rescale recheck

Because the Prime body is much larger than the original placeholder, run these checks on the new blockout:

1. Approach the same physical target nearly head-on at low speed.
2. Confirm contact occurs near the visible Prime body rather than meters inside the nose/engine.
3. Hold forward motion and confirm no tunneling.
4. Move away and confirm clean departure.
5. Approach diagonally and confirm some tangential slide/deflection remains.
6. Orbit/zoom the camera and confirm the 8 m envelope does not make interaction obviously detached from the visible body.
7. Scan the target and confirm scan/contact remain distinct systems.

## Current known limitation

A bounding sphere is deliberately conservative. It can create visible stand-off around narrower lateral/diagonal portions of an elongated spacecraft. If Product Reality makes that stand-off obvious or distracting, the next collision refinement should be a simulation-owned compound envelope rather than shrinking the sphere until the nose/engine can penetrate.
