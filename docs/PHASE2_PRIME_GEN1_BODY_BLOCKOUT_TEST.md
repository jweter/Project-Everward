# Phase 2 — Prime Generation-1 Body Blockout Product Reality Test

Status: implemented in PR; local Unreal Engine 5.8 Product Reality verification required.

## Purpose

Replace the temporary engineering-shell primitive with the first recognizable in-game body for the canonical **Probe A / Scientific Explorer / Prime Generation-1** machine.

This is a blockout, not production art. The goal is to make scale, silhouette, component layout, camera framing, collision scale, and future articulation architecture testable before investing in final meshes/materials.

Canonical visual authority remains:

`assets/reference/probe/gen1-prime/`

with the ordering defined by that package's `README.md`.

## Visible blockout systems

The Unreal pawn now exposes separate presentation components for:

- main structural spine;
- computation/core housing;
- power/reactor housing;
- main propulsion assembly;
- forward sensor/telescope hardware;
- port and starboard thermal radiators;
- port and starboard maneuvering pods;
- dorsal sensor/orientation mast;
- port and starboard manipulator shoulder mounting points.

These are separate components intentionally. Later damage, replacement, deployment, articulation, and upgrade work must not require converting one monolithic placeholder mesh back into identifiable hardware.

## Scale and collision

The blockout targets an approximately **15 m overall spacecraft length**.

The previous 0.75 m collision sphere belonged to the tiny engineering-shell placeholder and no longer represented the visible body. The authoritative simulation and Unreal mirror now use a conservative **8 m radius bounding sphere** for this first full-scale blockout.

This is intentionally transitional rather than final collision geometry. It should make front/rear/radiator contact occur near the visible spacecraft instead of through it, while the later compound-envelope pass can follow the actual Prime geometry more closely.

The important architecture remains unchanged:

- engine-independent simulation owns contact truth;
- Unreal mirrors/presents the envelope;
- visible system meshes have collision disabled;
- decorative art cannot silently redefine physical behavior.

## Camera

The default third-person orbit has been rescaled for the larger physical body:

- default boom: 26 m;
- minimum distance: 14 m;
- maximum distance: 50 m.

The player should be able to orbit the entire machine and still read fore/aft, dorsal orientation, radiators, engine, sensor hardware, and manipulator mounts.

## Local UE 5.8 test script

1. Launch the exact CI-green build.
2. Confirm the player no longer appears to inhabit a small sphere/ellipsoid placeholder.
3. Orbit the camera around the probe and identify without reading code:
   - long central structural body;
   - forward sensor end;
   - aft propulsion end;
   - two lateral radiators;
   - central system housings;
   - paired maneuvering hardware;
   - two manipulator shoulder mounts.
4. Confirm forward versus aft and dorsal orientation remain immediately readable while rotating.
5. Use mouse orbit/zoom through the full practical range. Confirm the default view frames the whole probe and zoom does not force the camera inside it.
6. Exercise W/S/A/D/Q/E and yaw/pitch/roll. Confirm the larger body still tracks authoritative motion/attitude without visual separation or unexpected scaling.
7. Press `R` from an unusual orientation and confirm camera-aligned righting still works.
8. Fly toward the Phase-2 physical scan body. Confirm contact occurs at a believable distance for the much larger probe rather than allowing the visible nose/engine to pass deeply through the target.
9. Make a glancing contact and confirm the existing slide/deflection behavior remains usable.
10. Scan the target and confirm scan behavior remains unchanged.
11. Inspect the probe from several angles and note any component placement that looks materially inconsistent with the canonical Prime references.

## Acceptance questions

- Does this unmistakably read as a spacecraft/probe rather than a primitive pawn?
- Does the silhouette begin to resemble the canonical Prime/Scientific Explorer family?
- Is roughly 15 m scale believable relative to the environment and camera?
- Can the player immediately distinguish forward, aft, dorsal, port/starboard structure?
- Are the major system regions visually understandable?
- Do radiators look like lateral hardware rather than an accidental solid wing?
- Are manipulator mounts positioned so Slice 6 can add articulated arms without redesigning the whole body?
- Does the 8 m conservative collision envelope feel acceptably close for blockout testing, or is invisible stand-off obvious enough to require compound collision immediately?
- Does the larger body make existing controls feel more appropriately heavy/deliberate rather than merely harder to understand?

## Explicitly not complete in this slice

- production-quality Prime Probe mesh;
- final materials/textures;
- deployable radiator animation;
- articulated manipulator arms;
- exact compound collision;
- per-component collision shapes;
- component detachment/replacement visuals;
- thruster VFX;
- production lighting/audio for the probe;
- final damage decals/deformation.

Those should build on this modular blockout rather than replacing its component architecture.
